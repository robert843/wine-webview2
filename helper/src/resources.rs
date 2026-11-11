//! WebResourceRequested: intercepting requests that match the app's filters.
//!
//! Flow (CEF IO thread):
//!   resource_request_handler - URL/context matches the view's filters -> our handler
//!   on_before_resource_load  - request held (CONTINUE_ASYNC), app handlers run on the UI thread
//!   (webresource.rs; after their decision or deferral Complete()) -> callback.cont()
//!   resource_handler         - the app's response, or None (request goes to the network, as in WebView2)

use crate::ipc::Channel;
use cef::rc::Rc as _;
use cef::*;
use std::collections::HashMap;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

/// Channel to the DLL usable from non-UI threads (sendmsg on SOCK_SEQPACKET is atomic).
static SENDER: OnceLock<Channel> = OnceLock::new();
/// Per-view filters: (pattern, COREWEBVIEW2_WEB_RESOURCE_CONTEXT).
static FILTERS: Mutex<Option<HashMap<u32, Vec<(String, u32)>>>> = Mutex::new(None);
static PENDING: Mutex<Option<HashMap<u64, Arc<Mutex<RequestState>>>>> = Mutex::new(None);
static NEXT_ID: AtomicU64 = AtomicU64::new(1);

const CONTEXT_ALL: u32 = 0;
/// net::ERR_ABORTED (maps to COREWEBVIEW2_WEB_ERROR_STATUS_OPERATION_CANCELED).
pub const ERR_ABORTED: i32 = -3;

/// Current main-frame navigation per view: (id, completed).
static NAVIGATIONS: Mutex<Option<HashMap<u32, Navigation>>> = Mutex::new(None);

/// Main-frame navigation of a view. `superseded`: the previous one, still unfinished when this one
/// started; CEF reports it later as ERR_ABORTED and it gets its own NavigationCompleted.
struct Navigation {
    id: u64,
    done: bool,
    url: String,
    superseded: Option<(u64, String)>,
}
static NEXT_NAVIGATION: AtomicU64 = AtomicU64::new(1);

pub fn send_event(ty: u32, view: u32, payload: &[u8]) {
    if let Some(sender) = SENDER.get() {
        if let Err(e) = sender.send(ty, view, payload, None) {
            eprintln!("helper: send failed: {e}");
        }
    }
}

pub fn begin_navigation(view: u32, url: &str) -> u64 {
    let id = NEXT_NAVIGATION.fetch_add(1, Ordering::Relaxed);
    let mut n = NAVIGATIONS.lock().unwrap();
    let map = n.get_or_insert_with(HashMap::new);
    let superseded = map.get(&view).filter(|old| !old.done).map(|old| (old.id, old.url.clone()));
    map.insert(view, Navigation { id, done: false, url: url.to_string(), superseded });
    id
}

pub fn active_navigation(view: u32) -> Option<u64> {
    NAVIGATIONS.lock().unwrap().as_ref()?.get(&view).filter(|n| !n.done).map(|n| n.id)
}

/// Marks the current navigation completed; None if it already was.
pub fn complete_navigation(view: u32) -> Option<u64> {
    let mut n = NAVIGATIONS.lock().unwrap();
    let entry = n.as_mut()?.get_mut(&view)?;
    if entry.done {
        return None;
    }
    entry.done = true;
    Some(entry.id)
}

/// The superseded navigation to `url`, if any (taken: it completes only once).
pub fn take_superseded(view: u32, url: &str) -> Option<u64> {
    let mut n = NAVIGATIONS.lock().unwrap();
    let entry = n.as_mut()?.get_mut(&view)?;
    match &entry.superseded {
        Some((id, u)) if u == url => {
            let id = *id;
            entry.superseded = None;
            Some(id)
        }
        _ => None,
    }
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

// ---------- Settings.UserAgent ----------
// Emulation.setUserAgentOverride (DevTools) only covers the main frame; cross-site frames (other
// renderer processes) would send the default UA, e.g. the FFXIV login then rejects the form.
// So the custom UA is also set on every request header of the view, as WebView2 does.
static USER_AGENTS: Mutex<Option<HashMap<u32, String>>> = Mutex::new(None);

pub fn set_user_agent(view: u32, ua: Option<String>) {
    let mut m = USER_AGENTS.lock().unwrap();
    let m = m.get_or_insert_with(HashMap::new);
    match ua {
        Some(ua) => m.insert(view, ua),
        None => m.remove(&view),
    };
}

fn user_agent(view: u32) -> Option<String> {
    USER_AGENTS.lock().unwrap().as_ref()?.get(&view).cloned()
}

pub fn remove_view(view: u32) {
    if let Some(u) = USER_AGENTS.lock().unwrap().as_mut() {
        u.remove(&view);
    }
    if let Some(f) = FILTERS.lock().unwrap().as_mut() {
        f.remove(&view);
    }
    if let Some(v) = VIRTUAL_HOSTS.lock().unwrap().as_mut() {
        v.remove(&view);
    }
}

// ---------- SetVirtualHostNameToFolderMapping ----------

/// View -> (lowercase host, unix folder).
static VIRTUAL_HOSTS: Mutex<Option<HashMap<u32, Vec<(String, std::path::PathBuf)>>>> = Mutex::new(None);

pub fn set_virtual_host(view: u32, host: &str, folder: std::path::PathBuf) {
    let host = host.to_ascii_lowercase();
    let mut v = VIRTUAL_HOSTS.lock().unwrap();
    let list = v.get_or_insert_with(HashMap::new).entry(view).or_default();
    list.retain(|(h, _)| *h != host);
    list.push((host, folder));
}

pub fn clear_virtual_host(view: u32, host: &str) {
    let host = host.to_ascii_lowercase();
    if let Some(list) = VIRTUAL_HOSTS.lock().unwrap().as_mut().and_then(|v| v.get_mut(&view)) {
        list.retain(|(h, _)| *h != host);
    }
}

fn percent_decode(s: &str) -> String {
    let b = s.as_bytes();
    let mut out = Vec::with_capacity(b.len());
    let mut i = 0;
    while i < b.len() {
        if b[i] == b'%' && i + 2 < b.len() {
            if let Ok(v) = u8::from_str_radix(&s[i + 1..i + 3], 16) {
                out.push(v);
                i += 3;
                continue;
            }
        }
        out.push(b[i]);
        i += 1;
    }
    String::from_utf8_lossy(&out).into_owned()
}

/// File for a URL under a mapped host (outer None = host not mapped, inner None = denied).
fn virtual_host_file(view: u32, url: &str) -> Option<Option<std::path::PathBuf>> {
    let rest = url.strip_prefix("https://").or_else(|| url.strip_prefix("http://"))?;
    let (host, path) = rest.split_once('/').unwrap_or((rest, ""));
    let host = host.split(':').next().unwrap_or(host).to_ascii_lowercase();
    let folder = VIRTUAL_HOSTS.lock().unwrap().as_ref()?.get(&view)?.iter().find(|(h, _)| *h == host)?.1.clone();
    let path = path.split(['?', '#']).next().unwrap_or("");
    let mut file = folder.clone();
    for part in percent_decode(path).split(['/', '\\']) {
        match part {
            "" | "." => {}
            ".." => return Some(None), // escapes the folder - deny
            p => file.push(p),
        }
    }
    if file.is_dir() {
        file.push("index.html");
    }
    Some(Some(file))
}

fn mime_type(path: &std::path::Path) -> &'static str {
    match path.extension().and_then(|e| e.to_str()).map(|e| e.to_ascii_lowercase()).as_deref() {
        Some("html" | "htm") => "text/html",
        Some("js" | "mjs") => "text/javascript",
        Some("css") => "text/css",
        Some("json" | "map") => "application/json",
        Some("svg") => "image/svg+xml",
        Some("png") => "image/png",
        Some("jpg" | "jpeg") => "image/jpeg",
        Some("gif") => "image/gif",
        Some("webp") => "image/webp",
        Some("ico") => "image/x-icon",
        Some("wasm") => "application/wasm",
        Some("woff") => "font/woff",
        Some("woff2") => "font/woff2",
        Some("ttf") => "font/ttf",
        Some("mp3") => "audio/mpeg",
        Some("wav") => "audio/wav",
        Some("mp4") => "video/mp4",
        Some("webm") => "video/webm",
        Some("pdf") => "application/pdf",
        Some("xml") => "text/xml",
        Some("txt") => "text/plain",
        _ => "application/octet-stream",
    }
}

fn file_response(file: Option<std::path::PathBuf>) -> AppResponse {
    match file.and_then(|f| std::fs::read(&f).ok().map(|d| (f, d))) {
        Some((f, content)) => AppResponse {
            status: 200,
            reason: "OK".into(),
            headers: vec![("Content-Type".into(), mime_type(&f).into()), ("Access-Control-Allow-Origin".into(), "*".into())],
            content,
        },
        None => AppResponse { status: 404, reason: "Not Found".into(), headers: Vec::new(), content: Vec::new() },
    }
}

/// WebView2 filter pattern: `*` matches any run, `?` a single character.
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
    response: Option<AppResponse>,     // app response, once decided
}

/// CEF callbacks are thread-safe (atomic refcount, cont() callable from any thread).
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
    // cef-rs passes the Vec's len() as the C array size, so it must be pre-filled.
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


// ---------- commands from the DLL (UI thread) ----------

pub fn finish(id: u64, response: Option<AppResponse>) {
    let Some(state) = PENDING.lock().unwrap().as_mut().and_then(|p| p.remove(&id)) else { return };
    let mut st = state.lock().unwrap();
    st.response = response;
    if let Some(cb) = st.callback.take() {
        cb.0.cont();
    }
}

// ---------- CEF handlers (IO thread) ----------

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
            let url = CefString::from(&request.url()).to_string();
            if crate::popup_hold_navigation(self.handler.view, &url) {
                return 1;
            }
            let id = begin_navigation(self.handler.view, &url);
            // Synchronous so the app's Cancel decision is made before the request goes out.
            let cancel = crate::webview::on_navigation_starting(self.handler.view, id, user_gesture != 0, is_redirect != 0, &url);
            if cancel && complete_navigation(self.handler.view).is_some() {
                // WebView2 reports a cancelled navigation as failed with OPERATION_CANCELED.
                crate::webview::on_navigation_completed(self.handler.view, id, false, 0, ERR_ABORTED);
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
            let user_agent = user_agent(self.handler.view);
            if !matches(self.handler.view, &url, web_resource_context(request)) {
                // Mapped virtual host: served from disk without involving the app.
                if let Some(file) = virtual_host_file(self.handler.view, &url) {
                    let state = RequestState { callback: None, response: Some(file_response(file)) };
                    return Some(ResourceRequestHandlerBuilder::new(InterceptedRequest {
                        view: self.handler.view,
                        state: Arc::new(Mutex::new(state)),
                        serve_only: true,
                        pass_through: false,
                        user_agent: None,
                    }));
                }
                // Custom UA only: set the header, the request goes to the network as usual.
                user_agent.as_ref()?;
                return Some(ResourceRequestHandlerBuilder::new(InterceptedRequest {
                    view: self.handler.view,
                    state: Arc::new(Mutex::new(RequestState::default())),
                    serve_only: false,
                    pass_through: true,
                    user_agent,
                }));
            }
            Some(ResourceRequestHandlerBuilder::new(InterceptedRequest {
                view: self.handler.view,
                state: Arc::new(Mutex::new(RequestState::default())),
                serve_only: false,
                pass_through: false,
                user_agent,
            }))
        }
    }
}

#[derive(Clone)]
struct InterceptedRequest {
    view: u32,
    state: Arc<Mutex<RequestState>>,
    /// Response already prepared (virtual host); the app is not asked.
    serve_only: bool,
    /// Only set the User-Agent header (Settings.UserAgent); the app is not asked.
    pass_through: bool,
    user_agent: Option<String>,
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
            if self.req.serve_only {
                return ReturnValue::CONTINUE;
            }
            let (Some(request), Some(callback)) = (request, callback) else {
                return ReturnValue::CONTINUE;
            };
            if let Some(ua) = &self.req.user_agent {
                request.set_header_by_name(Some(&"User-Agent".into()), Some(&ua.as_str().into()), 1);
            }
            if self.req.pass_through {
                return ReturnValue::CONTINUE;
            }
            let id = NEXT_ID.fetch_add(1, Ordering::Relaxed);
            let url = CefString::from(&request.url()).to_string();
            let method = CefString::from(&request.method()).to_string();
            let headers = headers_text(request);
            let body = request_body(request);
            self.req.state.lock().unwrap().callback = Some(SendCallback(callback.clone()));
            PENDING.lock().unwrap().get_or_insert_with(HashMap::new).insert(id, self.req.state.clone());

            // App handlers run on the UI thread (webresource.rs); the request waits.
            let (view, context) = (self.req.view, web_resource_context(request));
            crate::post_ui(move || crate::webresource::handle_request(view, id, context, url, method, headers, body));
            ReturnValue::CONTINUE_ASYNC
        }

        fn resource_handler(
            &self,
            _browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            request: Option<&mut Request>,
        ) -> Option<ResourceHandler> {
            let response = self.req.state.lock().unwrap().response.take().or_else(|| {
                // The app did not answer: a mapped virtual host still serves the file (as in WebView2).
                let url = CefString::from(&request?.url()).to_string();
                virtual_host_file(self.req.view, &url).map(file_response)
            })?;
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
