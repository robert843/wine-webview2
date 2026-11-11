#![allow(dead_code)] // część API zużyją kolejne przenoszone obiekty

//! Serwer ogólnych wywołań COM (docs/architektura-proxy.md): tabela obiektów runtime'u,
//! referencje do obiektów aplikacji, CALL/REPLY, INVOKE z zagnieżdżoną pętlą oczekiwania.
//!
//! Cały stan żyje na wątku UI CEF. Wątek czytający wrzuca komunikaty do kolejki; opróżnia ją
//! zadanie UI albo pętla oczekiwania synchronicznego INVOKE (model STA, bez zakleszczeń).

use crate::api_gen;
use crate::protocol::*;
use std::any::Any;
use std::cell::{Cell, RefCell};
use std::collections::{HashMap, HashSet, VecDeque};
use std::rc::{Rc, Weak};
use std::sync::{Condvar, Mutex};
use std::time::{Duration, Instant};

pub type WvResult<T> = Result<T, i32>;

pub const S_OK: i32 = 0;
pub const E_NOTIMPL: i32 = 0x8000_4001_u32 as i32;
pub const E_POINTER: i32 = 0x8000_4003_u32 as i32;
pub const E_FAIL: i32 = 0x8000_4005_u32 as i32;
pub const E_INVALIDARG: i32 = 0x8007_0057_u32 as i32;
pub const E_INVALID_STATE: i32 = 0x8007_139f_u32 as i32; // HRESULT_FROM_WIN32(ERROR_INVALID_STATE)

pub const CALL_ASYNC: u32 = 0x1;
pub const ROOT_CREATE_ENVIRONMENT: u32 = 0x8000_0002;
pub const ROOT_CREATE_CONTROLLER: u32 = 0x8000_0003;

const OBJREF_NULL: u32 = 0;
const OBJREF_RUNTIME: u32 = 1;
const OBJREF_APP: u32 = 2;

/// Obiekt runtime'u wywoływany przez pośrednika w DLL.
pub trait Dispatch {
    fn family(&self) -> u32;
    fn as_any(&self) -> &dyn Any;
    fn dispatch(&self, ctx: &mut Ctx, method: u32, rd: &mut Reader, wr: &mut Writer) -> i32;
}

/// Uchwyt obiektu aplikacji; ostatnia kopia zwalnia go po stronie DLL.
pub struct AppHandle {
    pub id: u64,
}

impl Drop for AppHandle {
    fn drop(&mut self) {
        // try_with: przy wyjściu z procesu tablica może być już zniszczona.
        let count = APPS.try_with(|a| a.try_borrow_mut().ok().and_then(|mut a| a.remove(&self.id)).map(|e| e.1)).ok().flatten();
        if let Some(count) = count {
            let payload = Writer::default().u64(self.id).u32(count).u32(0).0;
            crate::resources::send_event(EVT_RELEASE_APP, 0, &payload);
        }
    }
}

pub type AppRef = Rc<AppHandle>;

#[derive(Clone, Default)]
pub enum ObjRef {
    #[default]
    Null,
    Runtime(Rc<dyn Dispatch>),
    App(AppRef),
}

impl ObjRef {
    pub fn is_null(&self) -> bool {
        matches!(self, ObjRef::Null)
    }
    /// Konkretny typ obiektu runtime'u (np. `ICoreWebView2_29Obj<WebView>`).
    pub fn downcast<T: 'static>(&self) -> Option<&T> {
        match self {
            ObjRef::Runtime(rc) => rc.as_any().downcast_ref::<T>(),
            _ => None,
        }
    }
    pub fn app(&self) -> Option<&AppRef> {
        match self {
            ObjRef::App(a) => Some(a),
            _ => None,
        }
    }
}

struct ObjEntry {
    obj: Rc<dyn Dispatch>,
    remote_refs: u32,
}

#[derive(Default)]
struct Objects {
    by_id: HashMap<u64, ObjEntry>,
    by_ptr: HashMap<usize, u64>,
    next_id: u64,
}

thread_local! {
    static OBJECTS: RefCell<Objects> = RefCell::new(Objects { next_id: 1, ..Default::default() });
    static APPS: RefCell<HashMap<u64, (Weak<AppHandle>, u32)>> = RefCell::new(HashMap::new());
    static REPLIES: RefCell<HashMap<u64, (i32, Vec<u8>)>> = RefCell::new(HashMap::new());
    static FIXMES: RefCell<HashSet<&'static str>> = RefCell::new(HashSet::new());
    static CALL_DEPTH: Cell<u32> = const { Cell::new(0) };
    static NEXT_INVOKE: Cell<u64> = const { Cell::new(1) };
}

/// Obsługa obiektu 0 (korzeń) i komunikatów spoza RPC - dostarczane przez main.rs.
pub type RootFn = fn(&mut Ctx, u32, &mut Reader, &mut Writer) -> i32;
pub type MessageFn = fn(Message);
static HANDLERS: Mutex<Option<(RootFn, MessageFn)>> = Mutex::new(None);

pub fn set_handlers(root: RootFn, other: MessageFn) {
    *HANDLERS.lock().unwrap() = Some((root, other));
}

// ---------- kolejka komunikatów (wątek czytający -> wątek UI) ----------

static QUEUE: Mutex<VecDeque<Option<Message>>> = Mutex::new(VecDeque::new());
static QUEUE_CV: Condvar = Condvar::new();

/// Wątek czytający: dokłada komunikat (None = koniec połączenia).
pub fn enqueue(msg: Option<Message>) {
    QUEUE.lock().unwrap().push_back(msg);
    QUEUE_CV.notify_all();
}

fn pop(timeout: Option<Duration>) -> Option<Option<Message>> {
    let mut q = QUEUE.lock().unwrap();
    if q.is_empty() {
        if let Some(t) = timeout {
            q = QUEUE_CV.wait_timeout(q, t).unwrap().0;
        }
    }
    q.pop_front()
}

/// Zadanie UI: przetwarza wszystko, co czeka w kolejce. Zwraca false po końcu połączenia.
pub fn drain() -> bool {
    while let Some(msg) = pop(None) {
        match msg {
            Some(m) => process(m),
            None => return false,
        }
    }
    true
}

fn process(msg: Message) {
    match msg.ty {
        CMD_CALL => handle_call(&msg.payload),
        CMD_INVOKE_REPLY => {
            let mut r = Reader(&msg.payload);
            let (Some(id), Some(hr), Some(_)) = (r.u64(), r.u32(), r.u32()) else { return };
            REPLIES.with(|m| m.borrow_mut().insert(id, (hr as i32, r.rest().to_vec())));
        }
        CMD_RELEASE => {
            let mut r = Reader(&msg.payload);
            let (Some(id), Some(count)) = (r.u64(), r.u32()) else { return };
            OBJECTS.with(|o| {
                let mut o = o.borrow_mut();
                let gone = match o.by_id.get_mut(&id) {
                    Some(e) => {
                        e.remote_refs = e.remote_refs.saturating_sub(count);
                        e.remote_refs == 0
                    }
                    None => false,
                };
                if gone {
                    if let Some(e) = o.by_id.remove(&id) {
                        let ptr = Rc::as_ptr(&e.obj) as *const () as usize;
                        o.by_ptr.remove(&ptr);
                    }
                }
            });
        }
        _ => {
            let other = HANDLERS.lock().unwrap().map(|h| h.1);
            if let Some(f) = other {
                f(msg);
            }
        }
    }
}

fn handle_call(payload: &[u8]) {
    let mut r = Reader(payload);
    let (Some(call_id), Some(object), Some(method), Some(_flags)) = (r.u64(), r.u64(), r.u32(), r.u32()) else { return };
    let mut wr = Writer::default();
    let mut ctx = Ctx;
    CALL_DEPTH.with(|d| d.set(d.get() + 1));
    let hr = if object == 0 {
        let root = HANDLERS.lock().unwrap().map(|h| h.0);
        root.map(|f| f(&mut ctx, method, &mut r, &mut wr)).unwrap_or(E_FAIL)
    } else {
        let obj = OBJECTS.with(|o| o.borrow().by_id.get(&object).map(|e| e.obj.clone()));
        match obj {
            Some(obj) => obj.dispatch(&mut ctx, method, &mut r, &mut wr),
            None => {
                eprintln!("helper: call {method} on unknown object {object}");
                E_INVALID_STATE
            }
        }
    };
    CALL_DEPTH.with(|d| d.set(d.get() - 1));
    let mut reply = Writer::default().u64(call_id).u32(hr as u32).u32(0);
    reply.0.extend_from_slice(&wr.0);
    crate::resources::send_event(EVT_REPLY, 0, &reply.0);
}

/// Token kontekstu wywołań (stan jest w thread_local wątku UI CEF).
pub struct Ctx;

impl Ctx {
    pub fn fixme(&mut self, what: &'static str) {
        if FIXMES.with(|f| f.borrow_mut().insert(what)) {
            eprintln!("helper: fixme {what} stub!");
        }
    }

    /// Obiekt-zaślepka rodziny (dla domyślnych wyników).
    pub fn stub(&mut self, family: u32) -> ObjRef {
        api_gen::make_stub(family).map(ObjRef::Runtime).unwrap_or_default()
    }

    pub fn read_objref(&mut self, rd: &mut Reader) -> ObjRef {
        let (Some(kind), Some(_family), Some(id)) = (rd.u32(), rd.u32(), rd.u64()) else { return ObjRef::Null };
        match kind {
            OBJREF_RUNTIME => OBJECTS.with(|o| o.borrow().by_id.get(&id).map(|e| ObjRef::Runtime(e.obj.clone()))).unwrap_or_else(|| {
                eprintln!("helper: unknown runtime object {id}");
                ObjRef::Null
            }),
            OBJREF_APP => APPS.with(|a| {
                let mut a = a.borrow_mut();
                let entry = a.entry(id).or_insert_with(|| (Weak::new(), 0));
                entry.1 += 1;
                let handle = entry.0.upgrade().unwrap_or_else(|| {
                    let h = Rc::new(AppHandle { id });
                    entry.0 = Rc::downgrade(&h);
                    h
                });
                ObjRef::App(handle)
            }),
            _ => ObjRef::Null,
        }
    }

    pub fn write_objref(&mut self, wr: &mut Writer, r: &ObjRef) {
        match r {
            ObjRef::Null => {
                wr.put_u32(OBJREF_NULL);
                wr.put_u32(0);
                wr.put_u64(0);
            }
            ObjRef::Runtime(obj) => {
                let family = obj.family();
                let id = OBJECTS.with(|o| {
                    let mut o = o.borrow_mut();
                    let ptr = Rc::as_ptr(obj) as *const () as usize;
                    let id = match o.by_ptr.get(&ptr) {
                        Some(&id) => id,
                        None => {
                            let id = o.next_id;
                            o.next_id += 1;
                            o.by_ptr.insert(ptr, id);
                            o.by_id.insert(id, ObjEntry { obj: obj.clone(), remote_refs: 0 });
                            id
                        }
                    };
                    if let Some(e) = o.by_id.get_mut(&id) {
                        e.remote_refs += 1;
                    }
                    id
                });
                wr.put_u32(OBJREF_RUNTIME);
                wr.put_u32(family);
                wr.put_u64(id);
            }
            ObjRef::App(a) => {
                wr.put_u32(OBJREF_APP);
                wr.put_u32(0);
                wr.put_u64(a.id);
            }
        }
    }

    fn send_invoke(app: &AppRef, method: u32, args: Writer, flags: u32) -> u64 {
        let call_id = NEXT_INVOKE.with(|n| {
            let id = n.get();
            n.set(id + 1);
            id
        });
        let mut msg = Writer::default().u64(call_id).u64(app.id).u32(method).u32(flags);
        msg.0.extend_from_slice(&args.0);
        if std::env::var_os("WINE_WEBVIEW2_DEBUG_RPC").is_some() {
            eprintln!("helper: invoke {call_id} app {} method {method} flags {flags}", app.id);
        }
        crate::resources::send_event(EVT_INVOKE, 0, &msg.0);
        call_id
    }

    /// Wywołanie asynchroniczne metody obiektu aplikacji (kolejność zachowana).
    pub fn invoke_async(&mut self, app: &AppRef, method: u32, args: Writer) {
        Self::send_invoke(app, method, args, CALL_ASYNC);
    }

    /// Wywołanie synchroniczne: czeka na wynik, w międzyczasie obsługuje przychodzące komunikaty.
    pub fn invoke(&mut self, app: &AppRef, method: u32, args: Writer) -> WvResult<Vec<u8>> {
        if CALL_DEPTH.with(|d| d.get()) > 0 {
            // Reguła z docs/architektura-proxy.md: DLL czeka na REPLY, nie obsłuży INVOKE.
            eprintln!("helper: synchronous invoke {method} inside a call, sending asynchronously");
            Self::send_invoke(app, method, args, CALL_ASYNC);
            return Err(E_FAIL);
        }
        let call_id = Self::send_invoke(app, method, args, 0);
        let start = Instant::now();
        let mut warned = false;
        loop {
            if let Some((hr, data)) = REPLIES.with(|m| m.borrow_mut().remove(&call_id)) {
                return if hr >= 0 { Ok(data) } else { Err(hr) };
            }
            match pop(Some(Duration::from_millis(100))) {
                Some(Some(m)) => process(m),
                Some(None) => {
                    enqueue(None); // koniec połączenia - niech zobaczy go też główna pętla
                    return Err(E_FAIL);
                }
                None => {
                    if !warned && start.elapsed() > Duration::from_secs(10) {
                        eprintln!("helper: application still handling invoke {method} after 10 s");
                        warned = true;
                    }
                }
            }
        }
    }
}

// ---------- kodowanie wartości ----------

impl Writer {
    pub fn put_u32(&mut self, v: u32) {
        self.0.extend_from_slice(&v.to_le_bytes());
    }
    pub fn put_u64(&mut self, v: u64) {
        self.0.extend_from_slice(&v.to_le_bytes());
    }
    pub fn put_str(&mut self, s: Option<&str>) {
        match s {
            Some(s) => {
                self.put_u32(s.len() as u32);
                self.0.extend_from_slice(s.as_bytes());
            }
            None => self.put_u32(u32::MAX),
        }
    }
    pub fn put_stream(&mut self, data: Option<&[u8]>) {
        match data {
            Some(d) => {
                self.put_u32(d.len() as u32);
                self.0.extend_from_slice(d);
            }
            None => self.put_u32(u32::MAX),
        }
    }
    /// Struktura o stałym układzie (#[repr(C)], te same co w C).
    pub fn put_pod<T: Copy>(&mut self, v: &T) {
        let bytes = unsafe { std::slice::from_raw_parts(v as *const T as *const u8, std::mem::size_of::<T>()) };
        self.0.extend_from_slice(bytes);
    }
}

impl Reader<'_> {
    pub fn str_opt(&mut self) -> Option<String> {
        let len = self.u32()?;
        if len == u32::MAX {
            return None;
        }
        self.take(len as usize).map(|b| String::from_utf8_lossy(b).into_owned())
    }
    pub fn stream(&mut self) -> Option<Vec<u8>> {
        let len = self.u32()?;
        if len == u32::MAX {
            return None;
        }
        self.take(len as usize).map(|b| b.to_vec())
    }
    pub fn pod<T: Copy + Default>(&mut self) -> T {
        let mut v = T::default();
        if let Some(b) = self.take(std::mem::size_of::<T>()) {
            unsafe { std::ptr::copy_nonoverlapping(b.as_ptr(), &mut v as *mut T as *mut u8, b.len()) };
        }
        v
    }
}
