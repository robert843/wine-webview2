//! Środowisko, kontroler i ICoreWebView2 jako obiekty serwera (architektura v8).
//!
//! Zdarzenia CEF (main.rs, resources.rs) trafiają tu przez funkcje `on_*` i są dostarczane
//! handlerom aplikacji przez INVOKE — asynchronicznie, poza NavigationStarting
//! i WebResourceRequested, które muszą znać decyzję aplikacji.

use crate::api_gen::*;
use crate::rpc::{AppRef, Ctx, Dispatch, ObjRef, WvResult, E_INVALIDARG, E_INVALID_STATE, S_OK};
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::rc::Rc;

pub const VERSION: &str = "156.0.9999.0";

// ===================================================================
// Rejestracje zdarzeń
// ===================================================================

#[derive(Default)]
pub struct EventList {
    next: Cell<i64>,
    list: RefCell<Vec<(i64, AppRef)>>,
}

impl EventList {
    pub fn add(&self, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        let app = handler.app().cloned().ok_or(E_INVALIDARG)?;
        let token = self.next.get() + 1;
        self.next.set(token);
        self.list.borrow_mut().push((token, app));
        Ok(EventRegistrationToken { value: token })
    }
    pub fn remove(&self, token: EventRegistrationToken) -> WvResult<()> {
        self.list.borrow_mut().retain(|(t, _)| *t != token.value);
        Ok(())
    }
    /// Kopia (handlery mogą się (wy)rejestrować w trakcie wywołania).
    pub fn snapshot(&self) -> Vec<AppRef> {
        self.list.borrow().iter().map(|(_, a)| a.clone()).collect()
    }
}

#[derive(Clone, Copy)]
enum Ev {
    NavigationStarting,
    ContentLoading,
    SourceChanged,
    HistoryChanged,
    DomContentLoaded,
    NavigationCompleted,
    DocumentTitleChanged,
    WebMessageReceived,
    WebResourceRequested,
    NewWindowRequested,
    WindowCloseRequested,
    ScriptDialogOpening,
}
const EV_COUNT: usize = 12;

fn rc<T: Dispatch + 'static>(obj: T) -> ObjRef {
    ObjRef::Runtime(Rc::new(obj))
}

// ===================================================================
// Środowisko
// ===================================================================

pub struct Environment {
    #[allow(dead_code)] // profil użytkownika: później (root_cache_path per środowisko)
    pub user_data_dir: String,
}

impl ICoreWebView2Environment15Impl for Environment {
    fn get_BrowserVersionString(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(VERSION.to_string())
    }

    fn CreateWebResourceResponse(&self, _ctx: &mut Ctx, content: Option<Vec<u8>>, status: i32,
                                 reason: Option<String>, headers: Option<String>) -> WvResult<ObjRef> {
        Ok(crate::webresource::create_response(content, status, reason.unwrap_or_default(), headers.as_deref()))
    }
}

// ===================================================================
// Kontroler
// ===================================================================

/// Okno, rozmiar, widoczność, fokus i tło obsługuje host w DLL (nadpisania w vtable).
pub struct Controller {
    pub view: u32,
    pub webview: ObjRef,
}

impl ICoreWebView2Controller4Impl for Controller {
    fn get_CoreWebView2(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        if !WEBVIEWS.with(|w| w.borrow().contains_key(&self.view)) {
            return Err(E_INVALID_STATE);
        }
        Ok(self.webview.clone())
    }

    fn Close(&self, _ctx: &mut Ctx) -> WvResult<()> {
        close_view(self.view);
        Ok(())
    }
}

pub fn close_view(view: u32) {
    WEBVIEWS.with(|w| w.borrow_mut().remove(&view));
    crate::resources::remove_view(view);
    crate::close_browser(view);
}

// ===================================================================
// ICoreWebView2
// ===================================================================

pub type WebViewObj = ICoreWebView2_29Obj<WebView>;

thread_local! {
    /// Widok -> WebView (dla zdarzeń z CEF).
    static WEBVIEWS: RefCell<HashMap<u32, Rc<WebViewObj>>> = RefCell::new(HashMap::new());
}

pub struct WebView {
    pub view: u32,
    events: [EventList; EV_COUNT],
    source: RefCell<String>,
    title: RefCell<String>,
    can_go_back: Cell<bool>,
    can_go_forward: Cell<bool>,
    settings: RefCell<Option<ObjRef>>,
    pending_scripts: RefCell<HashMap<u64, AppRef>>,
    pending_devtools: RefCell<HashMap<i32, AppRef>>,
    devtools_receivers: RefCell<Vec<Rc<ICoreWebView2DevToolsProtocolEventReceiverObj<DevToolsReceiver>>>>,
    next_id: Cell<u64>,
    doc_scripts: RefCell<Vec<(u64, String)>>,
}

impl WebView {
    fn new(view: u32) -> Self {
        WebView {
            view,
            events: Default::default(),
            source: RefCell::new(String::new()),
            title: RefCell::new(String::new()),
            can_go_back: Cell::new(false),
            can_go_forward: Cell::new(false),
            settings: RefCell::new(None),
            pending_scripts: RefCell::new(HashMap::new()),
            pending_devtools: RefCell::new(HashMap::new()),
            devtools_receivers: RefCell::new(Vec::new()),
            next_id: Cell::new(1),
            doc_scripts: RefCell::new(Vec::new()),
        }
    }

    fn id(&self) -> u64 {
        let id = self.next_id.get();
        self.next_id.set(id + 1);
        id
    }

    fn ev(&self, e: Ev) -> &EventList {
        &self.events[e as usize]
    }
}

/// Tworzy widok: przeglądarkę CEF, WebView i kontroler.
pub fn create_controller(view: u32, width: u32, height: u32) -> WvResult<ObjRef> {
    if !crate::create_browser(view, width, height) {
        return Err(crate::rpc::E_FAIL);
    }
    let webview = Rc::new(ICoreWebView2_29Obj(WebView::new(view)));
    WEBVIEWS.with(|w| w.borrow_mut().insert(view, webview.clone()));
    let webview_ref = ObjRef::Runtime(webview);
    Ok(rc(ICoreWebView2Controller4Obj(Controller { view, webview: webview_ref })))
}

fn with_webview<R>(view: u32, f: impl FnOnce(&WebView, ObjRef) -> R) -> Option<R> {
    let wv = WEBVIEWS.with(|w| w.borrow().get(&view).cloned())?;
    let sender = ObjRef::Runtime(wv.clone());
    Some(f(&wv.0, sender))
}

macro_rules! events {
    ($( $add:ident, $remove:ident, $ev:ident; )*) => {
        $(
            fn $add(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
                self.ev(Ev::$ev).add(handler)
            }
            fn $remove(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
                self.ev(Ev::$ev).remove(token)
            }
        )*
    };
}

impl ICoreWebView2_29Impl for WebView {
    events! {
        add_NavigationStarting, remove_NavigationStarting, NavigationStarting;
        add_ContentLoading, remove_ContentLoading, ContentLoading;
        add_SourceChanged, remove_SourceChanged, SourceChanged;
        add_HistoryChanged, remove_HistoryChanged, HistoryChanged;
        add_DOMContentLoaded, remove_DOMContentLoaded, DomContentLoaded;
        add_NavigationCompleted, remove_NavigationCompleted, NavigationCompleted;
        add_DocumentTitleChanged, remove_DocumentTitleChanged, DocumentTitleChanged;
        add_WebMessageReceived, remove_WebMessageReceived, WebMessageReceived;
        add_WebResourceRequested, remove_WebResourceRequested, WebResourceRequested;
        add_NewWindowRequested, remove_NewWindowRequested, NewWindowRequested;
        add_WindowCloseRequested, remove_WindowCloseRequested, WindowCloseRequested;
        add_ScriptDialogOpening, remove_ScriptDialogOpening, ScriptDialogOpening;
    }

    fn get_Settings(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        let mut s = self.settings.borrow_mut();
        Ok(s.get_or_insert_with(|| rc(ICoreWebView2Settings9Obj(crate::settings::Settings::new(self.view)))).clone())
    }

    fn get_Source(&self, _ctx: &mut Ctx) -> WvResult<String> {
        let s = self.source.borrow();
        Ok(if s.is_empty() { "about:blank".into() } else { s.clone() })
    }

    fn get_DocumentTitle(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.title.borrow().clone())
    }

    fn get_CanGoBack(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.can_go_back.get())
    }

    fn get_CanGoForward(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.can_go_forward.get())
    }

    fn Navigate(&self, _ctx: &mut Ctx, uri: Option<String>) -> WvResult<()> {
        let uri = uri.ok_or(E_INVALIDARG)?;
        crate::navigate(self.view, &uri);
        Ok(())
    }

    fn NavigateToString(&self, _ctx: &mut Ctx, html: Option<String>) -> WvResult<()> {
        let html = html.ok_or(E_INVALIDARG)?;
        crate::navigate(self.view, &format!("data:text/html;charset=utf-8;base64,{}", crate::base64(html.as_bytes())));
        Ok(())
    }

    fn GoBack(&self, _ctx: &mut Ctx) -> WvResult<()> {
        crate::browser_command(self.view, crate::BrowserCommand::Back);
        Ok(())
    }

    fn GoForward(&self, _ctx: &mut Ctx) -> WvResult<()> {
        crate::browser_command(self.view, crate::BrowserCommand::Forward);
        Ok(())
    }

    fn Reload(&self, _ctx: &mut Ctx) -> WvResult<()> {
        crate::browser_command(self.view, crate::BrowserCommand::Reload);
        Ok(())
    }

    fn Stop(&self, _ctx: &mut Ctx) -> WvResult<()> {
        crate::browser_command(self.view, crate::BrowserCommand::Stop);
        Ok(())
    }

    fn OpenDevToolsWindow(&self, _ctx: &mut Ctx) -> WvResult<()> {
        crate::open_devtools(self.view);
        Ok(())
    }

    fn ExecuteScript(&self, ctx: &mut Ctx, script: Option<String>, handler: ObjRef) -> WvResult<()> {
        let script = script.ok_or(E_INVALIDARG)?;
        let id = self.id();
        if let Some(app) = handler.app() {
            self.pending_scripts.borrow_mut().insert(id, app.clone());
        }
        if !crate::execute_script(self.view, id, &script) {
            // Brak renderera (np. przed pierwszą nawigacją): jak skrypt bez wyniku.
            if let Some(app) = self.pending_scripts.borrow_mut().remove(&id) {
                ICoreWebView2ExecuteScriptCompletedHandlerApp(app).Invoke_async(ctx, S_OK, Some("null"));
            }
        }
        Ok(())
    }

    fn AddScriptToExecuteOnDocumentCreated(&self, ctx: &mut Ctx, script: Option<String>, handler: ObjRef) -> WvResult<()> {
        let script = script.ok_or(E_INVALIDARG)?;
        let id = self.id();
        self.doc_scripts.borrow_mut().push((id, script));
        crate::set_doc_scripts(self.view, self.doc_scripts.borrow().iter().map(|(_, s)| s.clone()).collect());
        if let Some(app) = handler.app() {
            let id = id.to_string();
            ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandlerApp(app.clone()).Invoke_async(ctx, S_OK, Some(&id));
        }
        Ok(())
    }

    fn RemoveScriptToExecuteOnDocumentCreated(&self, _ctx: &mut Ctx, id: Option<String>) -> WvResult<()> {
        let id: u64 = id.and_then(|i| i.parse().ok()).ok_or(E_INVALIDARG)?;
        self.doc_scripts.borrow_mut().retain(|(i, _)| *i != id);
        crate::set_doc_scripts(self.view, self.doc_scripts.borrow().iter().map(|(_, s)| s.clone()).collect());
        Ok(())
    }

    fn PostWebMessageAsJson(&self, _ctx: &mut Ctx, json: Option<String>) -> WvResult<()> {
        crate::post_web_message(self.view, &json.ok_or(E_INVALIDARG)?);
        Ok(())
    }

    fn PostWebMessageAsString(&self, _ctx: &mut Ctx, s: Option<String>) -> WvResult<()> {
        crate::post_web_message(self.view, &crate::renderer::json_string(&s.ok_or(E_INVALIDARG)?));
        Ok(())
    }

    /// folderPath przychodzi już jako ścieżka uniksowa (konwersja w hoście DLL).
    fn SetVirtualHostNameToFolderMapping(&self, _ctx: &mut Ctx, host: Option<String>, folder: Option<String>,
                                         _access: i32) -> WvResult<()> {
        crate::resources::set_virtual_host(self.view, &host.ok_or(E_INVALIDARG)?, folder.ok_or(E_INVALIDARG)?.into());
        Ok(())
    }

    fn ClearVirtualHostNameToFolderMapping(&self, _ctx: &mut Ctx, host: Option<String>) -> WvResult<()> {
        crate::resources::clear_virtual_host(self.view, &host.ok_or(E_INVALIDARG)?);
        Ok(())
    }

    fn CallDevToolsProtocolMethod(&self, ctx: &mut Ctx, method: Option<String>, params: Option<String>,
                                  handler: ObjRef) -> WvResult<()> {
        let method = method.ok_or(E_INVALIDARG)?;
        let id = crate::next_devtools_id();
        if let Some(app) = handler.app() {
            self.pending_devtools.borrow_mut().insert(id, app.clone());
        }
        if !crate::devtools_call(self.view, id, &method, params.as_deref().unwrap_or("{}")) {
            if let Some(app) = self.pending_devtools.borrow_mut().remove(&id) {
                ICoreWebView2CallDevToolsProtocolMethodCompletedHandlerApp(app).Invoke_async(ctx, crate::rpc::E_FAIL, Some("{}"));
            }
        }
        Ok(())
    }

    fn GetDevToolsProtocolEventReceiver(&self, _ctx: &mut Ctx, event: Option<String>) -> WvResult<ObjRef> {
        let event = event.ok_or(E_INVALIDARG)?;
        let mut list = self.devtools_receivers.borrow_mut();
        if let Some(r) = list.iter().find(|r| r.0.event == event) {
            return Ok(ObjRef::Runtime(r.clone()));
        }
        let r = Rc::new(ICoreWebView2DevToolsProtocolEventReceiverObj(DevToolsReceiver { event, handlers: EventList::default() }));
        list.push(r.clone());
        Ok(ObjRef::Runtime(r))
    }

    fn AddWebResourceRequestedFilter(&self, _ctx: &mut Ctx, uri: Option<String>, context: i32) -> WvResult<()> {
        crate::resources::add_filter(self.view, uri.ok_or(E_INVALIDARG)?, context as u32);
        Ok(())
    }

    fn RemoveWebResourceRequestedFilter(&self, _ctx: &mut Ctx, uri: Option<String>, context: i32) -> WvResult<()> {
        crate::resources::remove_filter(self.view, &uri.ok_or(E_INVALIDARG)?, context as u32);
        Ok(())
    }

    fn AddWebResourceRequestedFilterWithRequestSourceKinds(&self, _ctx: &mut Ctx, uri: Option<String>, context: i32,
                                                           _kinds: i32) -> WvResult<()> {
        crate::resources::add_filter(self.view, uri.ok_or(E_INVALIDARG)?, context as u32);
        Ok(())
    }

    fn RemoveWebResourceRequestedFilterWithRequestSourceKinds(&self, _ctx: &mut Ctx, uri: Option<String>, context: i32,
                                                              _kinds: i32) -> WvResult<()> {
        crate::resources::remove_filter(self.view, &uri.ok_or(E_INVALIDARG)?, context as u32);
        Ok(())
    }
}

// ===================================================================
// Argumenty zdarzeń
// ===================================================================

pub struct DevToolsReceiver {
    event: String,
    handlers: EventList,
}

impl ICoreWebView2DevToolsProtocolEventReceiverImpl for DevToolsReceiver {
    fn add_DevToolsProtocolEventReceived(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.handlers.add(handler)
    }
    fn remove_DevToolsProtocolEventReceived(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.handlers.remove(token)
    }
}

pub struct DevToolsEventArgs {
    params: String,
}

impl ICoreWebView2DevToolsProtocolEventReceivedEventArgs2Impl for DevToolsEventArgs {
    fn get_ParameterObjectAsJson(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.params.clone())
    }
    fn get_SessionId(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(String::new())
    }
}

pub struct NavigationStartingArgs {
    uri: String,
    user_initiated: bool,
    redirected: bool,
    id: u64,
    cancel: Cell<bool>,
}

impl ICoreWebView2NavigationStartingEventArgs3Impl for NavigationStartingArgs {
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.clone())
    }
    fn get_IsUserInitiated(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.user_initiated)
    }
    fn get_IsRedirected(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.redirected)
    }
    fn get_Cancel(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.cancel.get())
    }
    fn put_Cancel(&self, _ctx: &mut Ctx, cancel: bool) -> WvResult<()> {
        self.cancel.set(cancel);
        Ok(())
    }
    fn get_NavigationId(&self, _ctx: &mut Ctx) -> WvResult<u64> {
        Ok(self.id)
    }
    fn get_NavigationKind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(1) // COREWEBVIEW2_NAVIGATION_KIND_NEW_DOCUMENT
    }
}

pub struct ContentLoadingArgs {
    id: u64,
    error_page: bool,
}

impl ICoreWebView2ContentLoadingEventArgsImpl for ContentLoadingArgs {
    fn get_IsErrorPage(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.error_page)
    }
    fn get_NavigationId(&self, _ctx: &mut Ctx) -> WvResult<u64> {
        Ok(self.id)
    }
}

pub struct SourceChangedArgs {
    new_document: bool,
}

impl ICoreWebView2SourceChangedEventArgsImpl for SourceChangedArgs {
    fn get_IsNewDocument(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.new_document)
    }
}

pub struct DomContentLoadedArgs {
    id: u64,
}

impl ICoreWebView2DOMContentLoadedEventArgsImpl for DomContentLoadedArgs {
    fn get_NavigationId(&self, _ctx: &mut Ctx) -> WvResult<u64> {
        Ok(self.id)
    }
}

pub struct NavigationCompletedArgs {
    id: u64,
    success: bool,
    web_error: i32,
    http_status: i32,
}

impl ICoreWebView2NavigationCompletedEventArgs2Impl for NavigationCompletedArgs {
    fn get_IsSuccess(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.success)
    }
    fn get_WebErrorStatus(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.web_error)
    }
    fn get_NavigationId(&self, _ctx: &mut Ctx) -> WvResult<u64> {
        Ok(self.id)
    }
    fn get_HttpStatusCode(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.http_status)
    }
}

pub struct WebMessageArgs {
    source: String,
    json: String,
}

impl ICoreWebView2WebMessageReceivedEventArgs2Impl for WebMessageArgs {
    fn get_Source(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.source.clone())
    }
    fn get_WebMessageAsJson(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.json.clone())
    }
    /// Udaje się tylko, gdy strona wysłała napis.
    fn TryGetWebMessageAsString(&self, _ctx: &mut Ctx) -> WvResult<String> {
        json_unquote(&self.json).ok_or(E_INVALIDARG)
    }
}

fn json_unquote(json: &str) -> Option<String> {
    let s = json.trim();
    let inner = s.strip_prefix('"')?.strip_suffix('"')?;
    let mut out = String::with_capacity(inner.len());
    let mut chars = inner.chars();
    let mut pending_high: Option<u32> = None;
    while let Some(c) = chars.next() {
        if c != '\\' {
            out.push(c);
            continue;
        }
        match chars.next()? {
            'b' => out.push('\u{8}'),
            'f' => out.push('\u{c}'),
            'n' => out.push('\n'),
            'r' => out.push('\r'),
            't' => out.push('\t'),
            'u' => {
                let hex: String = chars.by_ref().take(4).collect();
                let v = u32::from_str_radix(&hex, 16).ok()?;
                if (0xd800..0xdc00).contains(&v) {
                    pending_high = Some(v);
                } else if let Some(h) = pending_high.take().filter(|_| (0xdc00..0xe000).contains(&v)) {
                    out.push(char::from_u32(0x10000 + ((h - 0xd800) << 10) + (v - 0xdc00))?);
                } else {
                    out.push(char::from_u32(v).unwrap_or('\u{fffd}'));
                }
            }
            other => out.push(other),
        }
    }
    Some(out)
}

/// Chromium net error -> COREWEBVIEW2_WEB_ERROR_STATUS
fn web_error_status(net_error: i32) -> i32 {
    match net_error {
        0 => 0,
        -3 => 14,
        -7 | -118 => 7,
        -100 | -106 => 11,
        -101 => 10,
        -102 | -104 => 12,
        -103 => 9,
        -105 | -137 => 13,
        -109 => 6,
        -200 => 1,
        -201 => 2,
        -202 | -207 => 5,
        -206 => 4,
        -310 => 15,
        -320 | -324 => 8,
        _ => 16,
    }
}

// ===================================================================
// Zdarzenia z CEF (wątek UI CEF)
// ===================================================================

macro_rules! raise_async {
    ($wv:expr, $sender:expr, $ev:ident, $client:ident, $args:expr) => {{
        let args = $args;
        for h in $wv.ev(Ev::$ev).snapshot() {
            $client(h).Invoke_async(&mut Ctx, &$sender, &args);
        }
    }};
}

/// NavigationStarting — synchronicznie; zwraca true, gdy aplikacja anulowała nawigację.
pub fn on_navigation_starting(view: u32, id: u64, user_initiated: bool, redirected: bool, uri: &str) -> bool {
    with_webview(view, |wv, sender| {
        let handlers = wv.ev(Ev::NavigationStarting).snapshot();
        if handlers.is_empty() {
            return false;
        }
        let args = Rc::new(ICoreWebView2NavigationStartingEventArgs3Obj(NavigationStartingArgs {
            uri: uri.to_string(),
            user_initiated,
            redirected,
            id,
            cancel: Cell::new(false),
        }));
        let args_ref = ObjRef::Runtime(args.clone());
        for h in handlers {
            if let Err(e) = ICoreWebView2NavigationStartingEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
                eprintln!("helper: NavigationStarting handler failed: {e:#x}");
            }
        }
        args.0.cancel.get()
    })
    .unwrap_or(false)
}

pub fn on_source_changed(view: u32, url: &str) {
    with_webview(view, |wv, sender| {
        let new_document = *wv.source.borrow() != url;
        *wv.source.borrow_mut() = url.to_string();
        raise_async!(wv, sender, SourceChanged, ICoreWebView2SourceChangedEventHandlerApp,
                     rc(ICoreWebView2SourceChangedEventArgsObj(SourceChangedArgs { new_document })));
    });
}

pub fn on_content_loading(view: u32, id: u64) {
    with_webview(view, |wv, sender| {
        raise_async!(wv, sender, ContentLoading, ICoreWebView2ContentLoadingEventHandlerApp,
                     rc(ICoreWebView2ContentLoadingEventArgsObj(ContentLoadingArgs { id, error_page: false })));
    });
}

pub fn on_history_changed(view: u32, back: bool, forward: bool) {
    with_webview(view, |wv, sender| {
        wv.can_go_back.set(back);
        wv.can_go_forward.set(forward);
        raise_async!(wv, sender, HistoryChanged, ICoreWebView2HistoryChangedEventHandlerApp, ObjRef::Null);
    });
}

pub fn on_navigation_completed(view: u32, id: u64, success: bool, http_status: i32, net_error: i32) {
    with_webview(view, |wv, sender| {
        if success {
            raise_async!(wv, sender, DomContentLoaded, ICoreWebView2DOMContentLoadedEventHandlerApp,
                         rc(ICoreWebView2DOMContentLoadedEventArgsObj(DomContentLoadedArgs { id })));
        }
        let web_error = if success { 0 } else { web_error_status(net_error) };
        raise_async!(wv, sender, NavigationCompleted, ICoreWebView2NavigationCompletedEventHandlerApp,
                     rc(ICoreWebView2NavigationCompletedEventArgs2Obj(NavigationCompletedArgs { id, success, web_error, http_status })));
    });
}

pub fn on_title_changed(view: u32, title: &str) {
    with_webview(view, |wv, sender| {
        *wv.title.borrow_mut() = title.to_string();
        raise_async!(wv, sender, DocumentTitleChanged, ICoreWebView2DocumentTitleChangedEventHandlerApp, ObjRef::Null);
    });
}

pub fn on_web_message(view: u32, source: &str, json: &str) {
    with_webview(view, |wv, sender| {
        raise_async!(wv, sender, WebMessageReceived, ICoreWebView2WebMessageReceivedEventHandlerApp,
                     rc(ICoreWebView2WebMessageReceivedEventArgs2Obj(WebMessageArgs { source: source.into(), json: json.into() })));
    });
}

pub fn on_script_result(view: u32, id: u64, json: &str) {
    with_webview(view, |wv, _| {
        if let Some(app) = wv.pending_scripts.borrow_mut().remove(&id) {
            ICoreWebView2ExecuteScriptCompletedHandlerApp(app).Invoke_async(&mut Ctx, S_OK, Some(json));
        }
    });
}

pub fn on_devtools_result(view: u32, id: i32, success: bool, json: &str) {
    with_webview(view, |wv, _| {
        if let Some(app) = wv.pending_devtools.borrow_mut().remove(&id) {
            let hr = if success { S_OK } else { crate::rpc::E_FAIL };
            ICoreWebView2CallDevToolsProtocolMethodCompletedHandlerApp(app).Invoke_async(&mut Ctx, hr, Some(json));
        }
    });
}

pub fn on_devtools_event(view: u32, method: &str, params: &str) {
    with_webview(view, |wv, sender| {
        let receivers: Vec<_> = wv.devtools_receivers.borrow().iter().filter(|r| r.0.event == method).cloned().collect();
        for r in receivers {
            let args = rc(ICoreWebView2DevToolsProtocolEventReceivedEventArgs2Obj(DevToolsEventArgs { params: params.into() }));
            for h in r.0.handlers.snapshot() {
                ICoreWebView2DevToolsProtocolEventReceivedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &args);
            }
        }
    });
}

/// Handlery WebResourceRequested widoku i nadawca (None = brak WebView/handlerów).
pub fn resource_handlers(view: u32) -> Option<(Vec<AppRef>, ObjRef)> {
    with_webview(view, |wv, sender| (wv.ev(Ev::WebResourceRequested).snapshot(), sender))
        .filter(|(h, _)| !h.is_empty())
}

// ===================================================================
// Odroczenia (GetDeferral)
// ===================================================================

/// ICoreWebView2Deferral: Complete() wykonuje odłożoną akcję (raz).
pub struct Deferral {
    action: RefCell<Option<Box<dyn FnOnce()>>>,
}

impl ICoreWebView2DeferralImpl for Deferral {
    fn Complete(&self, _ctx: &mut Ctx) -> WvResult<()> {
        let action = self.action.borrow_mut().take();
        if let Some(action) = action {
            action();
        }
        Ok(())
    }
}

/// Stan odroczenia w argumentach zdarzenia: Taken = aplikacja wzięła odroczenie, a zdarzenie
/// jeszcze nie skończyło się wywoływać; Done = Complete() przyszło przed końcem wywołania.
#[derive(Clone, Copy, PartialEq)]
enum DeferralState {
    None,
    Taken,
    Done,
}

/// Wspólna obsługa GetDeferral: po zakończeniu handlerów `finish` wykonuje się od razu albo przy Complete().
struct DeferralSlot {
    state: Cell<DeferralState>,
    finish: RefCell<Option<Box<dyn FnOnce()>>>,
}

impl DeferralSlot {
    fn new() -> Rc<Self> {
        Rc::new(DeferralSlot { state: Cell::new(DeferralState::None), finish: RefCell::new(None) })
    }

    fn get(self: &Rc<Self>) -> ObjRef {
        self.state.set(DeferralState::Taken);
        let slot = self.clone();
        rc(ICoreWebView2DeferralObj(Deferral {
            action: RefCell::new(Some(Box::new(move || {
                let finish = slot.finish.borrow_mut().take();
                match finish {
                    Some(f) => f(),
                    None => slot.state.set(DeferralState::Done),
                }
            }))),
        }))
    }

    /// Po wywołaniu handlerów: true = wynik gotowy teraz; false = `finish` poczeka na Complete().
    fn settle(&self, finish: Box<dyn FnOnce()>) -> bool {
        if self.state.get() == DeferralState::Taken {
            *self.finish.borrow_mut() = Some(finish);
            false
        } else {
            true
        }
    }
}

// ===================================================================
// NewWindowRequested / WindowCloseRequested
// ===================================================================

pub struct WindowFeatures {
    pub position: Option<(u32, u32)>,
    pub size: Option<(u32, u32)>,
}

impl ICoreWebView2WindowFeaturesImpl for WindowFeatures {
    fn get_HasPosition(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.position.is_some())
    }
    fn get_HasSize(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.size.is_some())
    }
    fn get_Left(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.position.map_or(0, |p| p.0))
    }
    fn get_Top(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.position.map_or(0, |p| p.1))
    }
    fn get_Width(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.size.map_or(0, |s| s.0))
    }
    fn get_Height(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.size.map_or(0, |s| s.1))
    }
    // Okno popup w Chromium: bez paska menu, statusu i narzędzi; paski przewijania tak.
    fn get_ShouldDisplayMenuBar(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(false)
    }
    fn get_ShouldDisplayStatus(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(false)
    }
    fn get_ShouldDisplayToolbar(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(false)
    }
    fn get_ShouldDisplayScrollBars(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(true)
    }
}

pub struct NewWindowArgs {
    uri: String,
    name: String,
    user_initiated: bool,
    features: ObjRef,
    size: Option<(u32, u32)>,
    handled: Cell<bool>,
    new_window: RefCell<ObjRef>,
    deferral: Rc<DeferralSlot>,
}

impl ICoreWebView2NewWindowRequestedEventArgs3Impl for NewWindowArgs {
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.clone())
    }
    fn put_NewWindow(&self, _ctx: &mut Ctx, new_window: ObjRef) -> WvResult<()> {
        // Musi to być nasz ICoreWebView2 (z innego kontrolera tego środowiska).
        if !new_window.is_null() && webview_of(&new_window).is_none() {
            return Err(E_INVALIDARG);
        }
        // Ustawienie NewWindow oznacza też Handled (dokumentacja WebView2).
        self.handled.set(true);
        *self.new_window.borrow_mut() = new_window;
        Ok(())
    }
    fn get_NewWindow(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.new_window.borrow().clone())
    }
    fn put_Handled(&self, _ctx: &mut Ctx, handled: bool) -> WvResult<()> {
        self.handled.set(handled);
        Ok(())
    }
    fn get_Handled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.handled.get())
    }
    fn get_IsUserInitiated(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.user_initiated)
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.deferral.get())
    }
    fn get_WindowFeatures(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.features.clone())
    }
    fn get_Name(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.name.clone())
    }
}

/// Widok WebView, jeśli obiekt jest naszym ICoreWebView2.
fn webview_of(obj: &ObjRef) -> Option<u32> {
    match obj {
        ObjRef::Runtime(o) => o.as_any().downcast_ref::<WebViewObj>().map(|w| w.0.view),
        _ => None,
    }
}

/// Co zrobić z popupem CEF.
pub enum PopupDecision {
    /// Domyślne okno (natywne okno CEF, z window.opener).
    Default,
    /// Aplikacja obsłużyła (Handled bez NewWindow): popup anulowany.
    Cancel,
    /// NewWindow: popup trafia do WebView tego widoku.
    Adopt(u32),
    /// Aplikacja wzięła odroczenie: decyzja przyjdzie przez `crate::popup_decided`.
    Deferred,
}

fn decide_new_window(args: &NewWindowArgs) -> PopupDecision {
    if let Some(target) = webview_of(&args.new_window.borrow()) {
        PopupDecision::Adopt(target)
    } else if args.handled.get() {
        PopupDecision::Cancel
    } else {
        PopupDecision::Default
    }
}

/// NewWindowRequested — synchronicznie z OnBeforePopup. `popup` = numer widoku, pod którym
/// przeglądarka popupu czeka na decyzję (przy odroczeniu).
pub fn on_new_window(view: u32, popup: u32, uri: &str, name: &str, user_initiated: bool,
                     features: WindowFeatures) -> PopupDecision {
    with_webview(view, |wv, sender| {
        let handlers = wv.ev(Ev::NewWindowRequested).snapshot();
        if handlers.is_empty() {
            return PopupDecision::Default;
        }
        let args = Rc::new(ICoreWebView2NewWindowRequestedEventArgs3Obj(NewWindowArgs {
            uri: uri.to_string(),
            name: name.to_string(),
            user_initiated,
            size: features.size,
            features: rc(ICoreWebView2WindowFeaturesObj(features)),
            handled: Cell::new(false),
            new_window: RefCell::new(ObjRef::Null),
            deferral: DeferralSlot::new(),
        }));
        let args_ref = ObjRef::Runtime(args.clone());
        for h in handlers {
            if let Err(e) = ICoreWebView2NewWindowRequestedEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
                eprintln!("helper: NewWindowRequested handler failed: {e:#x}");
            }
        }
        let later = args.clone();
        if args.0.deferral.settle(Box::new(move || {
            crate::popup_decided(popup, decide_new_window(&later.0), &later.0.uri, later.0.size);
        })) {
            decide_new_window(&args.0)
        } else {
            PopupDecision::Deferred
        }
    })
    .unwrap_or(PopupDecision::Default)
}

/// window.close() w dokumencie głównym.
pub fn on_window_close(view: u32) {
    with_webview(view, |wv, sender| {
        raise_async!(wv, sender, WindowCloseRequested, ICoreWebView2WindowCloseRequestedEventHandlerApp, ObjRef::Null);
    });
}

// ===================================================================
// ScriptDialogOpening
// ===================================================================

pub struct ScriptDialogArgs {
    uri: String,
    kind: i32,
    message: String,
    default_text: String,
    result_text: RefCell<String>,
    accepted: Cell<bool>,
    deferral: Rc<DeferralSlot>,
}

impl ICoreWebView2ScriptDialogOpeningEventArgsImpl for ScriptDialogArgs {
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.clone())
    }
    fn get_Kind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.kind)
    }
    fn get_Message(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.message.clone())
    }
    fn Accept(&self, _ctx: &mut Ctx) -> WvResult<()> {
        self.accepted.set(true);
        Ok(())
    }
    fn get_DefaultText(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.default_text.clone())
    }
    fn get_ResultText(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.result_text.borrow().clone())
    }
    fn put_ResultText(&self, _ctx: &mut Ctx, text: Option<String>) -> WvResult<()> {
        *self.result_text.borrow_mut() = text.unwrap_or_default();
        Ok(())
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.deferral.get())
    }
}

/// Wynik dialogu JS: (zaakceptowany, tekst prompt).
pub type DialogDone = Box<dyn FnOnce(bool, &str)>;

/// Dialog JS przy AreDefaultScriptDialogsEnabled = FALSE. Bez handlerów dialog zamyka się
/// jak anulowany (jak w WebView2). `done` może przyjść później (GetDeferral).
pub fn on_script_dialog(view: u32, uri: &str, kind: i32, message: &str, default_text: &str, done: DialogDone) {
    let done = Rc::new(RefCell::new(Some(done)));
    let deferred = with_webview(view, |wv, sender| {
        let handlers = wv.ev(Ev::ScriptDialogOpening).snapshot();
        if handlers.is_empty() {
            return false;
        }
        let args = Rc::new(ICoreWebView2ScriptDialogOpeningEventArgsObj(ScriptDialogArgs {
            uri: uri.to_string(),
            kind,
            message: message.to_string(),
            default_text: default_text.to_string(),
            result_text: RefCell::new(String::new()),
            accepted: Cell::new(false),
            deferral: DeferralSlot::new(),
        }));
        let args_ref = ObjRef::Runtime(args.clone());
        for h in handlers {
            if let Err(e) = ICoreWebView2ScriptDialogOpeningEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
                eprintln!("helper: ScriptDialogOpening handler failed: {e:#x}");
            }
        }
        let finish = {
            let (args, done) = (args.clone(), done.clone());
            move || {
                if let Some(d) = done.borrow_mut().take() {
                    d(args.0.accepted.get(), &args.0.result_text.borrow());
                }
            }
        };
        if args.0.deferral.settle(Box::new(finish.clone())) {
            finish();
            return false;
        }
        true
    })
    .unwrap_or(false);
    if !deferred {
        // Brak WebView albo handlerów (po handlerach `done` jest już wzięte).
        if let Some(d) = done.borrow_mut().take() {
            d(false, "");
        }
    }
}
