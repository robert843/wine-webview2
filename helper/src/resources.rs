//! WebResourceRequested: przechwytywanie żądań pasujących do filtrów aplikacji.
//!
//! Przebieg (wątek IO CEF):
//!   resource_request_handler — czy URL/kontekst pasuje do filtrów widoku → nasz handler
//!   on_before_resource_load  — żądanie wstrzymane (CONTINUE_ASYNC), handlery aplikacji na wątku UI
//!   (webresource.rs; po decyzji albo Complete() deferrala) → callback.cont()
//!   resource_handler         — odpowiedź aplikacji albo None (żądanie idzie do sieci, jak w WebView2)

use crate::ipc::Channel;
use cef::rc::Rc as _;
use cef::*;
use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

/// Kanał do DLL używany z wątków innych niż UI (sendmsg na SOCK_SEQPACKET jest atomowy).
static SENDER: OnceLock<Channel> = OnceLock::new();
/// Filtry per widok: (wzorzec, kontekst COREWEBVIEW2_WEB_RESOURCE_CONTEXT).
static FILTERS: Mutex<Option<HashMap<u32, Vec<(String, u32)>>>> = Mutex::new(None);
static PENDING: Mutex<Option<HashMap<u64, Arc<Mutex<RequestState>>>>> = Mutex::new(None);
static NEXT_ID: AtomicU64 = AtomicU64::new(1);

const CONTEXT_ALL: u32 = 0;

/// Bieżąca nawigacja ramki głównej per widok: (id, zakończona).
static NAVIGATIONS: Mutex<Option<HashMap<u32, (u64, bool)>>> = Mutex::new(None);
static NEXT_NAVIGATION: AtomicU64 = AtomicU64::new(1);

pub fn send_event(ty: u32, view: u32, payload: &[u8]) {
    if let Some(sender) = SENDER.get() {
        if let Err(e) = sender.send(ty, view, payload, None) {
            eprintln!("helper: send failed: {e}");
        }
    }
}

pub fn begin_navigation(view: u32) -> u64 {
    let id = NEXT_NAVIGATION.fetch_add(1, Ordering::Relaxed);
    NAVIGATIONS.lock().unwrap().get_or_insert_with(HashMap::new).insert(view, (id, false));
    id
}

pub fn current_navigation(view: u32) -> u64 {
    NAVIGATIONS.lock().unwrap().as_ref().and_then(|n| n.get(&view)).map(|n| n.0).unwrap_or(0)
}

/// Oznacza bieżącą nawigację jako zakończoną; None, jeśli już była zakończona.
pub fn complete_navigation(view: u32) -> Option<u64> {
    let mut n = NAVIGATIONS.lock().unwrap();
    let entry = n.as_mut()?.get_mut(&view)?;
    if entry.1 {
        return None;
    }
    entry.1 = true;
    Some(entry.0)
}

pub fn set_sender(chan: Channel) {
    let _ = SENDER.set(chan);
}

pub fn add_filter(view: u32, pattern: String, context: u32) {
    let mut f = FILTERS.lock().unwrap();
    f.get_or_insert_with(HashMap::new).entry(view).or_default().push((pattern, context));
}

pub fn remove_filter(view: u32, pattern: &str, context: u32) {
    if let Some(list) = FILTERS.lock().unwrap().as_mut().and_then(|f| f.get_mut(&view)) {
        if let Some(i) = list.iter().position(|(p, c)| p == pattern && *c == context) {
            list.remove(i);
        }
    }
}

pub fn remove_view(view: u32) {
    if let Some(f) = FILTERS.lock().unwrap().as_mut() {
        f.remove(&view);
    }
}

/// Dopasowanie wzorca WebView2: `*` = dowolny ciąg, `?` = jeden znak.
fn wildcard_match(pattern: &[char], text: &[char]) -> bool {
    let (mut p, mut t, mut star, mut mark) = (0, 0, None, 0);
    while t < text.len() {
        if p < pattern.len() && (pattern[p] == '?' || pattern[p] == text[t]) {
            p += 1;
            t += 1;
        } else if p < pattern.len() && pattern[p] == '*' {
            star = Some(p);
            mark = t;
            p += 1;
        } else if let Some(s) = star {
            p = s + 1;
            mark += 1;
            t = mark;
        } else {
            return false;
        }
    }
    while p < pattern.len() && pattern[p] == '*' {
        p += 1;
    }
    p == pattern.len()
}

fn matches(view: u32, url: &str, context: u32) -> bool {
    let f = FILTERS.lock().unwrap();
    let Some(list) = f.as_ref().and_then(|f| f.get(&view)) else { return false };
    let text: Vec<char> = url.chars().collect();
    list.iter().any(|(pattern, ctx)| {
        (*ctx == CONTEXT_ALL || *ctx == context) && wildcard_match(&pattern.chars().collect::<Vec<_>>(), &text)
    })
}

/// cef_resource_type_t -> COREWEBVIEW2_WEB_RESOURCE_CONTEXT
fn web_resource_context(request: &Request) -> u32 {
    match *request.resource_type().as_ref() as u32 {
        0 | 1 => 1,       // main frame, sub frame -> DOCUMENT
        2 => 2,           // STYLESHEET
        3 => 6,           // SCRIPT
        4 | 12 => 3,      // IMAGE, FAVICON
        5 => 5,           // FONT
        8 => 4,           // MEDIA
        13 => 7,          // XHR (fetch is XHR in CEF)
        14 => 14,         // PING
        16 => 15,         // CSP report
        _ => 16,          // OTHER
    }
}

#[derive(Default)]
pub struct AppResponse {
    pub status: i32,
    pub reason: String,
    pub headers: Vec<(String, String)>,
    pub content: Vec<u8>,
}

#[derive(Default)]
struct RequestState {
    callback: Option<SendCallback>,
    response: Option<AppResponse>,     // gotowa odpowiedź aplikacji
}

/// Callback CEF jest bezpieczny wątkowo (refcount atomowy, cont() z dowolnego wątku).
struct SendCallback(Callback);
unsafe impl Send for SendCallback {}

fn headers_text(request: &Request) -> String {
    let mut map = CefStringMultimap::new();
    request.header_map(Some(&mut map));
    let mut out = String::new();
    for (name, values) in map {
        for v in values {
            out.push_str(&format!("{name}: {v}\r\n"));
        }
    }
    out
}

fn request_body(request: &Request) -> Vec<u8> {
    let mut body = Vec::new();
    let debug = std::env::var_os("WINE_WEBVIEW2_DEBUG_BODY").is_some();
    let Some(post) = request.post_data() else {
        if debug {
            eprintln!("helper: {} {}: no post data", CefString::from(&request.method()), CefString::from(&request.url()));
        }
        return body;
    };
    // cef-rs przekazuje len() wektora jako rozmiar tablicy C — trzeba go wcześniej wypełnić.
    let mut elements: Vec<Option<PostDataElement>> = (0..post.element_count()).map(|_| None).collect();
    post.elements(Some(&mut elements));
    if debug {
        let types: Vec<String> = elements
            .iter()
            .flatten()
            .map(|e| format!("{:?}/{}", e.get_type(), e.bytes_count()))
            .collect();
        eprintln!(
            "helper: {}: post data count {}, has_excluded {}, elements {types:?}",
            CefString::from(&request.url()),
            post.element_count(),
            post.has_excluded_elements()
        );
    }
    for e in elements.into_iter().flatten() {
        let n = e.bytes_count();
        let start = body.len();
        body.resize(start + n, 0);
        let got = e.bytes(n, body[start..].as_mut_ptr());
        body.truncate(start + got);
    }
    body
}


// ---------- polecenia z DLL (wątek UI) ----------

pub fn finish(id: u64, response: Option<AppResponse>) {
    let Some(state) = PENDING.lock().unwrap().as_mut().and_then(|p| p.remove(&id)) else { return };
    let mut st = state.lock().unwrap();
    st.response = response;
    if let Some(cb) = st.callback.take() {
        cb.0.cont();
    }
}

// ---------- handlery CEF (wątek IO) ----------

#[derive(Clone)]
pub struct ViewRequests {
    pub view: u32,
}

wrap_request_handler! {
    pub struct RequestHandlerBuilder {
        handler: ViewRequests,
    }

    impl RequestHandler {
        fn on_before_browse(
            &self,
            _browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            request: Option<&mut Request>,
            user_gesture: ::std::os::raw::c_int,
            is_redirect: ::std::os::raw::c_int,
        ) -> ::std::os::raw::c_int {
            let (Some(frame), Some(request)) = (frame, request) else { return 0 };
            if frame.is_main() == 0 {
                return 0;
            }
            let id = begin_navigation(self.handler.view);
            let url = CefString::from(&request.url()).to_string();
            // Synchronicznie: decyzja aplikacji (Cancel) zapada, zanim żądanie wyjdzie.
            let cancel = crate::webview::on_navigation_starting(self.handler.view, id, user_gesture != 0, is_redirect != 0, &url);
            if cancel {
                complete_navigation(self.handler.view);
            }
            cancel as i32
        }

        fn resource_request_handler(
            &self,
            _browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            request: Option<&mut Request>,
            _is_navigation: ::std::os::raw::c_int,
            _is_download: ::std::os::raw::c_int,
            _request_initiator: Option<&CefString>,
            _disable_default_handling: Option<&mut ::std::os::raw::c_int>,
        ) -> Option<ResourceRequestHandler> {
            let request = request?;
            let url = CefString::from(&request.url()).to_string();
            if !matches(self.handler.view, &url, web_resource_context(request)) {
                return None;
            }
            Some(ResourceRequestHandlerBuilder::new(InterceptedRequest {
                view: self.handler.view,
                state: Arc::new(Mutex::new(RequestState::default())),
            }))
        }
    }
}

#[derive(Clone)]
struct InterceptedRequest {
    view: u32,
    state: Arc<Mutex<RequestState>>,
}

wrap_resource_request_handler! {
    struct ResourceRequestHandlerBuilder {
        req: InterceptedRequest,
    }

    impl ResourceRequestHandler {
        fn on_before_resource_load(
            &self,
            _browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            request: Option<&mut Request>,
            callback: Option<&mut Callback>,
        ) -> ReturnValue {
            let (Some(request), Some(callback)) = (request, callback) else {
                return ReturnValue::CONTINUE;
            };
            let id = NEXT_ID.fetch_add(1, Ordering::Relaxed);
            let url = CefString::from(&request.url()).to_string();
            let method = CefString::from(&request.method()).to_string();
            let headers = headers_text(request);
            let body = request_body(request);
            self.req.state.lock().unwrap().callback = Some(SendCallback(callback.clone()));
            PENDING.lock().unwrap().get_or_insert_with(HashMap::new).insert(id, self.req.state.clone());

            // Handlery aplikacji wołamy na wątku UI (webresource.rs); żądanie czeka.
            let (view, context) = (self.req.view, web_resource_context(request));
            crate::post_ui(move || crate::webresource::handle_request(view, id, context, url, method, headers, body));
            ReturnValue::CONTINUE_ASYNC
        }

        fn resource_handler(
            &self,
            _browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            _request: Option<&mut Request>,
        ) -> Option<ResourceHandler> {
            let response = self.req.state.lock().unwrap().response.take()?;
            Some(ResourceHandlerBuilder::new(ServedResponse {
                response: Arc::new(response),
                offset: Arc::new(Mutex::new(0)),
            }))
        }
    }
}

#[derive(Clone)]
struct ServedResponse {
    response: Arc<AppResponse>,
    offset: Arc<Mutex<usize>>,
}

wrap_resource_handler! {
    struct ResourceHandlerBuilder {
        served: ServedResponse,
    }

    impl ResourceHandler {
        fn open(
            &self,
            _request: Option<&mut Request>,
            handle_request: Option<&mut ::std::os::raw::c_int>,
            _callback: Option<&mut Callback>,
        ) -> ::std::os::raw::c_int {
            if let Some(h) = handle_request {
                *h = 1;
            }
            1
        }

        fn response_headers(
            &self,
            response: Option<&mut Response>,
            response_length: Option<&mut i64>,
            _redirect_url: Option<&mut CefString>,
        ) {
            let r = &self.served.response;
            if let Some(resp) = response {
                resp.set_status(r.status);
                resp.set_status_text(Some(&r.reason.as_str().into()));
                for (name, value) in &r.headers {
                    if name.eq_ignore_ascii_case("content-type") {
                        let (mime, rest) = value.split_once(';').unwrap_or((value.as_str(), ""));
                        resp.set_mime_type(Some(&mime.trim().into()));
                        if let Some(cs) = rest.split(';').find_map(|p| p.trim().strip_prefix("charset=")) {
                            resp.set_charset(Some(&cs.trim_matches('"').into()));
                        }
                    }
                    resp.set_header_by_name(Some(&name.as_str().into()), Some(&value.as_str().into()), 0);
                }
            }
            if let Some(len) = response_length {
                *len = r.content.len() as i64;
            }
        }

        fn read(
            &self,
            data_out: *mut u8,
            bytes_to_read: ::std::os::raw::c_int,
            bytes_read: Option<&mut ::std::os::raw::c_int>,
            _callback: Option<&mut ResourceReadCallback>,
        ) -> ::std::os::raw::c_int {
            let content = &self.served.response.content;
            let mut offset = self.served.offset.lock().unwrap();
            let n = (content.len() - *offset).min(bytes_to_read.max(0) as usize);
            if n > 0 {
                unsafe { std::ptr::copy_nonoverlapping(content[*offset..].as_ptr(), data_out, n) };
                *offset += n;
            }
            if let Some(b) = bytes_read {
                *b = n as i32;
            }
            (n > 0) as i32
        }
    }
}
