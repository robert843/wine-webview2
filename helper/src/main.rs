//! wine-webview2-helper - native (Linux) browser process for wine-webview2.
//!
//! Modes:
//!   --ipc-fd=N                     normal operation: commands from EmbeddedBrowserWebView.dll
//!                                  over socket N (protocol: protocol.rs / protocol.h)
//!   --screenshot <url> <file.ppm>  test: renders a page off-screen and saves one frame
//!
//! The same executable also serves as the CEF subprocess (renderer, GPU, ...): CEF starts it
//! with `--type=...` and `execute_process` handles that case and exits.

// Logging must never take the helper down: std's eprintln! panics when stderr is a closed pipe
// (e.g. the app's log reader is gone), which would skip shutdown and cleanup. Defined before the
// modules so it shadows the std macro everywhere.
macro_rules! eprintln {
    ($($arg:tt)*) => {{
        use std::io::Write as _;
        let _ = writeln!(std::io::stderr(), $($arg)*);
    }};
}

mod ipc;
mod protocol;
mod api_gen;
mod renderer;
mod resources;
mod rpc;
mod settings;
mod webresource;
mod webview;

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
use std::sync::atomic::{AtomicBool, AtomicU32, Ordering};
use std::sync::{Arc, Mutex, OnceLock};

/// Directory with document-created script files (passed to renderers via a switch).
static SCRIPTS_DIR: OnceLock<PathBuf> = OnceLock::new();

// ======================================================================
// Common: tasks on the CEF UI thread
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

pub fn post_ui(f: impl FnOnce() + Send + 'static) {
    let mut task = TaskBuilder::new(ClosureTask { f: Arc::new(Mutex::new(Some(Box::new(f)))) });
    post_task(ThreadId::UI, Some(&mut task));
}

// ======================================================================
// IPC mode
// ======================================================================

/// ARGB, matching the default COREWEBVIEW2_COLOR (opaque white).
const DEFAULT_BACKGROUND: u32 = 0xFFFF_FFFF;

/// Default User-Agent as in WebView2 on Windows: apps and sites check the platform and must
/// see Windows under Wine. Version = Chromium version in CEF.
pub const DEFAULT_USER_AGENT: &str =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/156.0.0.0 Safari/537.36 Edg/156.0.0.0";

static NEXT_DEVTOOLS_ID: std::sync::atomic::AtomicI32 = std::sync::atomic::AtomicI32::new(1);

pub fn next_devtools_id() -> i32 {
    NEXT_DEVTOOLS_ID.fetch_add(1, std::sync::atomic::Ordering::Relaxed)
}

fn devtools(browser: &Browser, method: &str, params: &str) -> Option<i32> {
    devtools_with_id(browser, next_devtools_id(), method, params)
}

/// CEF may reply synchronously (inside send_dev_tools_message), so the waiter must be
/// registered before sending - hence the caller-supplied id.
fn devtools_with_id(browser: &Browser, id: i32, method: &str, params: &str) -> Option<i32> {
    let msg = format!("{{\"id\":{id},\"method\":{},\"params\":{params}}}", renderer::json_string(method));
    let host = browser.host()?;
    if host.send_dev_tools_message(Some(msg.as_bytes())) == 0 {
        eprintln!("helper: devtools {method} failed");
        return None;
    }
    Some(id)
}

/// CallDevToolsProtocolMethod with a preassigned id (result -> webview::on_devtools_result).
pub fn devtools_call(view: u32, id: i32, method: &str, params: &str) -> bool {
    let params = if params.trim().is_empty() { "{}" } else { params };
    view_browser(view).and_then(|b| devtools_with_id(&b, id, method, params)).is_some()
}

#[derive(Clone)]
struct DevToolsObserver {
    view: u32,
}

wrap_dev_tools_message_observer! {
    struct DevToolsObserverBuilder {
        observer: DevToolsObserver,
    }

    impl DevToolsMessageObserver {
        fn on_dev_tools_method_result(
            &self,
            _browser: Option<&mut Browser>,
            message_id: ::std::os::raw::c_int,
            success: ::std::os::raw::c_int,
            result: Option<&[u8]>,
        ) {
            let json = String::from_utf8_lossy(result.unwrap_or(b"{}")).into_owned();
            if std::env::var_os("WINE_WEBVIEW2_DEBUG_RPC").is_some() {
                eprintln!("helper: devtools result {message_id} success {success}: {json:.120}");
            }
            webview::on_devtools_result(self.observer.view, message_id, success != 0, &json);
        }

        fn on_dev_tools_event(&self, _browser: Option<&mut Browser>, method: Option<&CefString>, params: Option<&[u8]>) {
            let method = method.map(|m| m.to_string()).unwrap_or_default();
            let json = String::from_utf8_lossy(params.unwrap_or(b"{}")).into_owned();
            webview::on_devtools_event(self.observer.view, &method, &json);
        }
    }
}

fn set_user_agent(browser: &Browser, ua: &str) {
    let ua = if ua.is_empty() { DEFAULT_USER_AGENT } else { ua };
    let params = format!("{{\"userAgent\":{},\"platform\":\"Win32\"}}", renderer::json_string(ua));
    devtools(browser, "Emulation.setUserAgentOverride", &params);
}



struct View {
    browser: Option<Browser>,
    devtools_observer: Option<Registration>,
    doc_scripts: Vec<String>,
    settings: u32,
    last_source: String,
    /// Navigation completion held until OnLoadingStateChange(false): (http_status, net_error).
    pending_completion: Option<(i32, i32)>,
    width: u32,
    height: u32,
    frames: Option<FrameBuffer>,
    dropped: bool,
    user_agent: String,
    /// Widget popup (<select>, autofill) in OSR: CEF paints it separately (PaintElementType::POPUP),
    /// so we composite it onto the last view frame.
    widget: Option<WidgetPopup>,
    /// Popup waiting to be adopted by an app WebView (NewWindow) or for a decision.
    popup: Option<PendingPopup>,
}

impl View {
    fn new(width: u32, height: u32) -> Self {
        View {
            browser: None,
            devtools_observer: None,
            doc_scripts: Vec::new(),
            settings: SETTINGS_DEFAULT,
            last_source: String::new(),
            pending_completion: None,
            width: width.max(1),
            height: height.max(1),
            frames: None,
            dropped: false,
            user_agent: String::new(),
            widget: None,
            popup: None,
        }
    }
}

#[derive(Default)]
struct WidgetPopup {
    rect: Option<Rect>,
    pixels: Vec<u8>,
    size: (i32, i32),
    /// Copy of the last view frame (BGRA) and its size.
    view: Vec<u8>,
    view_size: (i32, i32),
}

/// Composites popup pixels onto a copy of the view frame (clipped to the view).
fn compose_widget(w: &WidgetPopup) -> Option<Vec<u8>> {
    let r = w.rect.as_ref()?;
    let (vw, vh) = w.view_size;
    let (pw, ph) = w.size;
    if w.view.len() != (vw * vh * 4) as usize || w.pixels.len() != (pw * ph * 4) as usize {
        return None;
    }
    let mut out = w.view.clone();
    for y in 0..ph.min(r.height) {
        let dy = r.y + y;
        if dy < 0 || dy >= vh {
            continue;
        }
        let x0 = r.x.max(0);
        let x1 = (r.x + pw.min(r.width)).min(vw);
        if x1 <= x0 {
            continue;
        }
        let src = ((y * pw + (x0 - r.x)) * 4) as usize;
        let dst = ((dy * vw + x0) * 4) as usize;
        let n = ((x1 - x0) * 4) as usize;
        out[dst..dst + n].copy_from_slice(&w.pixels[src..src + n]);
    }
    Some(out)
}

struct IpcState {
    chan: Channel,
    views: HashMap<u32, View>,
    next_generation: u64,
}

thread_local! {
    /// IPC mode state - accessed only on the CEF UI thread.
    static STATE: RefCell<Option<IpcState>> = const { RefCell::new(None) };
}

fn with_state<R>(f: impl FnOnce(&mut IpcState) -> R) -> Option<R> {
    STATE.try_with(|s| s.try_borrow_mut().ok().and_then(|mut s| s.as_mut().map(f))).ok().flatten()
}

fn send_event(ty: u32, view: u32, payload: &[u8]) {
    with_state(|st| {
        if let Err(e) = st.chan.send(ty, view, payload, None) {
            eprintln!("helper: send failed: {e}");
        }
    });
}

pub fn base64(data: &[u8]) -> String {
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

/// CEF client of a view. The view id is mutable: a popup created in OnBeforePopup is later
/// adopted by an app WebView (NewWindow) without recreating the browser.
#[derive(Clone)]
struct ViewClient {
    id: Arc<AtomicU32>,
}

impl ViewClient {
    fn new(view: u32) -> Self {
        ViewClient { id: Arc::new(AtomicU32::new(view)) }
    }
    fn view(&self) -> u32 {
        self.id.load(Ordering::Relaxed)
    }
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

        fn request_handler(&self) -> Option<RequestHandler> {
            Some(resources::RequestHandlerBuilder::new(resources::ViewRequests { view: self.client.view() }))
        }

        fn life_span_handler(&self) -> Option<LifeSpanHandler> {
            Some(ViewLifeSpanHandlerBuilder::new(self.client.clone()))
        }

        fn jsdialog_handler(&self) -> Option<JsdialogHandler> {
            Some(ViewDialogHandlerBuilder::new(self.client.clone()))
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
                    // Like WebView2: messages from iframes belong to ICoreWebView2Frame.WebMessageReceived
                    // (not implemented yet), never to the top-level WebMessageReceived.
                    if frame.as_ref().is_none_or(|f| f.is_main() == 0) {
                        return 1;
                    }
                    let json = CefString::from(&args.string(0)).to_string();
                    let source = frame.map(|f| CefString::from(&f.url()).to_string()).unwrap_or_default();
                    webview::on_web_message(self.client.view(), &source, &json);
                    1
                }
                renderer::MSG_RESULT => {
                    let arg = |i| CefString::from(&args.string(i)).to_string();
                    let id = arg(0).parse::<u64>().unwrap_or(0);
                    let result = if arg(2) == "1" {
                        Ok(arg(1))
                    } else {
                        Err(webview::ScriptError {
                            name: arg(3),
                            message: arg(4),
                            line: arg(5).parse().unwrap_or(0),
                            column: arg(6).parse().unwrap_or(0),
                            json: arg(7),
                        })
                    };
                    webview::on_script_result(self.client.view(), id, result);
                    1
                }
                renderer::MSG_CLOSE => {
                    webview::on_window_close(self.client.view());
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
            let (w, h) = with_state(|st| st.views.get(&self.client.view()).map(|v| (v.width, v.height)))
                .flatten()
                .unwrap_or((1, 1));
            rect.x = 0;
            rect.y = 0;
            rect.width = w.max(1) as i32;
            rect.height = h.max(1) as i32;
        }

        fn on_popup_show(&self, browser: Option<&mut Browser>, show: ::std::os::raw::c_int) {
            let view = self.client.view();
            with_state(|st| {
                if let Some(v) = st.views.get_mut(&view) {
                    v.widget = if show != 0 { Some(WidgetPopup::default()) } else { None };
                }
            });
            // Fresh view frame: to composite under the popup (show) or without it (hide).
            if let Some(host) = browser.and_then(|b| b.host()) {
                host.invalidate(PaintElementType::VIEW);
            }
        }

        fn on_popup_size(&self, _browser: Option<&mut Browser>, rect: Option<&Rect>) {
            let view = self.client.view();
            with_state(|st| {
                if let Some(w) = st.views.get_mut(&view).and_then(|v| v.widget.as_mut()) {
                    w.rect = rect.cloned();
                }
            });
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
            if buffer.is_null() || width <= 0 || height <= 0 {
                return;
            }
            let src = unsafe { std::slice::from_raw_parts(buffer, (width * height * 4) as usize) };
            let view = self.client.view();
            let composed = with_state(|st| {
                let w = st.views.get_mut(&view)?.widget.as_mut()?;
                if type_ == PaintElementType::POPUP {
                    w.pixels.clear();
                    w.pixels.extend_from_slice(src);
                    w.size = (width, height);
                } else {
                    w.view.clear();
                    w.view.extend_from_slice(src);
                    w.view_size = (width, height);
                }
                compose_widget(w).map(|buf| (buf, w.view_size))
            })
            .flatten();
            if let Some((buf, (vw, vh))) = composed {
                with_state(|st| paint_view(st, view, &buf, vw as u32, vh as u32, (0, 0, vw, vh)));
                return;
            }
            if type_ != PaintElementType::VIEW {
                return;
            }
            let dirty = dirty_rects.unwrap_or(&[]).iter().fold(None::<(i32, i32, i32, i32)>, |acc, r| {
                let (l, t, rr, b) = (r.x, r.y, r.x + r.width, r.y + r.height);
                Some(match acc {
                    None => (l, t, rr, b),
                    Some((al, at, ar, ab)) => (al.min(l), at.min(t), ar.max(rr), ab.max(b)),
                })
            }).unwrap_or((0, 0, width, height));
            let invalidate = with_state(|st| paint_view(st, view, src, width as u32, height as u32, dirty));
            if invalidate == Some(true) {
                if let Some(host) = browser.and_then(|b| b.host()) {
                    host.invalidate(PaintElementType::VIEW);
                }
            }
        }
    }
}

/// Copies the frame into a free memfd slot and sends FRAME_READY. Returns true if a new frame
/// must be requested immediately (should not happen).
fn paint_view(st: &mut IpcState, view_id: u32, src: &[u8], width: u32, height: u32, dirty: (i32, i32, i32, i32)) -> bool {
    let Some(view) = st.views.get_mut(&view_id).filter(|v| v.popup.is_none()) else { return false };

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
        // DLL holds both slots - drop the frame and repaint after FRAME_DONE.
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
        fn on_console_message(
            &self,
            _browser: Option<&mut Browser>,
            _level: LogSeverity,
            message: Option<&CefString>,
            source: Option<&CefString>,
            line: ::std::os::raw::c_int,
        ) -> ::std::os::raw::c_int {
            // JS console to stderr so it shows up in the Wine log.
            let message = message.map(|m| m.to_string()).unwrap_or_default();
            let source = source.map(|s| s.to_string()).unwrap_or_default();
            eprintln!("helper: console[view {}] {source}:{line}: {message}", self.client.view());
            0
        }

        fn on_address_change(
            &self,
            _browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            url: Option<&CefString>,
        ) {
            if frame.map(|f| f.is_main() != 0) != Some(true) {
                return;
            }
            // New documents are reported by on_load_start; this catches same-document
            // navigations (pushState, anchors), detected by a URL change.
            let url = url.map(|u| u.to_string()).unwrap_or_default();
            if set_last_source(self.client.view(), &url) {
                webview::on_source_changed(self.client.view(), &url);
            }
        }

        fn on_title_change(&self, _browser: Option<&mut Browser>, title: Option<&CefString>) {
            let title = title.map(|t| t.to_string()).unwrap_or_default();
            webview::on_title_changed(self.client.view(), &title);
        }

        fn on_cursor_change(
            &self,
            _browser: Option<&mut Browser>,
            _cursor: ::std::os::raw::c_ulong,
            type_: CursorType,
            _custom_cursor_info: Option<&CursorInfo>,
        ) -> ::std::os::raw::c_int {
            let kind = *type_.as_ref() as u32;
            send_event(EVT_CURSOR, self.client.view(), &Writer::default().u32(kind).u32(0).0);
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
            can_go_back: ::std::os::raw::c_int,
            can_go_forward: ::std::os::raw::c_int,
        ) {
            let view = self.client.view();
            send_event(EVT_LOAD_STATE, view, &Writer::default().u32((is_loading != 0) as u32).u32(0).0);
            if is_loading == 0 {
                // Only now is the history state reliable; then the held NavigationCompleted.
                send_history(view, can_go_back != 0, can_go_forward != 0);
                if let Some((status, error)) = with_state(|st| st.views.get_mut(&view)?.pending_completion.take()).flatten() {
                    send_navigation_completed(view, status, error);
                }
            }
        }

        fn on_load_start(
            &self,
            browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            _transition_type: TransitionType,
        ) {
            let Some(frame) = frame.filter(|f| f.is_main() != 0) else { return };
            // WebView2 order: SourceChanged -> ContentLoading -> HistoryChanged.
            let view = self.client.view();
            let url = CefString::from(&frame.url()).to_string();
            // about:blank (and similar) skip OnBeforeBrowse: raise NavigationStarting here
            // (not cancellable - the document is already loading).
            let id = resources::active_navigation(view).unwrap_or_else(|| {
                let id = resources::begin_navigation(view, &url);
                webview::on_navigation_starting(view, id, false, false, &url);
                id
            });
            set_last_source(view, &url);
            webview::on_source_changed(view, &url);
            webview::on_content_loading(view, id);
            let _ = browser;
        }

        fn on_load_end(
            &self,
            browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            http_status_code: ::std::os::raw::c_int,
        ) {
            if frame.map(|f| f.is_main() != 0) != Some(true) {
                return;
            }
            finish_navigation(self.client.view(), browser, http_status_code, 0);
        }

        fn on_load_error(
            &self,
            browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            error_code: Errorcode,
            _error_text: Option<&CefString>,
            failed_url: Option<&CefString>,
        ) {
            if frame.map(|f| f.is_main() != 0) != Some(true) {
                return;
            }
            let (view, error) = (self.client.view(), *error_code.as_ref() as i32);
            // A navigation replaced by a newer one: complete it, not the current one.
            if error == resources::ERR_ABORTED {
                let url = failed_url.map(|u| u.to_string()).unwrap_or_default();
                if let Some(id) = resources::take_superseded(view, &url) {
                    webview::on_navigation_completed(view, id, false, 0, error);
                    return;
                }
            }
            finish_navigation(view, browser, 0, error);
        }
    }
}

/// Writes the browser's document-created scripts to the file read by the renderer (atomically).
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

fn send_history(view: u32, back: bool, forward: bool) {
    webview::on_history_changed(view, back, forward);
}

/// Main frame finished loading: if the browser is still "loading", wait for
/// OnLoadingStateChange(false) (reliable history state), otherwise report now.
fn finish_navigation(view: u32, browser: Option<&mut Browser>, http_status: i32, net_error: i32) {
    let loading = browser.as_ref().map(|b| b.is_loading() != 0).unwrap_or(false);
    if loading {
        with_state(|st| {
            if let Some(v) = st.views.get_mut(&view) {
                v.pending_completion = Some((http_status, net_error));
            }
        });
    } else {
        if let Some(b) = browser {
            send_history(view, b.can_go_back() != 0, b.can_go_forward() != 0);
        }
        send_navigation_completed(view, http_status, net_error);
    }
}

/// Stores the main frame URL; returns true if it changed.
fn set_last_source(view: u32, url: &str) -> bool {
    with_state(|st| {
        let v = st.views.get_mut(&view)?;
        if v.last_source == url {
            return Some(false);
        }
        v.last_source = url.to_string();
        Some(true)
    })
    .flatten()
    .unwrap_or(false)
}

fn send_navigation_completed(view: u32, http_status: i32, net_error: i32) {
    let Some(id) = resources::complete_navigation(view) else { return };
    let success = net_error == 0 && (http_status == 0 || http_status < 400);
    webview::on_navigation_completed(view, id, success, http_status.max(0), net_error);
}

/// View settings flags (from the Settings object): file for the renderer, state for DevTools.
pub fn apply_settings(view: u32, flags: u32) {
    with_state(|st| {
        if let Some(v) = st.views.get_mut(&view) {
            v.settings = flags;
        }
    });
    if let (Some(b), Some(dir)) = (view_browser(view), SCRIPTS_DIR.get()) {
        let path = renderer::settings_file(&dir.to_string_lossy(), b.identifier());
        if let Err(e) = std::fs::write(&path, flags.to_le_bytes()) {
            eprintln!("helper: cannot write {}: {e}", path.display());
        }
    }
}

pub fn apply_user_agent(view: u32, ua: &str) {
    resources::set_user_agent(view, (!ua.is_empty()).then(|| ua.to_string()));
    with_state(|st| {
        if let Some(v) = st.views.get_mut(&view) {
            v.user_agent = ua.to_string();
        }
    });
    if let Some(b) = view_browser(view) {
        set_user_agent(&b, ua);
    }
}

/// Root object (id 0) of generic calls.
fn root_call(ctx: &mut rpc::Ctx, method: u32, rd: &mut Reader, wr: &mut Writer) -> i32 {
    match method {
        rpc::ROOT_CREATE_ENVIRONMENT => {
            let user_data_dir = rd.str_opt().unwrap_or_default();
            let _options = ctx.read_objref(rd);
            let env = webview::Environment { user_data_dir };
            ctx.write_objref(wr, &rpc::ObjRef::Runtime(std::rc::Rc::new(api_gen::ICoreWebView2Environment15Obj(env))));
            rpc::S_OK
        }
        rpc::ROOT_CREATE_CONTROLLER => {
            let _env = ctx.read_objref(rd);
            let (view, width, height) = (rd.u32().unwrap_or(0), rd.u32().unwrap_or(1), rd.u32().unwrap_or(1));
            match webview::create_controller(view, width, height) {
                Ok(controller) => {
                    ctx.write_objref(wr, &controller);
                    rpc::S_OK
                }
                Err(e) => e,
            }
        }
        _ => {
            eprintln!("helper: unknown root method {method:#x}");
            rpc::E_NOTIMPL
        }
    }
}

fn view_browser(view: u32) -> Option<Browser> {
    with_state(|st| st.views.get(&view).and_then(|v| v.browser.clone())).flatten()
}

static SHUTTING_DOWN: AtomicBool = AtomicBool::new(false);
/// Per-process CEF profile and cache, removed on exit (also by the shutdown watchdog).
static CACHE_DIR: OnceLock<PathBuf> = OnceLock::new();

/// Cache directories of helpers that were killed (SIGTERM to the whole process group, e.g. Steam's
/// "Stop" or a timeout) and could not clean up. A live helper holds an flock on its directory's
/// lock file; PIDs alone are not enough (Steam containers have their own PID namespace).
fn remove_stale_cache_dirs() {
    let Ok(entries) = std::fs::read_dir(std::env::temp_dir()) else { return };
    for entry in entries.flatten() {
        let name = entry.file_name();
        if !name.to_str().is_some_and(|n| n.starts_with("wine-webview2-helper-")) {
            continue;
        }
        // Skip fresh ones: a starting helper creates its lock file just before locking it.
        let old = entry.metadata().and_then(|m| m.modified()).ok()
            .and_then(|t| t.elapsed().ok())
            .is_some_and(|age| age > std::time::Duration::from_secs(60));
        if !old {
            continue;
        }
        let Ok(lock) = std::fs::File::open(entry.path().join("lock")) else { continue };
        if unsafe { libc::flock(lock.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } == 0 {
            let _ = std::fs::remove_dir_all(entry.path());
        }
    }
}

/// Creates the cache directory and keeps it locked for the lifetime of the process.
fn lock_cache_dir(dir: &std::path::Path) {
    static LOCK: OnceLock<std::fs::File> = OnceLock::new();
    let _ = std::fs::create_dir_all(dir);
    if let Ok(f) = std::fs::File::create(dir.join("lock")) {
        if unsafe { libc::flock(f.as_raw_fd(), libc::LOCK_EX | libc::LOCK_NB) } == 0 {
            let _ = LOCK.set(f);
        }
    }
}

fn remove_cache_dir() {
    if let Some(dir) = CACHE_DIR.get() {
        let _ = std::fs::remove_dir_all(dir);
    }
}

thread_local! {
    /// Every live browser (views, pending and native popups), from OnAfterCreated to OnBeforeClose.
    /// At shutdown all are closed and the loop quits when this is empty: CefShutdown hangs while a
    /// browser is still open.
    static LIVE_BROWSERS: RefCell<HashMap<i32, Browser>> = RefCell::new(HashMap::new());
}

fn browser_created(browser: &Browser) {
    LIVE_BROWSERS.with(|b| b.borrow_mut().insert(browser.identifier(), browser.clone()));
    if SHUTTING_DOWN.load(Ordering::SeqCst) {
        if let Some(host) = browser.host() {
            host.close_browser(1);
        }
    }
}

fn browser_closed(browser: Option<&mut Browser>) {
    let Some(browser) = browser else { return };
    let empty = LIVE_BROWSERS.with(|b| {
        let mut b = b.borrow_mut();
        b.remove(&browser.identifier());
        b.is_empty()
    });
    if empty && SHUTTING_DOWN.load(Ordering::SeqCst) {
        quit_message_loop();
    }
}

/// Connection to the DLL lost (app exited, possibly without Close()).
fn shutdown_ipc() {
    if SHUTTING_DOWN.swap(true, Ordering::SeqCst) {
        return;
    }
    // Watchdog: the process must not linger (Steam waits for all game processes),
    // even if CEF fails to shut down.
    std::thread::spawn(|| {
        std::thread::sleep(std::time::Duration::from_secs(5));
        eprintln!("helper: CEF did not shut down in 5 s, exiting");
        remove_cache_dir();
        std::process::exit(0);
    });
    with_state(|st| st.views.clear());
    let browsers: Vec<Browser> = LIVE_BROWSERS.with(|b| b.borrow().values().cloned().collect());
    if browsers.is_empty() {
        quit_message_loop();
        return;
    }
    for b in browsers {
        if let Some(host) = b.host() {
            host.close_browser(1);
        }
    }
}

// ======================================================================
// Popups (NewWindowRequested) and JS dialogs (ScriptDialogOpening)
// ======================================================================

wrap_life_span_handler! {
    struct ViewLifeSpanHandlerBuilder {
        client: ViewClient,
    }

    impl LifeSpanHandler {
        fn on_before_popup(
            &self,
            _browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            _popup_id: ::std::os::raw::c_int,
            target_url: Option<&CefString>,
            target_frame_name: Option<&CefString>,
            _target_disposition: WindowOpenDisposition,
            user_gesture: ::std::os::raw::c_int,
            popup_features: Option<&PopupFeatures>,
            window_info: Option<&mut WindowInfo>,
            client: Option<&mut Option<Client>>,
            settings: Option<&mut BrowserSettings>,
            _extra_info: Option<&mut Option<DictionaryValue>>,
            _no_javascript_access: Option<&mut ::std::os::raw::c_int>,
        ) -> ::std::os::raw::c_int {
            let url = target_url.map(|u| u.to_string()).unwrap_or_default();
            let name = target_frame_name.map(|n| n.to_string()).unwrap_or_default();
            let features = webview::WindowFeatures {
                position: popup_features.filter(|f| f.x_set != 0 && f.y_set != 0).map(|f| (f.x.max(0) as u32, f.y.max(0) as u32)),
                size: popup_features.filter(|f| f.width_set != 0 && f.height_set != 0).map(|f| (f.width.max(0) as u32, f.height.max(0) as u32)),
            };
            let size = features.size;
            let popup = NEXT_POPUP.fetch_add(1, Ordering::Relaxed);
            let (adopt, osr_size) = match webview::on_new_window(self.client.view(), popup, &url, &name, user_gesture != 0, features) {
                webview::PopupDecision::Cancel => return 1,
                webview::PopupDecision::Default => {
                    // Native CEF window (keeps window.opener); own client so popup events
                    // don't reach the parent view.
                    if let Some(wi) = window_info {
                        *wi = popup_window_info(size);
                    }
                    if let Some(client) = client {
                        *client = Some(PopupClientBuilder::new(PopupClient));
                    }
                    return 0;
                }
                webview::PopupDecision::Adopt(target) => (Some(target), view_size(target)),
                webview::PopupDecision::Deferred => (None, size),
            };
            // Real (off-screen) CEF popup so window.opener and the window.open handle work;
            // adopted by the app WebView once both are ready (on_after_created + decision).
            let (width, height) = osr_size.filter(|s| s.0 > 0 && s.1 > 0).unwrap_or((800, 600));
            let popup_client = ViewClient::new(popup);
            with_state(|st| {
                st.views.insert(popup, View {
                    popup: Some(PendingPopup { adopt, close: false, held: None, intercepted: false, client: popup_client.clone() }),
                    ..View::new(width, height)
                })
            });
            if let Some(wi) = window_info {
                *wi = WindowInfo { windowless_rendering_enabled: true as _, ..Default::default() };
            }
            if let Some(settings) = settings {
                settings.windowless_frame_rate = 60;
                settings.background_color = 0;
            }
            if let Some(client) = client {
                *client = Some(ViewClientBuilder::new(popup_client));
            }
            0
        }

        fn on_before_close(&self, browser: Option<&mut Browser>) {
            browser_closed(browser);
        }

        fn on_after_created(&self, browser: Option<&mut Browser>) {
            let (view, Some(browser)) = (self.client.view(), browser) else { return };
            browser_created(browser);
            if view < POPUP_BASE {
                return;
            }
            with_state(|st| {
                if let Some(v) = st.views.get_mut(&view) {
                    v.browser = Some(browser.clone());
                }
            });
            popup_progress(view);
        }
    }
}

/// View ids for pending popups (outside the range assigned by the DLL).
const POPUP_BASE: u32 = 0x4000_0000;
static NEXT_POPUP: AtomicU32 = AtomicU32::new(POPUP_BASE);

struct PendingPopup {
    /// Decision: adopt into this view.
    adopt: Option<u32>,
    /// Decision: close (Handled, or default window after a deferral).
    close: bool,
    /// Held first navigation (URL), resumed after adoption so navigation events
    /// (NavigationStarting...) reach the app WebView, as in WebView2.
    held: Option<String>,
    intercepted: bool,
    client: ViewClient,
}

fn view_size(view: u32) -> Option<(u32, u32)> {
    with_state(|st| st.views.get(&view).map(|v| (v.width, v.height))).flatten()
}

/// Decision after a deferral (NewWindowRequested deferral Complete()).
pub fn popup_decided(popup: u32, decision: webview::PopupDecision, uri: &str, size: Option<(u32, u32)>) {
    with_state(|st| {
        let Some(p) = st.views.get_mut(&popup).and_then(|v| v.popup.as_mut()) else { return };
        match decision {
            webview::PopupDecision::Adopt(target) => p.adopt = Some(target),
            webview::PopupDecision::Default => {
                // Off-screen browser has no window: open a new native one (no window.opener).
                p.close = true;
                open_native_popup(uri, size);
            }
            _ => p.close = true,
        }
    });
    popup_progress(popup);
}

/// Holds the first navigation of a pending popup (true = cancel in CEF).
pub fn popup_hold_navigation(view: u32, url: &str) -> bool {
    if view < POPUP_BASE {
        return false;
    }
    let held = with_state(|st| {
        let p = st.views.get_mut(&view)?.popup.as_mut()?;
        if p.intercepted {
            return Some(false);
        }
        p.intercepted = true;
        p.held = Some(url.to_string());
        Some(true)
    })
    .flatten()
    .unwrap_or(false);
    if held {
        // Not inside OnBeforeBrowse: adoption may resume the navigation right away (load_url).
        post_ui(move || popup_progress(view));
    }
    held
}

/// Carries out the decision once the popup browser exists.
fn popup_progress(popup: u32) {
    let ready = with_state(|st| {
        let v = st.views.get(&popup)?;
        let p = v.popup.as_ref()?;
        (v.browser.is_some() && (p.close || p.adopt.is_some())).then_some(())
    })
    .flatten();
    if ready.is_none() {
        return;
    }
    let Some(v) = with_state(|st| st.views.remove(&popup)).flatten() else { return };
    let (Some(browser), Some(p)) = (v.browser, v.popup) else { return };
    match p.adopt.filter(|_| !p.close) {
        Some(target) => adopt_popup(browser, p, target),
        None => {
            if let Some(host) = browser.host() {
                host.close_browser(1);
            }
        }
    }
}

/// NewWindow: the popup browser replaces the (still empty) browser of the app view.
fn adopt_popup(browser: Browser, p: PendingPopup, target: u32) {
    let old = with_state(|st| {
        let v = st.views.get_mut(&target)?;
        let old = v.browser.replace(browser.clone());
        let observer = v.devtools_observer.take();
        v.last_source.clear();
        v.pending_completion = None;
        Some((old, observer, v.settings, v.doc_scripts.clone(), v.user_agent.clone()))
    })
    .flatten();
    let Some((old, observer, settings, scripts, ua)) = old else {
        if let Some(host) = browser.host() {
            host.close_browser(1);
        }
        return;
    };
    p.client.id.store(target, Ordering::Relaxed);
    drop(observer);
    if let Some(host) = old.and_then(|b| b.host()) {
        host.close_browser(1);
    }
    let registration = browser.host().and_then(|h| {
        let mut observer = DevToolsObserverBuilder::new(DevToolsObserver { view: target });
        h.add_dev_tools_message_observer(Some(&mut observer))
    });
    with_state(|st| {
        if let Some(v) = st.views.get_mut(&target) {
            v.devtools_observer = registration;
        }
    });
    set_user_agent(&browser, &ua);
    apply_settings(target, settings);
    set_doc_scripts(target, scripts);
    if let Some(host) = browser.host() {
        host.was_resized();
        host.invalidate(PaintElementType::VIEW);
    }
    eprintln!("helper: popup adopted by view {target}");
    // FIXME: POST data of the first navigation is lost (CEF rejects cross-origin load_request).
    match p.held {
        Some(url) => {
            if let Some(frame) = browser.main_frame() {
                frame.load_url(Some(&url.as_str().into()));
            }
        }
        None => {
            // Document without network navigation (about:blank) already loaded: emit events now.
            let url = browser.main_frame().map(|f| CefString::from(&f.url()).to_string()).unwrap_or_default();
            if !url.is_empty() {
                let id = resources::begin_navigation(target, &url);
                webview::on_navigation_starting(target, id, false, false, &url);
                set_last_source(target, &url);
                webview::on_source_changed(target, &url);
                webview::on_content_loading(target, id);
                if browser.is_loading() == 0 {
                    send_history(target, browser.can_go_back() != 0, browser.can_go_forward() != 0);
                    send_navigation_completed(target, 0, 0);
                }
            }
        }
    }
}

fn popup_window_info(size: Option<(u32, u32)>) -> WindowInfo {
    let (width, height) = size.filter(|s| s.0 > 0 && s.1 > 0).unwrap_or((800, 600));
    WindowInfo {
        bounds: Rect { x: 0, y: 0, width: width as i32, height: height as i32 },
        runtime_style: RuntimeStyle::ALLOY,
        ..Default::default()
    }
}

/// Default window when decided after a deferral (the CEF popup was already cancelled).
pub fn open_native_popup(url: &str, size: Option<(u32, u32)>) {
    let url = url.to_string();
    post_ui(move || {
        let mut client = PopupClientBuilder::new(PopupClient);
        browser_host_create_browser(
            Some(&popup_window_info(size)),
            Some(&mut client),
            Some(&url.as_str().into()),
            Some(&BrowserSettings::default()),
            None,
            None,
        );
    });
}

#[derive(Clone)]
struct PopupClient;

wrap_life_span_handler! {
    struct PopupLifeSpanHandlerBuilder {
        client: PopupClient,
    }

    impl LifeSpanHandler {
        fn on_after_created(&self, browser: Option<&mut Browser>) {
            if let Some(browser) = browser {
                browser_created(browser);
            }
        }

        fn on_before_close(&self, browser: Option<&mut Browser>) {
            browser_closed(browser);
        }
    }
}

wrap_client! {
    struct PopupClientBuilder {
        client: PopupClient,
    }

    impl Client {
        fn life_span_handler(&self) -> Option<LifeSpanHandler> {
            Some(PopupLifeSpanHandlerBuilder::new(self.client.clone()))
        }

        fn on_process_message_received(
            &self,
            browser: Option<&mut Browser>,
            _frame: Option<&mut Frame>,
            _source_process: ProcessId,
            message: Option<&mut ProcessMessage>,
        ) -> ::std::os::raw::c_int {
            // window.close() in a native popup (the renderer overrides window.close).
            let Some(message) = message else { return 0 };
            if CefString::from(&message.name()).to_string() == renderer::MSG_CLOSE {
                if let Some(host) = browser.and_then(|b| b.host()) {
                    host.close_browser(0);
                }
                return 1;
            }
            0
        }
    }
}

/// COREWEBVIEW2_SCRIPT_DIALOG_KIND
const DIALOG_ALERT: i32 = 0;
const DIALOG_CONFIRM: i32 = 1;
const DIALOG_PROMPT: i32 = 2;
const DIALOG_BEFOREUNLOAD: i32 = 3;

thread_local! {
    /// Default dialogs awaiting the DLL reply (CMD_SCRIPT_DIALOG_REPLY).
    static DIALOGS: RefCell<HashMap<u64, JsdialogCallback>> = RefCell::new(HashMap::new());
    static NEXT_DIALOG: std::cell::Cell<u64> = const { std::cell::Cell::new(1) };
}

/// JS dialog: default UI in the app (MessageBox in the DLL) or ScriptDialogOpening.
fn show_dialog(view: u32, uri: &str, kind: i32, message: &str, default_text: &str, callback: JsdialogCallback) {
    let defaults = with_state(|st| st.views.get(&view).map(|v| v.settings & SETTING_SCRIPT_DIALOGS != 0))
        .flatten()
        .unwrap_or(true);
    if defaults {
        let id = NEXT_DIALOG.with(|n| {
            let id = n.get();
            n.set(id + 1);
            id
        });
        DIALOGS.with(|d| d.borrow_mut().insert(id, callback));
        let payload = Writer::default()
            .u64(id)
            .u32(kind as u32)
            .u32(uri.len() as u32)
            .u32(message.len() as u32)
            .u32(default_text.len() as u32)
            .bytes(uri.as_bytes())
            .bytes(message.as_bytes())
            .bytes(default_text.as_bytes());
        send_event(EVT_SCRIPT_DIALOG, view, &payload.0);
        return;
    }
    // Separate UI task: the app handler runs synchronously and must not nest inside OnJSDialog.
    let (uri, message, default_text) = (uri.to_string(), message.to_string(), default_text.to_string());
    post_ui(move || {
        let done: webview::DialogDone = Box::new(move |accepted, text| {
            callback.cont(accepted as i32, Some(&text.into()));
        });
        webview::on_script_dialog(view, &uri, kind, &message, &default_text, done);
    });
}

fn dialog_reply(id: u64, accepted: bool, text: &str) {
    if let Some(callback) = DIALOGS.with(|d| d.borrow_mut().remove(&id)) {
        callback.cont(accepted as i32, Some(&text.into()));
    }
}

wrap_jsdialog_handler! {
    struct ViewDialogHandlerBuilder {
        client: ViewClient,
    }

    impl JsdialogHandler {
        fn on_jsdialog(
            &self,
            _browser: Option<&mut Browser>,
            origin_url: Option<&CefString>,
            dialog_type: JsdialogType,
            message_text: Option<&CefString>,
            default_prompt_text: Option<&CefString>,
            callback: Option<&mut JsdialogCallback>,
            _suppress_message: Option<&mut ::std::os::raw::c_int>,
        ) -> ::std::os::raw::c_int {
            let Some(callback) = callback else { return 0 };
            let kind = match dialog_type {
                JsdialogType::CONFIRM => DIALOG_CONFIRM,
                JsdialogType::PROMPT => DIALOG_PROMPT,
                _ => DIALOG_ALERT,
            };
            let uri = origin_url.map(|u| u.to_string()).unwrap_or_default();
            let message = message_text.map(|m| m.to_string()).unwrap_or_default();
            let default_text = default_prompt_text.map(|t| t.to_string()).unwrap_or_default();
            show_dialog(self.client.view(), &uri, kind, &message, &default_text, callback.clone());
            1
        }

        fn on_before_unload_dialog(
            &self,
            browser: Option<&mut Browser>,
            message_text: Option<&CefString>,
            _is_reload: ::std::os::raw::c_int,
            callback: Option<&mut JsdialogCallback>,
        ) -> ::std::os::raw::c_int {
            let Some(callback) = callback else { return 0 };
            let uri = browser.and_then(|b| b.main_frame()).map(|f| CefString::from(&f.url()).to_string()).unwrap_or_default();
            let message = message_text.map(|m| m.to_string()).unwrap_or_default();
            show_dialog(self.client.view(), &uri, DIALOG_BEFOREUNLOAD, &message, "", callback.clone());
            1
        }

        fn on_reset_dialog_state(&self, _browser: Option<&mut Browser>) {}
    }
}

// ======================================================================
// Browser operations for server objects (webview.rs)
// ======================================================================

/// Creates the view's CEF browser (called from ROOT_CREATE_CONTROLLER).
pub fn create_browser(view: u32, width: u32, height: u32) -> bool {
    with_state(|st| {
        st.views.insert(view, View::new(width, height))
    });
    let window_info = WindowInfo { windowless_rendering_enabled: true as _, ..Default::default() };
    // Always transparent: OSR transparency is fixed at creation, so the DLL applies
    // DefaultBackgroundColor under the frame (color or parent content).
    let settings = BrowserSettings { windowless_frame_rate: 60, background_color: 0, ..Default::default() };
    let mut client = ViewClientBuilder::new(ViewClient::new(view));
    let browser = browser_host_create_browser_sync(
        Some(&window_info),
        Some(&mut client),
        // Empty URL = no initial navigation (WebView2 raises no events before Navigate).
        Some(&"".into()),
        Some(&settings),
        None,
        None,
    );
    let Some(browser) = browser else {
        with_state(|st| st.views.remove(&view));
        return false;
    };
    set_user_agent(&browser, "");
    let registration = browser.host().and_then(|h| {
        let mut observer = DevToolsObserverBuilder::new(DevToolsObserver { view });
        h.add_dev_tools_message_observer(Some(&mut observer))
    });
    with_state(|st| {
        if let Some(v) = st.views.get_mut(&view) {
            v.browser = Some(browser);
            v.devtools_observer = registration;
        }
    });
    true
}

pub fn close_browser(view: u32) {
    if let Some(v) = with_state(|st| st.views.remove(&view)).flatten() {
        if let Some(host) = v.browser.and_then(|b| b.host()) {
            host.close_browser(1);
        }
    }
}

/// Runs in a separate UI task: CEF calls OnBeforeBrowse inside load_url, and the synchronous
/// NavigationStarting must not fire during the DLL call. WebView2 also raises it after Navigate() returns.
pub fn navigate(view: u32, url: &str) {
    let url = url.to_string();
    post_ui(move || {
        if let Some(frame) = view_browser(view).and_then(|b| b.main_frame()) {
            frame.load_url(Some(&url.as_str().into()));
        }
    });
}

pub enum BrowserCommand {
    Back,
    Forward,
    Reload,
    Stop,
}

pub fn browser_command(view: u32, cmd: BrowserCommand) {
    post_ui(move || {
        if let Some(b) = view_browser(view) {
            match cmd {
                BrowserCommand::Back => b.go_back(),
                BrowserCommand::Forward => b.go_forward(),
                BrowserCommand::Reload => b.reload(),
                BrowserCommand::Stop => b.stop_load(),
            }
        }
    });
}

/// Native DevTools window (Chrome style); blocked by AreDevToolsEnabled = FALSE.
pub fn open_devtools(view: u32) {
    let allowed = with_state(|st| st.views.get(&view).map(|v| v.settings & SETTING_DEVTOOLS != 0)).flatten().unwrap_or(true);
    if let Some(host) = view_browser(view).and_then(|b| b.host()).filter(|_| allowed) {
        host.show_dev_tools(Some(&WindowInfo::default()), None, Some(&BrowserSettings::default()), None);
    }
}

/// Sends a script to the renderer; false if there is no main frame.
pub fn execute_script(view: u32, id: u64, script: &str) -> bool {
    let frame = view_browser(view).and_then(|b| b.main_frame());
    frame
        .and_then(|frame| {
            let mut m = process_message_create(Some(&renderer::MSG_EXEC.into()))?;
            let list = m.argument_list()?;
            list.set_string(0, Some(&id.to_string().as_str().into()));
            list.set_string(1, Some(&script.into()));
            frame.send_process_message(ProcessId::RENDERER, Some(&mut m));
            Some(())
        })
        .is_some()
}

pub fn set_doc_scripts(view: u32, scripts: Vec<String>) {
    let browser_id = with_state(|st| {
        let v = st.views.get_mut(&view)?;
        v.doc_scripts = scripts.clone();
        Some(v.browser.as_ref()?.identifier())
    })
    .flatten();
    if let (Some(browser_id), Some(dir)) = (browser_id, SCRIPTS_DIR.get()) {
        write_doc_scripts(dir, browser_id, &scripts);
    }
}

pub fn post_web_message(view: u32, json: &str) {
    if let Some(frame) = view_browser(view).and_then(|b| b.main_frame()) {
        let code = format!("window.chrome&&window.chrome.webview&&window.chrome.webview.__dispatch({json});");
        frame.execute_java_script(Some(&code.as_str().into()), Some(&"".into()), 0);
    }
}

/// Host commands (window, frames, input) - on the CEF UI thread.
fn handle_command(msg: Message) {
    let mut r = Reader(&msg.payload);
    match msg.ty {
        CMD_CLOSE_VIEW => webview::close_view(msg.view),
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
        CMD_SCRIPT_DIALOG_REPLY => {
            let (id, accepted, len) = (r.u64().unwrap_or(0), r.u32().unwrap_or(0) != 0, r.u32().unwrap_or(0));
            let text = r.take(len as usize).map(|t| String::from_utf8_lossy(t).into_owned()).unwrap_or_default();
            dialog_reply(id, accepted, &text);
        }
        CMD_SHUTDOWN => shutdown_ipc(),
        other => eprintln!("helper: unknown command {other:#x}"),
    }
}

fn reader_thread(chan: Channel) {
    loop {
        match chan.recv() {
            Ok(Some(msg)) => {
                rpc::enqueue(Some(msg));
                post_ui(drain_queue);
            }
            Ok(None) => break,
            Err(e) => {
                eprintln!("helper: recv failed: {e}");
                break;
            }
        }
    }
    rpc::enqueue(None);
    post_ui(drain_queue);
}

/// UI task: drains the queue (CALL/RELEASE via rpc, the rest via handle_command).
fn drain_queue() {
    if !rpc::drain() {
        shutdown_ipc();
    }
}

// ======================================================================
// Screenshot mode (test)
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

/// CEF delivers BGRA; PPM (P6) is RGB.
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
            // WebView2 has no popup blocker (window.open without a gesture -> NewWindowRequested).
            cl.append_switch(Some(&"disable-popup-blocking".into()));
            cl.append_switch(Some(&"password-store=basic".into()));
            // Debugging: remote DevTools (chrome://inspect in regular Chrome).
            if let Ok(port) = std::env::var("WINE_WEBVIEW2_DEVTOOLS_PORT") {
                cl.append_switch_with_value(Some(&"remote-debugging-port".into()), Some(&port.as_str().into()));
            }
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
                    match chan.try_clone() {
                        Ok(c) => resources::set_sender(c),
                        Err(e) => eprintln!("helper: cannot clone channel: {e}"),
                    }
                    STATE.with(|s| {
                        *s.borrow_mut() = Some(IpcState { chan, views: HashMap::new(), next_generation: 0 })
                    });
                    rpc::set_handlers(root_call, handle_command);
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

/// Directory with libcef.so and resources (icudtl.dat, *.pak, locales/).
fn cef_dir() -> PathBuf {
    if let Some(dir) = std::env::var_os("WINE_WEBVIEW2_CEF_DIR") {
        return PathBuf::from(dir);
    }
    std::env::current_exe().ok().and_then(|p| p.parent().map(|p| p.to_path_buf())).unwrap_or_default()
}

/// Protocol version handshake before starting CEF.
fn handshake(chan: &Channel) -> Result<(), String> {
    let msg = chan.recv().map_err(|e| e.to_string())?.ok_or("connection closed")?;
    let mut r = Reader(&msg.payload);
    let (version, _pid, hash) = (r.u32(), r.u32(), r.u64());
    if msg.ty != CMD_HELLO || version != Some(PROTOCOL_VERSION) || hash != Some(api_gen::API_HASH) {
        let _ = chan.send(EVT_ERROR, 0, b"protocol version mismatch", None);
        return Err(format!("bad hello (type {:#x}, version {:?}, api {:x?})", msg.ty, version, hash));
    }
    let reply = Writer::default().u32(PROTOCOL_VERSION).u32(std::process::id()).u64(api_gen::API_HASH).0;
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
        eprintln!("usage: wine-webview2-helper --ipc-fd=N | --screenshot <url> <file.ppm>");
        return std::process::ExitCode::from(2);
    };

    let args = Args::new();
    let mut app = AppBuilder::new(HelperApp { mode: mode.clone() });

    // CEF subprocesses (renderer, gpu, utility) exit here.
    let ret = execute_process(Some(args.as_main_args()), Some(&mut app), std::ptr::null_mut());
    if ret >= 0 {
        return std::process::ExitCode::from(ret as u8);
    }

    let dir = cef_dir();
    let exe = std::env::current_exe().unwrap_or_default();
    remove_stale_cache_dirs();
    let cache = std::env::temp_dir().join(format!("wine-webview2-helper-{}", std::process::id()));
    let _ = CACHE_DIR.set(cache.clone());
    lock_cache_dir(&cache);
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
        // Windows/Edge UA from the first request (the DevTools override arrives later).
        user_agent: DEFAULT_USER_AGENT.into(),
        ..Default::default()
    };
    if initialize(Some(args.as_main_args()), Some(&settings), Some(&mut app), std::ptr::null_mut()) != 1 {
        eprintln!("helper: cef initialize failed");
        return std::process::ExitCode::from(1);
    }
    run_message_loop();
    STATE.with(|s| s.borrow_mut().take());
    shutdown();
    remove_cache_dir();

    match mode {
        Mode::Shot(s) if !s.borrow().done => std::process::ExitCode::from(1),
        _ => std::process::ExitCode::SUCCESS,
    }
}
