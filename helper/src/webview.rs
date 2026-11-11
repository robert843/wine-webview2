//! Environment, controller and ICoreWebView2 as server-side objects (architecture v8).
//!
//! CEF events (main.rs, resources.rs) arrive through the `on_*` functions and are delivered to
//! app handlers via INVOKE - asynchronously, except NavigationStarting and WebResourceRequested,
//! which need the app's decision.

use crate::api_gen::*;
use crate::rpc::{AppRef, Ctx, Dispatch, ObjRef, WvResult, E_INVALIDARG, E_INVALID_STATE, S_OK};
use std::cell::{Cell, RefCell};
use std::collections::HashMap;
use std::rc::Rc;

pub const VERSION: &str = "156.0.9999.0";

// ===================================================================
// Event registrations
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
    /// Copy, since handlers may (un)register themselves during the call.
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
    ContainsFullScreenElementChanged,
    DownloadStarting,
    PermissionRequested,
    StatusBarTextChanged,
    FaviconChanged,
    IsMutedChanged,
    IsDocumentPlayingAudioChanged,
}
const EV_COUNT: usize = 19;

fn rc<T: Dispatch + 'static>(obj: T) -> ObjRef {
    ObjRef::Runtime(Rc::new(obj))
}

// ===================================================================
// Environment
// ===================================================================

pub struct Environment {
    pub user_data_dir: String,
}

thread_local! {
    /// User data folder of the (last created) environment, for profile paths.
    static USER_DATA_DIR: RefCell<String> = RefCell::new(String::new());
}

thread_local! {
    /// Default download folder (Windows path) of the profile; the DLL sends the user's Downloads.
    static DOWNLOAD_FOLDER: RefCell<String> = RefCell::new(String::new());
}

pub fn set_default_download_folder(dir: &str) {
    if !dir.is_empty() {
        DOWNLOAD_FOLDER.with(|d| *d.borrow_mut() = dir.to_string());
    }
}

pub fn set_user_data_dir(dir: &str) {
    USER_DATA_DIR.with(|d| *d.borrow_mut() = dir.to_string());
}

impl ICoreWebView2Environment15Impl for Environment {
    fn get_BrowserVersionString(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(VERSION.to_string())
    }

    fn get_UserDataFolder(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.user_data_dir.clone())
    }

    fn CreateCoreWebView2ControllerOptions(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(rc(ICoreWebView2ControllerOptions4Obj(ControllerOptions::default())))
    }

    fn CreateWebResourceResponse(&self, _ctx: &mut Ctx, content: Option<Vec<u8>>, status: i32,
                                 reason: Option<String>, headers: Option<String>) -> WvResult<ObjRef> {
        Ok(crate::webresource::create_response(content, status, reason.unwrap_or_default(), headers.as_deref()))
    }
}

// ===================================================================
// Controller
// ===================================================================

/// Window, size, visibility, focus and background are handled by the DLL host (vtable overrides).
pub struct Controller {
    pub view: u32,
    pub webview: ObjRef,
    got_focus: EventList,
    lost_focus: EventList,
    move_focus: EventList,
    zoom_changed: EventList,
    scale_changed: EventList,
    zoom: Cell<f64>,
    scale: Cell<f64>,
    detect_scale: Cell<bool>,
    bounds_mode: Cell<i32>,
    allow_drop: Cell<bool>,
}

pub type ControllerObj = ICoreWebView2Controller4Obj<Controller>;

thread_local! {
    /// View -> controller (for focus events from the host and CEF).
    static CONTROLLERS: RefCell<HashMap<u32, Rc<ControllerObj>>> = RefCell::new(HashMap::new());
}

fn with_controller<R>(view: u32, f: impl FnOnce(&Controller, ObjRef) -> R) -> Option<R> {
    let c = CONTROLLERS.with(|c| c.borrow().get(&view).cloned())?;
    let sender = ObjRef::Runtime(c.clone());
    Some(f(&c.0, sender))
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

    fn add_GotFocus(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.got_focus.add(handler)
    }
    fn remove_GotFocus(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.got_focus.remove(token)
    }
    fn add_LostFocus(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.lost_focus.add(handler)
    }
    fn remove_LostFocus(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.lost_focus.remove(token)
    }
    fn add_MoveFocusRequested(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.move_focus.add(handler)
    }
    fn remove_MoveFocusRequested(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.move_focus.remove(token)
    }

    fn get_ZoomFactor(&self, _ctx: &mut Ctx) -> WvResult<f64> {
        Ok(self.zoom.get())
    }
    fn put_ZoomFactor(&self, ctx: &mut Ctx, zoom: f64) -> WvResult<()> {
        if !(zoom > 0.0) {
            return Err(E_INVALIDARG);
        }
        if (zoom - self.zoom.get()).abs() < 1e-9 {
            return Ok(());
        }
        self.zoom.set(zoom);
        crate::set_zoom(self.view, zoom);
        let sender = CONTROLLERS.with(|c| c.borrow().get(&self.view).cloned()).map(|c| ObjRef::Runtime(c)).unwrap_or_default();
        for h in self.zoom_changed.snapshot() {
            ICoreWebView2ZoomFactorChangedEventHandlerApp(h).Invoke_async(ctx, &sender, &ObjRef::Null);
        }
        Ok(())
    }
    fn add_ZoomFactorChanged(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.zoom_changed.add(handler)
    }
    fn remove_ZoomFactorChanged(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.zoom_changed.remove(token)
    }

    // FIXME: DPI scaling - the value is kept but the page is not scaled yet.
    fn get_RasterizationScale(&self, _ctx: &mut Ctx) -> WvResult<f64> {
        Ok(self.scale.get())
    }
    fn put_RasterizationScale(&self, ctx: &mut Ctx, scale: f64) -> WvResult<()> {
        if !(scale > 0.0) {
            return Err(E_INVALIDARG);
        }
        if (scale - self.scale.get()).abs() < 1e-9 {
            return Ok(());
        }
        self.scale.set(scale);
        let sender = CONTROLLERS.with(|c| c.borrow().get(&self.view).cloned()).map(|c| ObjRef::Runtime(c)).unwrap_or_default();
        for h in self.scale_changed.snapshot() {
            ICoreWebView2RasterizationScaleChangedEventHandlerApp(h).Invoke_async(ctx, &sender, &ObjRef::Null);
        }
        Ok(())
    }
    fn add_RasterizationScaleChanged(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.scale_changed.add(handler)
    }
    fn remove_RasterizationScaleChanged(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.scale_changed.remove(token)
    }
    fn get_ShouldDetectMonitorScaleChanges(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.detect_scale.get())
    }
    fn put_ShouldDetectMonitorScaleChanges(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.detect_scale.set(value);
        Ok(())
    }
    fn get_BoundsMode(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.bounds_mode.get())
    }
    fn put_BoundsMode(&self, _ctx: &mut Ctx, value: i32) -> WvResult<()> {
        self.bounds_mode.set(value);
        Ok(())
    }
    // FIXME: drag and drop into the page is not implemented; the setting is only kept.
    fn get_AllowExternalDrop(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.allow_drop.get())
    }
    fn put_AllowExternalDrop(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.allow_drop.set(value);
        Ok(())
    }
}

/// The host window gained or lost focus (CMD_FOCUS from the DLL).
pub fn on_focus_changed(view: u32, focused: bool) {
    with_controller(view, |c, sender| {
        let list = if focused { &c.got_focus } else { &c.lost_focus };
        for h in list.snapshot() {
            ICoreWebView2FocusChangedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &ObjRef::Null);
        }
    });
}

pub struct MoveFocusArgs {
    reason: i32,
    handled: Cell<bool>,
}

impl ICoreWebView2MoveFocusRequestedEventArgsImpl for MoveFocusArgs {
    fn get_Reason(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.reason)
    }
    fn get_Handled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.handled.get())
    }
    fn put_Handled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.handled.set(value);
        Ok(())
    }
}

/// Tab moved focus out of the page (CEF OnTakeFocus): COREWEBVIEW2_MOVE_FOCUS_REASON_NEXT/PREVIOUS.
pub fn on_move_focus_requested(view: u32, next: bool) {
    with_controller(view, |c, sender| {
        let args = rc(ICoreWebView2MoveFocusRequestedEventArgsObj(MoveFocusArgs {
            reason: if next { 1 } else { 2 },
            handled: Cell::new(false),
        }));
        for h in c.move_focus.snapshot() {
            ICoreWebView2MoveFocusRequestedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &args);
        }
    });
}

// ===================================================================
// Controller options and profile
// ===================================================================

/// ICoreWebView2ControllerOptions. FIXME: the DLL host does not apply them to the new controller yet
/// (profile name, InPrivate, script locale, background colour).
pub struct ControllerOptions {
    profile_name: RefCell<String>,
    in_private: Cell<bool>,
    script_locale: RefCell<String>,
    background: Cell<COREWEBVIEW2_COLOR>,
    allow_host_input: Cell<bool>,
}

impl Default for ControllerOptions {
    fn default() -> Self {
        ControllerOptions {
            profile_name: RefCell::new(String::new()),
            in_private: Cell::new(false),
            script_locale: RefCell::new(String::new()),
            background: Cell::new(COREWEBVIEW2_COLOR { A: 255, R: 255, G: 255, B: 255 }),
            allow_host_input: Cell::new(true),
        }
    }
}

impl ICoreWebView2ControllerOptions4Impl for ControllerOptions {
    fn get_ProfileName(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.profile_name.borrow().clone())
    }
    fn put_ProfileName(&self, _ctx: &mut Ctx, value: Option<String>) -> WvResult<()> {
        // WebView2: up to 64 letters, digits and #@$()+-_~. or spaces, not ending with '.' or ' '.
        let value = value.unwrap_or_default();
        let valid_char = |c: char| c.is_alphanumeric() || "#@$()+-_~. ".contains(c);
        if value.chars().count() > 64 || !value.chars().all(valid_char) || value.ends_with(['.', ' ']) {
            return Err(E_INVALIDARG);
        }
        *self.profile_name.borrow_mut() = value;
        Ok(())
    }
    fn get_IsInPrivateModeEnabled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.in_private.get())
    }
    fn put_IsInPrivateModeEnabled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.in_private.set(value);
        Ok(())
    }
    fn get_ScriptLocale(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.script_locale.borrow().clone())
    }
    fn put_ScriptLocale(&self, _ctx: &mut Ctx, value: Option<String>) -> WvResult<()> {
        *self.script_locale.borrow_mut() = value.unwrap_or_default();
        Ok(())
    }
    fn get_DefaultBackgroundColor(&self, _ctx: &mut Ctx) -> WvResult<COREWEBVIEW2_COLOR> {
        Ok(self.background.get())
    }
    fn put_DefaultBackgroundColor(&self, _ctx: &mut Ctx, value: COREWEBVIEW2_COLOR) -> WvResult<()> {
        self.background.set(value);
        Ok(())
    }
    fn get_AllowHostInputProcessing(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.allow_host_input.get())
    }
    fn put_AllowHostInputProcessing(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.allow_host_input.set(value);
        Ok(())
    }
}

/// ICoreWebView2Profile of a view (the default profile; named and InPrivate profiles: FIXME).
pub struct Profile {
    view: u32,
    name: String,
    in_private: bool,
    color_scheme: Cell<i32>,
    tracking_level: Cell<i32>,
    password_autosave: Cell<bool>,
    general_autofill: Cell<bool>,
    worker_script_apis: Cell<bool>,
}

impl ICoreWebView2Profile9Impl for Profile {
    fn get_ProfileName(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.name.clone())
    }
    fn get_IsInPrivateModeEnabled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.in_private)
    }
    fn get_ProfilePath(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(USER_DATA_DIR.with(|d| format!("{}\\EBWebView\\{}", d.borrow().trim_end_matches('\\'), self.name)))
    }
    fn get_DefaultDownloadFolderPath(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(DOWNLOAD_FOLDER.with(|d| d.borrow().clone()))
    }
    fn put_DefaultDownloadFolderPath(&self, _ctx: &mut Ctx, value: Option<String>) -> WvResult<()> {
        let value = value.ok_or(E_INVALIDARG)?;
        if crate::paths::to_unix(&value).is_none() {
            return Err(E_INVALIDARG);
        }
        DOWNLOAD_FOLDER.with(|d| *d.borrow_mut() = value);
        Ok(())
    }
    fn get_PreferredColorScheme(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.color_scheme.get())
    }
    /// COREWEBVIEW2_PREFERRED_COLOR_SCHEME: 0 auto, 1 light, 2 dark (CSS prefers-color-scheme).
    fn put_PreferredColorScheme(&self, _ctx: &mut Ctx, value: i32) -> WvResult<()> {
        if !(0..=2).contains(&value) {
            return Err(E_INVALIDARG);
        }
        self.color_scheme.set(value);
        crate::set_color_scheme(self.view, value);
        Ok(())
    }
    fn ClearBrowsingDataAll(&self, ctx: &mut Ctx, handler: ObjRef) -> WvResult<()> {
        crate::clear_browsing_data(self.view);
        if let Some(app) = handler.app() {
            ICoreWebView2ClearBrowsingDataCompletedHandlerApp(app.clone()).Invoke_async(ctx, S_OK);
        }
        Ok(())
    }
    // FIXME: data kinds and time ranges are not distinguished: cookies and cache are cleared.
    fn ClearBrowsingData(&self, ctx: &mut Ctx, _kinds: i32, handler: ObjRef) -> WvResult<()> {
        self.ClearBrowsingDataAll(ctx, handler)
    }
    fn ClearBrowsingDataInTimeRange(&self, ctx: &mut Ctx, _kinds: i32, _start: f64, _end: f64, handler: ObjRef) -> WvResult<()> {
        self.ClearBrowsingDataAll(ctx, handler)
    }
    fn get_PreferredTrackingPreventionLevel(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.tracking_level.get())
    }
    fn put_PreferredTrackingPreventionLevel(&self, _ctx: &mut Ctx, value: i32) -> WvResult<()> {
        self.tracking_level.set(value);
        Ok(())
    }
    fn get_IsPasswordAutosaveEnabled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.password_autosave.get())
    }
    fn put_IsPasswordAutosaveEnabled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.password_autosave.set(value);
        Ok(())
    }
    fn get_IsGeneralAutofillEnabled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.general_autofill.get())
    }
    fn put_IsGeneralAutofillEnabled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.general_autofill.set(value);
        Ok(())
    }
    fn get_AreWebViewScriptApisEnabledForServiceWorkers(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.worker_script_apis.get())
    }
    fn put_AreWebViewScriptApisEnabledForServiceWorkers(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.worker_script_apis.set(value);
        Ok(())
    }
}

pub fn close_view(view: u32) {
    WEBVIEWS.with(|w| w.borrow_mut().remove(&view));
    CONTROLLERS.with(|c| c.borrow_mut().remove(&view));
    crate::resources::remove_view(view);
    crate::close_browser(view);
}

// ===================================================================
// ICoreWebView2
// ===================================================================

pub type WebViewObj = ICoreWebView2_29Obj<WebView>;

thread_local! {
    /// View -> WebView, for routing CEF events.
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
    pending_scripts: RefCell<HashMap<u64, PendingScript>>,
    pending_devtools: RefCell<HashMap<i32, AppRef>>,
    devtools_receivers: RefCell<Vec<Rc<ICoreWebView2DevToolsProtocolEventReceiverObj<DevToolsReceiver>>>>,
    next_id: Cell<u64>,
    doc_scripts: RefCell<Vec<(u64, String)>>,
    profile: RefCell<Option<ObjRef>>,
    profile_name: String,
    in_private: bool,
    fullscreen: Cell<bool>,
    status_text: RefCell<String>,
    favicon: RefCell<String>,
    muted: Cell<bool>,
}

impl WebView {
    fn new(view: u32, profile_name: String, in_private: bool) -> Self {
        WebView {
            profile_name,
            in_private,
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
            profile: RefCell::new(None),
            fullscreen: Cell::new(false),
            status_text: RefCell::new(String::new()),
            favicon: RefCell::new(String::new()),
            muted: Cell::new(false),
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

pub fn create_controller(view: u32, width: u32, height: u32, options: &ObjRef) -> WvResult<ObjRef> {
    let (mut profile, in_private) = options
        .downcast::<ICoreWebView2ControllerOptions4Obj<ControllerOptions>>()
        .map(|o| (o.0.profile_name.borrow().clone(), o.0.in_private.get()))
        .unwrap_or_default();
    if profile.is_empty() {
        profile = "Default".into();
    }
    crate::create_browser(view, width, height, &profile, in_private)?;
    let webview = Rc::new(ICoreWebView2_29Obj(WebView::new(view, profile, in_private)));
    WEBVIEWS.with(|w| w.borrow_mut().insert(view, webview.clone()));
    let webview_ref = ObjRef::Runtime(webview);
    let controller = Rc::new(ICoreWebView2Controller4Obj(Controller {
        view,
        webview: webview_ref,
        got_focus: EventList::default(),
        lost_focus: EventList::default(),
        move_focus: EventList::default(),
        zoom_changed: EventList::default(),
        scale_changed: EventList::default(),
        zoom: Cell::new(1.0),
        scale: Cell::new(1.0),
        detect_scale: Cell::new(true),
        bounds_mode: Cell::new(0),
        allow_drop: Cell::new(true),
    }));
    CONTROLLERS.with(|c| c.borrow_mut().insert(view, controller.clone()));
    Ok(ObjRef::Runtime(controller))
}

fn webview_ref(view: u32) -> ObjRef {
    WEBVIEWS.with(|w| w.borrow().get(&view).cloned()).map(|wv| ObjRef::Runtime(wv)).unwrap_or(ObjRef::Null)
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

pub enum PendingScript {
    Plain(AppRef),
    WithResult(AppRef),
}

/// JS exception thrown by the script (ExecuteScriptWithResult).
pub struct ScriptError {
    pub name: String,
    pub message: String,
    pub line: u32,
    pub column: u32,
    pub json: String,
}

impl PendingScript {
    /// ExecuteScript reports "null" even after an exception (as WebView2); WithResult gets details.
    fn complete(self, ctx: &mut Ctx, result: Result<String, ScriptError>) {
        match self {
            PendingScript::Plain(app) => {
                let json = result.unwrap_or_else(|_| "null".to_string());
                ICoreWebView2ExecuteScriptCompletedHandlerApp(app).Invoke_async(ctx, S_OK, Some(&json));
            }
            PendingScript::WithResult(app) => {
                let obj = rc(ICoreWebView2ExecuteScriptResultObj(ScriptResult { result }));
                ICoreWebView2ExecuteScriptWithResultCompletedHandlerApp(app).Invoke_async(ctx, S_OK, &obj);
            }
        }
    }
}

impl WebView {
    fn run_script(&self, ctx: &mut Ctx, script: String, pending: Option<PendingScript>) -> WvResult<()> {
        let id = self.id();
        if let Some(p) = pending {
            self.pending_scripts.borrow_mut().insert(id, p);
        }
        if !crate::execute_script(self.view, id, &script) {
            // No renderer yet (e.g. before the first navigation): complete as a script with no result.
            if let Some(p) = self.pending_scripts.borrow_mut().remove(&id) {
                p.complete(ctx, Ok("null".to_string()));
            }
        }
        Ok(())
    }
}

pub struct ScriptResult {
    result: Result<String, ScriptError>,
}

impl ICoreWebView2ExecuteScriptResultImpl for ScriptResult {
    fn get_Succeeded(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.result.is_ok())
    }
    fn get_ResultAsJson(&self, _ctx: &mut Ctx) -> WvResult<String> {
        // Empty string after an exception, per WebView2 docs.
        Ok(self.result.as_ref().map(|j| j.clone()).unwrap_or_default())
    }
    fn TryGetResultAsString(&self, _ctx: &mut Ctx) -> WvResult<(String, bool)> {
        match self.result.as_ref().ok().and_then(|j| json_unquote(j)) {
            Some(s) => Ok((s, true)),
            None => Ok((String::new(), false)),
        }
    }
    fn get_Exception(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(match &self.result {
            Ok(_) => ObjRef::Null,
            Err(e) => rc(ICoreWebView2ScriptExceptionObj(ScriptException {
                name: e.name.clone(),
                message: e.message.clone(),
                line: e.line,
                column: e.column,
                json: e.json.clone(),
            })),
        })
    }
}

pub struct ScriptException {
    name: String,
    message: String,
    line: u32,
    column: u32,
    json: String,
}

impl ICoreWebView2ScriptExceptionImpl for ScriptException {
    fn get_LineNumber(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.line)
    }
    fn get_ColumnNumber(&self, _ctx: &mut Ctx) -> WvResult<u32> {
        Ok(self.column)
    }
    fn get_Name(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.name.clone())
    }
    fn get_Message(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.message.clone())
    }
    fn get_ToJson(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.json.clone())
    }
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
        add_ContainsFullScreenElementChanged, remove_ContainsFullScreenElementChanged, ContainsFullScreenElementChanged;
        add_DownloadStarting, remove_DownloadStarting, DownloadStarting;
        add_PermissionRequested, remove_PermissionRequested, PermissionRequested;
        add_StatusBarTextChanged, remove_StatusBarTextChanged, StatusBarTextChanged;
        add_FaviconChanged, remove_FaviconChanged, FaviconChanged;
        add_IsMutedChanged, remove_IsMutedChanged, IsMutedChanged;
        add_IsDocumentPlayingAudioChanged, remove_IsDocumentPlayingAudioChanged, IsDocumentPlayingAudioChanged;
    }

    fn get_StatusBarText(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.status_text.borrow().clone())
    }

    fn get_FaviconUri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.favicon.borrow().clone())
    }

    fn GetFavicon(&self, ctx: &mut Ctx, format: i32, handler: ObjRef) -> WvResult<()> {
        let app = handler.app().cloned().ok_or(E_INVALIDARG)?;
        let _ = ctx;
        // COREWEBVIEW2_FAVICON_IMAGE_FORMAT: 0 = PNG, 1 = JPEG
        crate::get_favicon(self.view, &self.favicon.borrow(), format != 1, Box::new(move |bytes| {
            ICoreWebView2GetFaviconCompletedHandlerApp(app).Invoke_async(&mut Ctx, S_OK, bytes.as_deref());
        }));
        Ok(())
    }

    fn get_IsMuted(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.muted.get())
    }

    fn put_IsMuted(&self, ctx: &mut Ctx, muted: bool) -> WvResult<()> {
        if self.muted.replace(muted) != muted {
            crate::set_audio_muted(self.view, muted);
            let sender = webview_ref(self.view);
            for h in self.ev(Ev::IsMutedChanged).snapshot() {
                ICoreWebView2IsMutedChangedEventHandlerApp(h).Invoke_async(ctx, &sender, &ObjRef::Null);
            }
        }
        Ok(())
    }

    // FIXME: CEF only reports audio through an AudioHandler that captures the stream instead of playing it.
    fn get_IsDocumentPlayingAudio(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(false)
    }

    fn get_ContainsFullScreenElement(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.fullscreen.get())
    }

    fn get_Profile(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        let mut p = self.profile.borrow_mut();
        Ok(p.get_or_insert_with(|| {
            rc(ICoreWebView2Profile9Obj(Profile {
                view: self.view,
                name: self.profile_name.clone(),
                in_private: self.in_private,
                color_scheme: Cell::new(0),
                tracking_level: Cell::new(2), // BALANCED
                password_autosave: Cell::new(false),
                general_autofill: Cell::new(true),
                worker_script_apis: Cell::new(false),
            }))
        })
        .clone())
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
        self.run_script(ctx, script, handler.app().cloned().map(PendingScript::Plain))
    }

    fn ExecuteScriptWithResult(&self, ctx: &mut Ctx, script: Option<String>, handler: ObjRef) -> WvResult<()> {
        let script = script.ok_or(E_INVALIDARG)?;
        self.run_script(ctx, script, handler.app().cloned().map(PendingScript::WithResult))
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

    /// folderPath is already a unix path (converted by the DLL host).
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
// Event args
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
    /// Succeeds only if the page posted a string.
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
// Events from CEF (CEF UI thread)
// ===================================================================

macro_rules! raise_async {
    ($wv:expr, $sender:expr, $ev:ident, $client:ident, $args:expr) => {{
        let args = $args;
        for h in $wv.ev(Ev::$ev).snapshot() {
            $client(h).Invoke_async(&mut Ctx, &$sender, &args);
        }
    }};
}

/// Synchronous; returns true if the app cancelled the navigation.
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

pub fn on_script_result(view: u32, id: u64, result: Result<String, ScriptError>) {
    with_webview(view, |wv, _| {
        let pending = wv.pending_scripts.borrow_mut().remove(&id);
        if let Some(p) = pending {
            p.complete(&mut Ctx, result);
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

/// WebResourceRequested handlers of the view and the sender (None = no WebView or no handlers).
pub fn resource_handlers(view: u32) -> Option<(Vec<AppRef>, ObjRef)> {
    with_webview(view, |wv, sender| (wv.ev(Ev::WebResourceRequested).snapshot(), sender))
        .filter(|(h, _)| !h.is_empty())
}

// ===================================================================
// Deferrals (GetDeferral)
// ===================================================================

/// Complete() runs the deferred action once.
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

/// Taken = the app took a deferral while handlers are still running;
/// Done = Complete() arrived before the handlers returned.
#[derive(Clone, Copy, PartialEq)]
enum DeferralState {
    None,
    Taken,
    Done,
}

/// Shared GetDeferral logic: after the handlers return, `finish` runs immediately or on Complete().
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

    /// After the handlers: true = result ready now; false = `finish` waits for Complete().
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
    // Chromium popups have no menu bar, status bar or toolbar, but do have scroll bars.
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
        // Must be one of our ICoreWebView2 objects (from another controller of this environment).
        if !new_window.is_null() && webview_of(&new_window).is_none() {
            return Err(E_INVALIDARG);
        }
        // Setting NewWindow also implies Handled, per WebView2 docs.
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

/// View number if the object is one of our ICoreWebView2.
fn webview_of(obj: &ObjRef) -> Option<u32> {
    match obj {
        ObjRef::Runtime(o) => o.as_any().downcast_ref::<WebViewObj>().map(|w| w.0.view),
        _ => None,
    }
}

pub enum PopupDecision {
    /// Default window (native CEF window, keeps window.opener).
    Default,
    /// Handled without NewWindow: the popup is cancelled.
    Cancel,
    /// NewWindow set: the popup goes into the WebView of this view.
    Adopt(u32),
    /// App took a deferral: the decision arrives via `crate::popup_decided`.
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

/// Synchronous from OnBeforePopup. `popup` is the view number under which the popup browser
/// waits for the decision (when deferred).
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

/// window.close() in the main document.
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

/// JS dialog result: (accepted, prompt text).
pub type DialogDone = Box<dyn FnOnce(bool, &str)>;

/// JS dialog when AreDefaultScriptDialogsEnabled = FALSE. Without handlers it closes as cancelled,
/// as in WebView2. `done` may be called later (GetDeferral).
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
        // No WebView or no handlers (if handlers ran, `done` was already taken).
        if let Some(d) = done.borrow_mut().take() {
            d(false, "");
        }
    }
}

/// The page entered or left full screen (CEF OnFullscreenModeChange).
pub fn on_fullscreen_changed(view: u32, fullscreen: bool) {
    with_webview(view, |wv, sender| {
        if wv.fullscreen.replace(fullscreen) != fullscreen {
            raise_async!(wv, sender, ContainsFullScreenElementChanged,
                         ICoreWebView2ContainsFullScreenElementChangedEventHandlerApp, ObjRef::Null);
        }
    });
}

// ===================================================================
// Downloads (DownloadStarting, DownloadOperation)
// ===================================================================

pub struct DownloadInfo {
    pub id: u32,
    pub uri: String,
    pub mime: String,
    pub disposition: String,
    pub total: i64,
    pub suggested: String,
}

#[derive(Clone, Copy, PartialEq)]
pub enum DownloadState {
    InProgress,
    Paused,
    Completed,
    /// CEF interrupt reason (Chromium numbering).
    Interrupted(u32),
}

pub struct DownloadUpdate {
    pub state: DownloadState,
    pub received: i64,
    pub total: i64,
    /// Bytes per second.
    pub speed: i64,
    pub full_path: String,
}

const DOWNLOAD_STATE_IN_PROGRESS: i32 = 0;
const DOWNLOAD_STATE_INTERRUPTED: i32 = 1;
const DOWNLOAD_STATE_COMPLETED: i32 = 2;
const INTERRUPT_REASON_USER_PAUSED: i32 = 28;

/// Chromium's DownloadInterruptReason -> COREWEBVIEW2_DOWNLOAD_INTERRUPT_REASON.
fn interrupt_reason(cef: u32) -> i32 {
    match cef {
        0 => 0,
        1 => 1,
        2 => 2,
        3 => 3,
        5 => 4,
        6 => 5,
        7 => 6,
        10 => 7,
        11 => 8,
        12 => 9,
        13 => 10,
        14 => 11,
        15 => 1,
        20..=24 => cef as i32 - 8, // NETWORK_*: 12..16
        30 => 17,
        31 => 18,
        33 => 19,
        34 => 20,
        35 => 21,
        36 => 22,
        37 => 15,
        38 => 24,
        39 => 25,
        40 => 26,
        41 => 27,
        50 => 29,
        _ => 1,
    }
}

/// Requested before CEF gave us the item's callback (first OnDownloadUpdated).
#[derive(Clone, Copy, PartialEq)]
enum DownloadAction {
    None,
    Cancel,
    Pause,
    Resume,
}

pub struct DownloadOperation {
    uri: String,
    mime: String,
    disposition: String,
    total: Cell<i64>,
    received: Cell<i64>,
    state: Cell<i32>,
    reason: Cell<i32>,
    end_time: RefCell<String>,
    /// Windows path of the file.
    path: RefCell<String>,
    bytes_changed: EventList,
    end_time_changed: EventList,
    state_changed: EventList,
    control: RefCell<Option<cef::DownloadItemCallback>>,
    pending: Cell<DownloadAction>,
}

type DownloadObj = ICoreWebView2DownloadOperationObj<DownloadOperation>;

thread_local! {
    static DOWNLOADS: RefCell<HashMap<u32, Rc<DownloadObj>>> = RefCell::new(HashMap::new());
}

impl DownloadOperation {
    fn act(&self, action: DownloadAction) {
        use cef::ImplDownloadItemCallback;
        match self.control.borrow().as_ref() {
            Some(c) => match action {
                DownloadAction::Cancel => c.cancel(),
                DownloadAction::Pause => c.pause(),
                DownloadAction::Resume => c.resume(),
                DownloadAction::None => {}
            },
            None => self.pending.set(action),
        }
    }
}

impl ICoreWebView2DownloadOperationImpl for DownloadOperation {
    fn add_BytesReceivedChanged(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.bytes_changed.add(handler)
    }
    fn remove_BytesReceivedChanged(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.bytes_changed.remove(token)
    }
    fn add_EstimatedEndTimeChanged(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.end_time_changed.add(handler)
    }
    fn remove_EstimatedEndTimeChanged(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.end_time_changed.remove(token)
    }
    fn add_StateChanged(&self, _ctx: &mut Ctx, handler: ObjRef) -> WvResult<EventRegistrationToken> {
        self.state_changed.add(handler)
    }
    fn remove_StateChanged(&self, _ctx: &mut Ctx, token: EventRegistrationToken) -> WvResult<()> {
        self.state_changed.remove(token)
    }
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.clone())
    }
    fn get_ContentDisposition(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.disposition.clone())
    }
    fn get_MimeType(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.mime.clone())
    }
    fn get_TotalBytesToReceive(&self, _ctx: &mut Ctx) -> WvResult<i64> {
        Ok(self.total.get())
    }
    fn get_BytesReceived(&self, _ctx: &mut Ctx) -> WvResult<i64> {
        Ok(self.received.get())
    }
    fn get_EstimatedEndTime(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.end_time.borrow().clone())
    }
    fn get_ResultFilePath(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.path.borrow().clone())
    }
    fn get_State(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.state.get())
    }
    fn get_InterruptReason(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.reason.get())
    }
    fn Cancel(&self, _ctx: &mut Ctx) -> WvResult<()> {
        if self.state.get() != DOWNLOAD_STATE_COMPLETED {
            self.act(DownloadAction::Cancel);
        }
        Ok(())
    }
    fn Pause(&self, _ctx: &mut Ctx) -> WvResult<()> {
        if self.state.get() == DOWNLOAD_STATE_IN_PROGRESS {
            self.act(DownloadAction::Pause);
        }
        Ok(())
    }
    fn Resume(&self, _ctx: &mut Ctx) -> WvResult<()> {
        if self.reason.get() == INTERRUPT_REASON_USER_PAUSED {
            self.act(DownloadAction::Resume);
        }
        Ok(())
    }
    fn get_CanResume(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.reason.get() == INTERRUPT_REASON_USER_PAUSED)
    }
}

pub struct DownloadStartingArgs {
    operation: ObjRef,
    cancel: Cell<bool>,
    path: RefCell<String>,
    handled: Cell<bool>,
    deferral: Rc<DeferralSlot>,
}

impl ICoreWebView2DownloadStartingEventArgsImpl for DownloadStartingArgs {
    fn get_DownloadOperation(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.operation.clone())
    }
    fn get_Cancel(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.cancel.get())
    }
    fn put_Cancel(&self, _ctx: &mut Ctx, cancel: bool) -> WvResult<()> {
        self.cancel.set(cancel);
        Ok(())
    }
    fn get_ResultFilePath(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.path.borrow().clone())
    }
    fn put_ResultFilePath(&self, _ctx: &mut Ctx, path: Option<String>) -> WvResult<()> {
        let path = path.ok_or(E_INVALIDARG)?;
        if crate::paths::to_unix(&path).is_none() {
            return Err(E_INVALIDARG);
        }
        *self.path.borrow_mut() = path;
        Ok(())
    }
    fn get_Handled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.handled.get())
    }
    fn put_Handled(&self, _ctx: &mut Ctx, handled: bool) -> WvResult<()> {
        // Only hides the download UI in WebView2; there is none here.
        self.handled.set(handled);
        Ok(())
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.deferral.get())
    }
}

/// "name.ext" in `dir`, or "name (1).ext", ... if it exists (like Chromium).
fn unique_download_path(dir: &std::path::Path, name: &str) -> std::path::PathBuf {
    let name = if name.is_empty() { "download" } else { name };
    let candidate = dir.join(name);
    if !candidate.exists() {
        return candidate;
    }
    let (stem, ext) = match name.rfind('.') {
        Some(i) if i > 0 => (&name[..i], &name[i..]),
        _ => (name, ""),
    };
    (1..1000)
        .map(|n| dir.join(format!("{stem} ({n}){ext}")))
        .find(|p| !p.exists())
        .unwrap_or(candidate)
}

/// Synchronous from OnBeforeDownload: DownloadStarting, then CEF continues to the chosen path.
pub fn on_download_starting(view: u32, info: DownloadInfo, callback: cef::BeforeDownloadCallback) {
    let folder = DOWNLOAD_FOLDER.with(|d| d.borrow().clone());
    // Keep the folder as the app knows it (C:\users\...\Downloads), even if it links elsewhere.
    let default_path = crate::paths::to_unix(&folder)
        .and_then(|dir| unique_download_path(&dir, &info.suggested).file_name().map(|n| n.to_string_lossy().into_owned()))
        .map(|name| format!("{}\\{name}", folder.trim_end_matches('\\')))
        .unwrap_or_default();
    let operation = Rc::new(ICoreWebView2DownloadOperationObj(DownloadOperation {
        uri: info.uri,
        mime: info.mime,
        disposition: info.disposition,
        total: Cell::new(info.total.max(-1)),
        received: Cell::new(0),
        state: Cell::new(DOWNLOAD_STATE_IN_PROGRESS),
        reason: Cell::new(0),
        end_time: RefCell::new(String::new()),
        path: RefCell::new(default_path.clone()),
        bytes_changed: EventList::default(),
        end_time_changed: EventList::default(),
        state_changed: EventList::default(),
        control: RefCell::new(None),
        pending: Cell::new(DownloadAction::None),
    }));
    let args = Rc::new(ICoreWebView2DownloadStartingEventArgsObj(DownloadStartingArgs {
        operation: ObjRef::Runtime(operation.clone()),
        cancel: Cell::new(false),
        path: RefCell::new(default_path),
        handled: Cell::new(false),
        deferral: DeferralSlot::new(),
    }));
    let id = info.id;
    let finish = {
        let (args, operation) = (args.clone(), operation.clone());
        move || {
            use cef::ImplBeforeDownloadCallback;
            if args.0.cancel.get() {
                // Dropping the callback without cont() cancels the download.
                return;
            }
            let windows = args.0.path.borrow().clone();
            let Some(unix) = crate::paths::to_unix(&windows) else {
                eprintln!("helper: download path {windows:?} has no unix equivalent, cancelled");
                return;
            };
            if let Some(dir) = unix.parent() {
                let _ = std::fs::create_dir_all(dir);
            }
            *operation.0.path.borrow_mut() = windows;
            DOWNLOADS.with(|d| d.borrow_mut().insert(id, operation));
            callback.cont(Some(&unix.to_string_lossy().as_ref().into()), 0);
        }
    };
    let handlers = with_webview(view, |wv, _| wv.ev(Ev::DownloadStarting).snapshot()).unwrap_or_default();
    if handlers.is_empty() {
        finish();
        return;
    }
    with_webview(view, |_, sender| {
        let args_ref = ObjRef::Runtime(args.clone());
        for h in handlers {
            if let Err(e) = ICoreWebView2DownloadStartingEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
                eprintln!("helper: DownloadStarting handler failed: {e:#x}");
            }
        }
    });
    if args.0.deferral.settle(Box::new(finish.clone())) {
        finish();
    }
}

/// ISO 8601 UTC ("2026-11-11T05:23:22Z") of a unix timestamp.
fn iso8601(secs: i64) -> String {
    let (days, rem) = (secs.div_euclid(86400), secs.rem_euclid(86400));
    // Civil date from days since 1970-01-01 (Howard Hinnant's algorithm).
    let z = days + 719468;
    let era = z.div_euclid(146097);
    let doe = z - era * 146097;
    let yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    let doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    let mp = (5 * doy + 2) / 153;
    let day = doy - (153 * mp + 2) / 5 + 1;
    let month = if mp < 10 { mp + 3 } else { mp - 9 };
    let year = yoe + era * 400 + (month <= 2) as i64;
    format!("{year:04}-{month:02}-{day:02}T{:02}:{:02}:{:02}Z", rem / 3600, rem / 60 % 60, rem % 60)
}

pub fn on_download_updated(id: u32, update: DownloadUpdate, callback: cef::DownloadItemCallback) {
    let Some(op) = DOWNLOADS.with(|d| d.borrow().get(&id).cloned()) else { return };
    let o = &op.0;
    if o.control.borrow().is_none() {
        *o.control.borrow_mut() = Some(callback);
        let pending = o.pending.replace(DownloadAction::None);
        o.act(pending);
    }
    // CEF reports the unix path; convert only if it differs from the requested one (renamed).
    let full_path = std::path::Path::new(&update.full_path);
    if !update.full_path.is_empty() && crate::paths::to_unix(&o.path.borrow()).as_deref() != Some(full_path) {
        *o.path.borrow_mut() = crate::paths::to_windows(full_path);
    }
    if update.total > 0 {
        o.total.set(update.total);
    }
    let (state, reason) = match update.state {
        DownloadState::InProgress => (DOWNLOAD_STATE_IN_PROGRESS, 0),
        DownloadState::Paused => (DOWNLOAD_STATE_INTERRUPTED, INTERRUPT_REASON_USER_PAUSED),
        DownloadState::Completed => (DOWNLOAD_STATE_COMPLETED, 0),
        DownloadState::Interrupted(r) => (DOWNLOAD_STATE_INTERRUPTED, interrupt_reason(r)),
    };
    let sender = ObjRef::Runtime(op.clone());
    if o.received.replace(update.received) != update.received {
        for h in o.bytes_changed.snapshot() {
            ICoreWebView2BytesReceivedChangedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &ObjRef::Null);
        }
    }
    let end_time = if state == DOWNLOAD_STATE_IN_PROGRESS && update.speed > 0 && o.total.get() > 0 {
        let now = std::time::SystemTime::now().duration_since(std::time::UNIX_EPOCH).map(|d| d.as_secs() as i64).unwrap_or(0);
        iso8601(now + (o.total.get() - update.received).max(0) / update.speed)
    } else {
        o.end_time.borrow().clone()
    };
    if *o.end_time.borrow() != end_time {
        *o.end_time.borrow_mut() = end_time;
        for h in o.end_time_changed.snapshot() {
            ICoreWebView2EstimatedEndTimeChangedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &ObjRef::Null);
        }
    }
    let state_changed = o.state.replace(state) != state;
    let reason_changed = o.reason.replace(reason) != reason;
    if state_changed || reason_changed {
        for h in o.state_changed.snapshot() {
            ICoreWebView2StateChangedEventHandlerApp(h).Invoke_async(&mut Ctx, &sender, &ObjRef::Null);
        }
    }
    if state == DOWNLOAD_STATE_COMPLETED || (state == DOWNLOAD_STATE_INTERRUPTED && reason != INTERRUPT_REASON_USER_PAUSED) {
        DOWNLOADS.with(|d| d.borrow_mut().remove(&id));
    }
}

// ===================================================================
// PermissionRequested
// ===================================================================

const PERMISSION_STATE_DEFAULT: i32 = 0;
const PERMISSION_STATE_ALLOW: i32 = 1;
const PERMISSION_STATE_DENY: i32 = 2;

thread_local! {
    /// Answers the user gave in the default prompt, kept for the session: (origin, kind) -> allowed.
    static PERMISSION_ANSWERS: RefCell<HashMap<(String, i32), bool>> = RefCell::new(HashMap::new());
}

pub struct PermissionArgs {
    uri: String,
    kind: i32,
    state: Cell<i32>,
    handled: Cell<bool>,
    saves_in_profile: Cell<bool>,
    deferral: Rc<DeferralSlot>,
}

impl ICoreWebView2PermissionRequestedEventArgs3Impl for PermissionArgs {
    fn get_Uri(&self, _ctx: &mut Ctx) -> WvResult<String> {
        Ok(self.uri.clone())
    }
    fn get_PermissionKind(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.kind)
    }
    fn get_IsUserInitiated(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(false) // CEF does not say
    }
    fn get_State(&self, _ctx: &mut Ctx) -> WvResult<i32> {
        Ok(self.state.get())
    }
    fn put_State(&self, _ctx: &mut Ctx, state: i32) -> WvResult<()> {
        if !(PERMISSION_STATE_DEFAULT..=PERMISSION_STATE_DENY).contains(&state) {
            return Err(E_INVALIDARG);
        }
        self.state.set(state);
        Ok(())
    }
    fn GetDeferral(&self, _ctx: &mut Ctx) -> WvResult<ObjRef> {
        Ok(self.deferral.get())
    }
    fn get_Handled(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.handled.get())
    }
    fn put_Handled(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.handled.set(value);
        Ok(())
    }
    fn get_SavesInProfile(&self, _ctx: &mut Ctx) -> WvResult<bool> {
        Ok(self.saves_in_profile.get())
    }
    fn put_SavesInProfile(&self, _ctx: &mut Ctx, value: bool) -> WvResult<()> {
        self.saves_in_profile.set(value);
        Ok(())
    }
}

/// One permission kind: event, then (state DEFAULT) the remembered answer or the default prompt.
fn decide_permission(view: u32, origin: String, kind: i32, done: Box<dyn FnOnce(bool)>) {
    let args = Rc::new(ICoreWebView2PermissionRequestedEventArgs3Obj(PermissionArgs {
        uri: origin.clone(),
        kind,
        state: Cell::new(PERMISSION_STATE_DEFAULT),
        handled: Cell::new(false),
        saves_in_profile: Cell::new(true),
        deferral: DeferralSlot::new(),
    }));
    with_webview(view, |wv, sender| {
        let args_ref = ObjRef::Runtime(args.clone());
        for h in wv.ev(Ev::PermissionRequested).snapshot() {
            if let Err(e) = ICoreWebView2PermissionRequestedEventHandlerApp(h).Invoke(&mut Ctx, &sender, &args_ref) {
                eprintln!("helper: PermissionRequested handler failed: {e:#x}");
            }
        }
    });
    let finish = {
        let args = args.clone();
        move || match args.0.state.get() {
            PERMISSION_STATE_ALLOW => done(true),
            PERMISSION_STATE_DENY => done(false),
            _ => {
                let key = (origin.clone(), kind);
                if let Some(allow) = PERMISSION_ANSWERS.with(|a| a.borrow().get(&key).copied()) {
                    return done(allow);
                }
                crate::show_permission_prompt(view, &origin, kind, Box::new(move |allow| {
                    PERMISSION_ANSWERS.with(|a| a.borrow_mut().insert(key, allow));
                    done(allow);
                }));
            }
        }
    };
    let deferred: Rc<RefCell<Option<Box<dyn FnOnce()>>>> = Rc::new(RefCell::new(Some(Box::new(finish))));
    let later = deferred.clone();
    if args.0.deferral.settle(Box::new(move || {
        if let Some(f) = later.borrow_mut().take() {
            f()
        }
    })) {
        if let Some(f) = deferred.borrow_mut().take() {
            f()
        }
    }
}

/// All `kinds` must be allowed (CEF may ask for several in one prompt); one event per kind.
pub fn on_permission_requested(view: u32, origin: &str, kinds: Vec<i32>, done: Box<dyn FnOnce(bool)>) {
    fn next(view: u32, origin: String, mut kinds: Vec<i32>, done: Box<dyn FnOnce(bool)>) {
        let Some(kind) = kinds.pop() else { return done(true) };
        let o = origin.clone();
        decide_permission(view, o, kind, Box::new(move |allow| {
            if allow {
                next(view, origin, kinds, done)
            } else {
                done(false)
            }
        }));
    }
    let mut kinds = kinds;
    kinds.reverse();
    next(view, origin.to_string(), kinds, done);
}

// ===================================================================
// Status bar text, favicon
// ===================================================================

pub fn on_status_text(view: u32, text: &str) {
    with_webview(view, |wv, sender| {
        if *wv.status_text.borrow() != text {
            *wv.status_text.borrow_mut() = text.to_string();
            raise_async!(wv, sender, StatusBarTextChanged, ICoreWebView2StatusBarTextChangedEventHandlerApp, ObjRef::Null);
        }
    });
}

pub fn on_favicon_changed(view: u32, url: &str) {
    with_webview(view, |wv, sender| {
        if *wv.favicon.borrow() != url {
            *wv.favicon.borrow_mut() = url.to_string();
            raise_async!(wv, sender, FaviconChanged, ICoreWebView2FaviconChangedEventHandlerApp, ObjRef::Null);
        }
    });
}
