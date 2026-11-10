//! wine-webview2-helper — natywny (linuksowy) proces przeglądarki dla wine-webview2.
//!
//! Tryby:
//!   --ipc-fd=N                     normalna praca: polecenia z EmbeddedBrowserWebView.dll
//!                                  przez gniazdo N (protokół: protocol.rs / protocol.h)
//!   --screenshot <url> <plik.ppm>  test: renderuje stronę off-screen i zapisuje klatkę
//!
//! Ten sam plik wykonywalny jest też podprocesem CEF (renderer, GPU, …): CEF uruchamia go
//! z `--type=…`, a `execute_process` obsługuje taki przypadek i kończy proces.

mod ipc;
mod protocol;
mod renderer;

use cef::args::Args;
use cef::rc::Rc as _;
use cef::*;
use ipc::{Channel, FrameBuffer};
use protocol::*;
use std::cell::RefCell;
use std::collections::HashMap;
use std::os::fd::AsRawFd;
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::{Arc, Mutex, OnceLock};

/// Katalog plików ze skryptami startowymi (przekazywany rendererom przełącznikiem).
static SCRIPTS_DIR: OnceLock<PathBuf> = OnceLock::new();

// ======================================================================
// Wspólne: zadania na wątku UI CEF
// ======================================================================

type BoxedFn = Box<dyn FnOnce() + Send>;

#[derive(Clone)]
struct ClosureTask {
    f: Arc<Mutex<Option<BoxedFn>>>,
}

wrap_task! {
    struct TaskBuilder {
        task: ClosureTask,
    }

    impl Task {
        fn execute(&self) {
            let f = self.task.f.lock().unwrap().take();
            if let Some(f) = f {
                f();
            }
        }
    }
}

fn post_ui(f: impl FnOnce() + Send + 'static) {
    let mut task = TaskBuilder::new(ClosureTask { f: Arc::new(Mutex::new(Some(Box::new(f)))) });
    post_task(ThreadId::UI, Some(&mut task));
}

// ======================================================================
// Tryb IPC
// ======================================================================

/// ARGB, jak COREWEBVIEW2_COLOR domyślnie (biały, nieprzezroczysty).
const DEFAULT_BACKGROUND: u32 = 0xFFFF_FFFF;

struct View {
    browser: Option<Browser>,
    doc_scripts: Vec<String>,
    width: u32,
    height: u32,
    frames: Option<FrameBuffer>,
    dropped: bool,
}

struct IpcState {
    chan: Channel,
    views: HashMap<u32, View>,
    next_generation: u64,
}

thread_local! {
    /// Stan trybu IPC — używany wyłącznie na wątku UI CEF.
    static STATE: RefCell<Option<IpcState>> = const { RefCell::new(None) };
}

fn with_state<R>(f: impl FnOnce(&mut IpcState) -> R) -> Option<R> {
    STATE.with(|s| s.borrow_mut().as_mut().map(f))
}

fn send_event(ty: u32, view: u32, payload: &[u8]) {
    with_state(|st| {
        if let Err(e) = st.chan.send(ty, view, payload, None) {
            eprintln!("helper: send failed: {e}");
        }
    });
}

fn base64(data: &[u8]) -> String {
    const T: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut out = String::with_capacity(data.len().div_ceil(3) * 4);
    for c in data.chunks(3) {
        let n = (c[0] as u32) << 16 | (*c.get(1).unwrap_or(&0) as u32) << 8 | *c.get(2).unwrap_or(&0) as u32;
        out.push(T[(n >> 18) as usize & 63] as char);
        out.push(T[(n >> 12) as usize & 63] as char);
        out.push(if c.len() > 1 { T[(n >> 6) as usize & 63] as char } else { '=' });
        out.push(if c.len() > 2 { T[n as usize & 63] as char } else { '=' });
    }
    out
}

#[derive(Clone)]
struct ViewClient {
    view: u32,
}

wrap_client! {
    struct ViewClientBuilder {
        client: ViewClient,
    }

    impl Client {
        fn render_handler(&self) -> Option<RenderHandler> {
            Some(ViewRenderHandlerBuilder::new(self.client.clone()))
        }

        fn load_handler(&self) -> Option<LoadHandler> {
            Some(ViewLoadHandlerBuilder::new(self.client.clone()))
        }

        fn display_handler(&self) -> Option<DisplayHandler> {
            Some(ViewDisplayHandlerBuilder::new(self.client.clone()))
        }

        fn on_process_message_received(
            &self,
            _browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            _source_process: ProcessId,
            message: Option<&mut ProcessMessage>,
        ) -> ::std::os::raw::c_int {
            let Some(message) = message else { return 0 };
            let name = CefString::from(&message.name()).to_string();
            let Some(args) = message.argument_list() else { return 0 };
            match name.as_str() {
                renderer::MSG_POST => {
                    let json = CefString::from(&args.string(0)).to_string();
                    let source = frame.map(|f| CefString::from(&f.url()).to_string()).unwrap_or_default();
                    let payload = Writer::default()
                        .u32(source.len() as u32)
                        .u32(0)
                        .bytes(source.as_bytes())
                        .bytes(json.as_bytes())
                        .0;
                    send_event(EVT_WEB_MESSAGE, self.client.view, &payload);
                    1
                }
                renderer::MSG_RESULT => {
                    let id = CefString::from(&args.string(0)).to_string().parse::<u64>().unwrap_or(0);
                    let json = CefString::from(&args.string(1)).to_string();
                    send_event(EVT_SCRIPT_RESULT, self.client.view, &Writer::default().u64(id).bytes(json.as_bytes()).0);
                    1
                }
                _ => 0,
            }
        }
    }
}

wrap_render_handler! {
    struct ViewRenderHandlerBuilder {
        client: ViewClient,
    }

    impl RenderHandler {
        fn view_rect(&self, _browser: Option<&mut Browser>, rect: Option<&mut Rect>) {
            let Some(rect) = rect else { return };
            let (w, h) = with_state(|st| st.views.get(&self.client.view).map(|v| (v.width, v.height)))
                .flatten()
                .unwrap_or((1, 1));
            rect.x = 0;
            rect.y = 0;
            rect.width = w.max(1) as i32;
            rect.height = h.max(1) as i32;
        }

        fn on_paint(
            &self,
            browser: Option<&mut Browser>,
            type_: PaintElementType,
            dirty_rects: Option<&[Rect]>,
            buffer: *const u8,
            width: ::std::os::raw::c_int,
            height: ::std::os::raw::c_int,
        ) {
            if type_ != PaintElementType::VIEW || buffer.is_null() || width <= 0 || height <= 0 {
                return;
            }
            let src = unsafe { std::slice::from_raw_parts(buffer, (width * height * 4) as usize) };
            let dirty = dirty_rects.unwrap_or(&[]).iter().fold(None::<(i32, i32, i32, i32)>, |acc, r| {
                let (l, t, rr, b) = (r.x, r.y, r.x + r.width, r.y + r.height);
                Some(match acc {
                    None => (l, t, rr, b),
                    Some((al, at, ar, ab)) => (al.min(l), at.min(t), ar.max(rr), ab.max(b)),
                })
            }).unwrap_or((0, 0, width, height));
            let invalidate = with_state(|st| paint_view(st, self.client.view, src, width as u32, height as u32, dirty));
            if invalidate == Some(true) {
                if let Some(host) = browser.and_then(|b| b.host()) {
                    host.invalidate(PaintElementType::VIEW);
                }
            }
        }
    }
}

/// Zapisuje klatkę do wolnego slotu memfd i wysyła FRAME_READY. Zwraca true, gdy trzeba
/// natychmiast poprosić o nową klatkę (nie powinno się zdarzać).
fn paint_view(st: &mut IpcState, view_id: u32, src: &[u8], width: u32, height: u32, dirty: (i32, i32, i32, i32)) -> bool {
    let Some(view) = st.views.get_mut(&view_id) else { return false };

    let need_new = view.frames.as_ref().is_none_or(|f| f.width != width || f.height != height);
    if need_new {
        st.next_generation += 1;
        match FrameBuffer::new(width, height, st.next_generation) {
            Ok(fb) => {
                if let Err(e) = st.chan.send(EVT_FRAME_BUFFER, view_id, &fb.describe(), Some(fb.fd.as_raw_fd())) {
                    eprintln!("helper: cannot send frame buffer: {e}");
                    return false;
                }
                view.frames = Some(fb);
            }
            Err(e) => {
                eprintln!("helper: cannot create frame buffer: {e}");
                return false;
            }
        }
    }
    let fb = view.frames.as_mut().unwrap();
    let Some(slot) = fb.busy.iter().position(|b| !b) else {
        // DLL trzyma oba sloty — pomijamy klatkę, odświeżymy po FRAME_DONE.
        view.dropped = true;
        return false;
    };
    fb.slot_mut(slot)[..src.len()].copy_from_slice(src);
    fb.busy[slot] = true;
    let payload = Writer::default()
        .u64(fb.generation)
        .u32(slot as u32)
        .u32(0)
        .i32(dirty.0)
        .i32(dirty.1)
        .i32(dirty.2)
        .i32(dirty.3)
        .0;
    if let Err(e) = st.chan.send(EVT_FRAME_READY, view_id, &payload, None) {
        eprintln!("helper: send failed: {e}");
    }
    false
}

wrap_display_handler! {
    struct ViewDisplayHandlerBuilder {
        client: ViewClient,
    }

    impl DisplayHandler {
        fn on_cursor_change(
            &self,
            _browser: Option<&mut Browser>,
            _cursor: ::std::os::raw::c_ulong,
            type_: CursorType,
            _custom_cursor_info: Option<&CursorInfo>,
        ) -> ::std::os::raw::c_int {
            let kind = *type_.as_ref() as u32;
            send_event(EVT_CURSOR, self.client.view, &Writer::default().u32(kind).u32(0).0);
            1
        }
    }
}

wrap_load_handler! {
    struct ViewLoadHandlerBuilder {
        client: ViewClient,
    }

    impl LoadHandler {
        fn on_loading_state_change(
            &self,
            _browser: Option<&mut Browser>,
            is_loading: ::std::os::raw::c_int,
            _can_go_back: ::std::os::raw::c_int,
            _can_go_forward: ::std::os::raw::c_int,
        ) {
            send_event(EVT_LOAD_STATE, self.client.view, &Writer::default().u32((is_loading != 0) as u32).u32(0).0);
        }
    }
}

/// Zapisuje skrypty startowe przeglądarki do pliku czytanego przez renderer (zapis atomowy).
fn write_doc_scripts(dir: &std::path::Path, browser_id: i32, scripts: &[String]) {
    let mut data = Vec::new();
    for s in scripts {
        data.extend_from_slice(&(s.len() as u32).to_le_bytes());
        data.extend_from_slice(s.as_bytes());
    }
    let path = renderer::scripts_file(&dir.to_string_lossy(), browser_id);
    let tmp = path.with_extension("tmp");
    if let Err(e) = std::fs::write(&tmp, &data).and_then(|_| std::fs::rename(&tmp, &path)) {
        eprintln!("helper: cannot write {}: {e}", path.display());
    }
}

fn view_browser(view: u32) -> Option<Browser> {
    with_state(|st| st.views.get(&view).and_then(|v| v.browser.clone())).flatten()
}

fn shutdown_ipc() {
    let browsers: Vec<Browser> =
        with_state(|st| st.views.drain().filter_map(|(_, v)| v.browser).collect()).unwrap_or_default();
    for b in browsers {
        if let Some(host) = b.host() {
            host.close_browser(1);
        }
    }
    quit_message_loop();
}

/// Obsługa jednego polecenia na wątku UI CEF.
fn handle_command(msg: Message) {
    let mut r = Reader(&msg.payload);
    match msg.ty {
        CMD_CREATE_VIEW => {
            let (w, h) = (r.u32().unwrap_or(1), r.u32().unwrap_or(1));
            with_state(|st| {
                st.views.insert(msg.view, View { browser: None, doc_scripts: Vec::new(), width: w, height: h, frames: None, dropped: false })
            });
            let window_info = WindowInfo { windowless_rendering_enabled: true as _, ..Default::default() };
            // WebView2: DefaultBackgroundColor = nieprzezroczysta biel (CEF OSR domyślnie: przezroczyste).
            let settings = BrowserSettings {
                windowless_frame_rate: 60,
                background_color: DEFAULT_BACKGROUND,
                ..Default::default()
            };
            let mut client = ViewClientBuilder::new(ViewClient { view: msg.view });
            let browser = browser_host_create_browser_sync(
                Some(&window_info),
                Some(&mut client),
                Some(&"about:blank".into()),
                Some(&settings),
                None,
                None,
            );
            if browser.is_none() {
                send_event(EVT_ERROR, msg.view, b"cannot create browser");
                return;
            }
            with_state(|st| {
                if let Some(v) = st.views.get_mut(&msg.view) {
                    v.browser = browser;
                }
            });
            send_event(EVT_VIEW_CREATED, msg.view, &[]);
        }
        CMD_CLOSE_VIEW => {
            if let Some(view) = with_state(|st| st.views.remove(&msg.view)).flatten() {
                if let Some(host) = view.browser.and_then(|b| b.host()) {
                    host.close_browser(1);
                }
            }
        }
        CMD_RESIZE => {
            let (w, h) = (r.u32().unwrap_or(1), r.u32().unwrap_or(1));
            with_state(|st| {
                if let Some(v) = st.views.get_mut(&msg.view) {
                    v.width = w.max(1);
                    v.height = h.max(1);
                }
            });
            if let Some(host) = view_browser(msg.view).and_then(|b| b.host()) {
                host.was_resized();
            }
        }
        CMD_SET_VISIBLE => {
            let visible = r.u32().unwrap_or(1) != 0;
            if let Some(host) = view_browser(msg.view).and_then(|b| b.host()) {
                host.was_hidden((!visible) as i32);
            }
        }
        CMD_NAVIGATE | CMD_NAVIGATE_TO_STRING => {
            let text = r.rest_str();
            let url = if msg.ty == CMD_NAVIGATE {
                text
            } else {
                format!("data:text/html;charset=utf-8;base64,{}", base64(text.as_bytes()))
            };
            if let Some(frame) = view_browser(msg.view).and_then(|b| b.main_frame()) {
                frame.load_url(Some(&url.as_str().into()));
            }
        }
        CMD_EXECUTE_SCRIPT => {
            let id = r.u64().unwrap_or(0);
            let script = r.rest_str();
            let frame = view_browser(msg.view).and_then(|b| b.main_frame());
            let sent = frame.and_then(|frame| {
                let mut m = process_message_create(Some(&renderer::MSG_EXEC.into()))?;
                let list = m.argument_list()?;
                list.set_string(0, Some(&id.to_string().as_str().into()));
                list.set_string(1, Some(&script.as_str().into()));
                frame.send_process_message(ProcessId::RENDERER, Some(&mut m));
                Some(())
            });
            if sent.is_none() {
                send_event(EVT_SCRIPT_RESULT, msg.view, &Writer::default().u64(id).bytes(b"null").0);
            }
        }
        CMD_ADD_DOCUMENT_SCRIPT => {
            let _id = r.u64();
            let script = r.rest_str();
            let target = with_state(|st| {
                let v = st.views.get_mut(&msg.view)?;
                v.doc_scripts.push(script);
                Some((v.browser.as_ref()?.identifier(), v.doc_scripts.clone()))
            })
            .flatten();
            if let (Some((browser_id, scripts)), Some(dir)) = (target, SCRIPTS_DIR.get()) {
                write_doc_scripts(dir, browser_id, &scripts);
            }
        }
        CMD_POST_WEB_MESSAGE => {
            let json = r.rest_str();
            if let Some(frame) = view_browser(msg.view).and_then(|b| b.main_frame()) {
                let code = format!("window.chrome&&window.chrome.webview&&window.chrome.webview.__dispatch({json});");
                frame.execute_java_script(Some(&code.as_str().into()), Some(&"".into()), 0);
            }
        }
        CMD_FRAME_DONE => {
            let (generation, slot) = (r.u64().unwrap_or(0), r.u32().unwrap_or(u32::MAX) as usize);
            let refresh = with_state(|st| {
                let v = st.views.get_mut(&msg.view)?;
                let fb = v.frames.as_mut()?;
                if fb.generation == generation && slot < fb.busy.len() {
                    fb.busy[slot] = false;
                }
                Some(std::mem::take(&mut v.dropped))
            })
            .flatten()
            .unwrap_or(false);
            if refresh {
                if let Some(host) = view_browser(msg.view).and_then(|b| b.host()) {
                    host.invalidate(PaintElementType::VIEW);
                }
            }
        }
        CMD_MOUSE => {
            let Some(host) = view_browser(msg.view).and_then(|b| b.host()) else { return };
            let (kind, x, y) = (r.u32().unwrap_or(0), r.i32().unwrap_or(0), r.i32().unwrap_or(0));
            let (button, clicks, modifiers) = (r.u32().unwrap_or(0), r.u32().unwrap_or(1), r.u32().unwrap_or(0));
            let (dx, dy) = (r.i32().unwrap_or(0), r.i32().unwrap_or(0));
            let event = MouseEvent { x, y, modifiers };
            let button = match button {
                1 => MouseButtonType::MIDDLE,
                2 => MouseButtonType::RIGHT,
                _ => MouseButtonType::LEFT,
            };
            match kind {
                MOUSE_MOVE => host.send_mouse_move_event(Some(&event), 0),
                MOUSE_LEAVE => host.send_mouse_move_event(Some(&event), 1),
                MOUSE_DOWN => host.send_mouse_click_event(Some(&event), button, 0, clicks as i32),
                MOUSE_UP => host.send_mouse_click_event(Some(&event), button, 1, clicks as i32),
                MOUSE_WHEEL => host.send_mouse_wheel_event(Some(&event), dx, dy),
                _ => {}
            }
        }
        CMD_KEY => {
            let Some(host) = view_browser(msg.view).and_then(|b| b.host()) else { return };
            let (kind, modifiers) = (r.u32().unwrap_or(0), r.u32().unwrap_or(0));
            let (windows_key_code, native_key_code) = (r.i32().unwrap_or(0), r.i32().unwrap_or(0));
            let is_system_key = r.u32().unwrap_or(0) as i32;
            let (character, unmodified_character) = (r.u16().unwrap_or(0), r.u16().unwrap_or(0));
            let type_ = match kind {
                1 => KeyEventType::KEYDOWN,
                2 => KeyEventType::KEYUP,
                3 => KeyEventType::CHAR,
                _ => KeyEventType::RAWKEYDOWN,
            };
            let event = KeyEvent {
                type_,
                modifiers,
                windows_key_code,
                native_key_code,
                is_system_key,
                character,
                unmodified_character,
                ..Default::default()
            };
            host.send_key_event(Some(&event));
        }
        CMD_FOCUS => {
            let focus = r.u32().unwrap_or(0);
            if let Some(host) = view_browser(msg.view).and_then(|b| b.host()) {
                host.set_focus(focus as i32);
            }
        }
        CMD_SHUTDOWN => shutdown_ipc(),
        other => eprintln!("helper: unknown command {other:#x}"),
    }
}

fn reader_thread(chan: Channel) {
    loop {
        match chan.recv() {
            Ok(Some(msg)) => post_ui(move || handle_command(msg)),
            Ok(None) => break,
            Err(e) => {
                eprintln!("helper: recv failed: {e}");
                break;
            }
        }
    }
    post_ui(shutdown_ipc);
}

// ======================================================================
// Tryb zrzutu (test)
// ======================================================================

#[derive(Default)]
struct Shot {
    url: String,
    out: PathBuf,
    loaded: bool,
    done: bool,
}

type SharedShot = Rc<RefCell<Shot>>;

#[derive(Clone)]
struct ShotClient {
    shot: SharedShot,
}

wrap_client! {
    struct ShotClientBuilder {
        client: ShotClient,
    }

    impl Client {
        fn render_handler(&self) -> Option<RenderHandler> {
            Some(ShotRenderHandlerBuilder::new(self.client.clone()))
        }

        fn load_handler(&self) -> Option<LoadHandler> {
            Some(ShotLoadHandlerBuilder::new(self.client.clone()))
        }
    }
}

wrap_render_handler! {
    struct ShotRenderHandlerBuilder {
        client: ShotClient,
    }

    impl RenderHandler {
        fn view_rect(&self, _browser: Option<&mut Browser>, rect: Option<&mut Rect>) {
            if let Some(rect) = rect {
                *rect = Rect { x: 0, y: 0, width: 800, height: 600 };
            }
        }

        fn on_paint(
            &self,
            _browser: Option<&mut Browser>,
            type_: PaintElementType,
            _dirty_rects: Option<&[Rect]>,
            buffer: *const u8,
            width: ::std::os::raw::c_int,
            height: ::std::os::raw::c_int,
        ) {
            let mut shot = self.client.shot.borrow_mut();
            if shot.done || !shot.loaded || type_ != PaintElementType::VIEW || buffer.is_null() {
                return;
            }
            let pixels = unsafe { std::slice::from_raw_parts(buffer, (width * height * 4) as usize) };
            match write_ppm(&shot.out, pixels, width as usize, height as usize) {
                Ok(()) => eprintln!("helper: saved {}x{} frame to {}", width, height, shot.out.display()),
                Err(e) => eprintln!("helper: cannot write {}: {e}", shot.out.display()),
            }
            shot.done = true;
            quit_message_loop();
        }
    }
}

wrap_load_handler! {
    struct ShotLoadHandlerBuilder {
        client: ShotClient,
    }

    impl LoadHandler {
        fn on_loading_state_change(
            &self,
            browser: Option<&mut Browser>,
            is_loading: ::std::os::raw::c_int,
            _can_go_back: ::std::os::raw::c_int,
            _can_go_forward: ::std::os::raw::c_int,
        ) {
            if is_loading != 0 {
                return;
            }
            self.client.shot.borrow_mut().loaded = true;
            if let Some(host) = browser.and_then(|b| b.host()) {
                host.invalidate(PaintElementType::VIEW);
            }
        }
    }
}

/// CEF podaje BGRA; PPM (P6) to RGB.
fn write_ppm(path: &PathBuf, bgra: &[u8], w: usize, h: usize) -> std::io::Result<()> {
    let mut data = format!("P6\n{w} {h}\n255\n").into_bytes();
    data.reserve(w * h * 3);
    for px in bgra.chunks_exact(4) {
        data.extend_from_slice(&[px[2], px[1], px[0]]);
    }
    std::fs::write(path, data)
}

// ======================================================================
// App
// ======================================================================

#[derive(Clone)]
enum Mode {
    Subprocess,
    Ipc(Arc<Mutex<Option<Channel>>>),
    Shot(SharedShot),
}

#[derive(Clone)]
struct HelperApp {
    mode: Mode,
}

wrap_app! {
    struct AppBuilder {
        app: HelperApp,
    }

    impl App {
        fn on_before_command_line_processing(
            &self,
            _process_type: Option<&CefStringUtf16>,
            command_line: Option<&mut CommandLine>,
        ) {
            let Some(cl) = command_line else { return };
            cl.append_switch(Some(&"no-startup-window".into()));
            cl.append_switch(Some(&"noerrdialogs".into()));
            cl.append_switch(Some(&"use-mock-keychain".into()));
            cl.append_switch(Some(&"password-store=basic".into()));
        }

        fn browser_process_handler(&self) -> Option<BrowserProcessHandler> {
            match &self.app.mode {
                Mode::Subprocess => None,
                mode => Some(ProcessHandlerBuilder::new(ProcessHandler { mode: mode.clone() })),
            }
        }

        fn render_process_handler(&self) -> Option<RenderProcessHandler> {
            match &self.app.mode {
                Mode::Subprocess => Some(renderer::RendererBuilder::new(renderer::Renderer)),
                _ => None,
            }
        }
    }
}

#[derive(Clone)]
struct ProcessHandler {
    mode: Mode,
}

wrap_browser_process_handler! {
    struct ProcessHandlerBuilder {
        handler: ProcessHandler,
    }

    impl BrowserProcessHandler {
        fn on_before_child_process_launch(&self, command_line: Option<&mut CommandLine>) {
            if let (Some(cl), Some(dir)) = (command_line, SCRIPTS_DIR.get()) {
                cl.append_switch_with_value(
                    Some(&renderer::SCRIPTS_DIR_SWITCH.into()),
                    Some(&dir.to_string_lossy().as_ref().into()),
                );
            }
        }

        fn on_context_initialized(&self) {
            match &self.handler.mode {
                Mode::Ipc(chan) => {
                    let Some(chan) = chan.lock().unwrap().take() else { return };
                    let reader = match chan.try_clone() {
                        Ok(r) => r,
                        Err(e) => {
                            eprintln!("helper: cannot clone channel: {e}");
                            quit_message_loop();
                            return;
                        }
                    };
                    STATE.with(|s| {
                        *s.borrow_mut() = Some(IpcState { chan, views: HashMap::new(), next_generation: 0 })
                    });
                    std::thread::spawn(move || reader_thread(reader));
                }
                Mode::Shot(shot) => {
                    let url = shot.borrow().url.clone();
                    let window_info = WindowInfo { windowless_rendering_enabled: true as _, ..Default::default() };
                    let settings = BrowserSettings { windowless_frame_rate: 30, ..Default::default() };
                    let mut client = ShotClientBuilder::new(ShotClient { shot: shot.clone() });
                    if browser_host_create_browser(
                        Some(&window_info),
                        Some(&mut client),
                        Some(&url.as_str().into()),
                        Some(&settings),
                        None,
                        None,
                    ) != 1
                    {
                        eprintln!("helper: browser_host_create_browser failed");
                        quit_message_loop();
                    }
                }
                Mode::Subprocess => {}
            }
        }
    }
}

/// Katalog z libcef.so i zasobami (icudtl.dat, *.pak, locales/).
fn cef_dir() -> PathBuf {
    if let Some(dir) = std::env::var_os("WINE_WEBVIEW2_CEF_DIR") {
        return PathBuf::from(dir);
    }
    std::env::current_exe().ok().and_then(|p| p.parent().map(|p| p.to_path_buf())).unwrap_or_default()
}

/// Uzgodnienie wersji protokołu przed startem CEF.
fn handshake(chan: &Channel) -> Result<(), String> {
    let msg = chan.recv().map_err(|e| e.to_string())?.ok_or("connection closed")?;
    let version = Reader(&msg.payload).u32();
    if msg.ty != CMD_HELLO || version != Some(PROTOCOL_VERSION) {
        let _ = chan.send(EVT_ERROR, 0, b"protocol version mismatch", None);
        return Err(format!("bad hello (type {:#x}, version {:?})", msg.ty, version));
    }
    let reply = Writer::default().u32(PROTOCOL_VERSION).u32(std::process::id()).0;
    chan.send(EVT_HELLO, 0, &reply, None).map_err(|e| e.to_string())
}

fn main() -> std::process::ExitCode {
    let _ = api_hash(sys::CEF_API_VERSION_LAST, 0);

    let argv: Vec<String> = std::env::args().collect();
    let ipc_fd = argv.iter().find_map(|a| a.strip_prefix("--ipc-fd=")).and_then(|v| v.parse::<i32>().ok());
    let is_subprocess = argv.iter().any(|a| a.starts_with("--type="));

    let mode = if is_subprocess {
        Mode::Subprocess
    } else if let Some(fd) = ipc_fd {
        let chan = unsafe { Channel::from_raw(fd) };
        if let Err(e) = handshake(&chan) {
            eprintln!("helper: handshake failed: {e}");
            return std::process::ExitCode::from(1);
        }
        Mode::Ipc(Arc::new(Mutex::new(Some(chan))))
    } else if let Some(i) = argv.iter().position(|a| a == "--screenshot").filter(|i| argv.len() > i + 2) {
        Mode::Shot(Rc::new(RefCell::new(Shot {
            url: argv[i + 1].clone(),
            out: PathBuf::from(&argv[i + 2]),
            ..Default::default()
        })))
    } else {
        eprintln!("użycie: wine-webview2-helper --ipc-fd=N | --screenshot <url> <plik.ppm>");
        return std::process::ExitCode::from(2);
    };

    let args = Args::new();
    let mut app = AppBuilder::new(HelperApp { mode: mode.clone() });

    // Podprocesy CEF (renderer, gpu, utility) kończą się tutaj.
    let ret = execute_process(Some(args.as_main_args()), Some(&mut app), std::ptr::null_mut());
    if ret >= 0 {
        return std::process::ExitCode::from(ret as u8);
    }

    let dir = cef_dir();
    let exe = std::env::current_exe().unwrap_or_default();
    let cache = std::env::temp_dir().join(format!("wine-webview2-helper-{}", std::process::id()));
    if matches!(mode, Mode::Ipc(_)) {
        let dir = cache.join("scripts");
        let _ = std::fs::create_dir_all(&dir);
        let _ = SCRIPTS_DIR.set(dir);
    }
    let settings = Settings {
        no_sandbox: true as _,
        windowless_rendering_enabled: true as _,
        browser_subprocess_path: exe.to_string_lossy().as_ref().into(),
        resources_dir_path: dir.to_string_lossy().as_ref().into(),
        locales_dir_path: dir.join("locales").to_string_lossy().as_ref().into(),
        root_cache_path: cache.to_string_lossy().as_ref().into(),
        log_severity: LogSeverity::WARNING,
        background_color: DEFAULT_BACKGROUND,
        ..Default::default()
    };
    if initialize(Some(args.as_main_args()), Some(&settings), Some(&mut app), std::ptr::null_mut()) != 1 {
        eprintln!("helper: cef initialize failed");
        return std::process::ExitCode::from(1);
    }
    run_message_loop();
    STATE.with(|s| s.borrow_mut().take());
    shutdown();
    let _ = std::fs::remove_dir_all(&cache);

    match mode {
        Mode::Shot(s) if !s.borrow().done => std::process::ExitCode::from(1),
        _ => std::process::ExitCode::SUCCESS,
    }
}
