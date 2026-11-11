//! CEF renderer process: `window.chrome.webview` bridge, document-created scripts, ExecuteScript.
//!
//! Process messages (CefProcessMessage) between renderer and browser process:
//!   renderer -> browser  "wv2.post"   [json]                       chrome.webview.postMessage
//!   browser  -> renderer "wv2.exec"   [id, script]                 ExecuteScript
//!   renderer -> browser  "wv2.result" [id, json, ok, name, message, line, col, exception_json]  ExecuteScript result
//!   renderer -> browser  "wv2.close"  []                         window.close() in the main frame
//!   renderer -> browser  "wv2.host"   [request, op, root, name, path, args]  chrome.webview.hostObjects
//!   browser  -> renderer "wv2.hostresult" [request, ok, value | error message]
//! AddScriptToExecuteOnDocumentCreated scripts: file `<--wv2-scripts-dir>/<browser_id>.scripts`
//! (records: u32 LE length + UTF-8), re-read for every new JS context so it also survives
//! renderer process swaps on cross-site navigation.

use cef::rc::Rc as _;
use cef::*;
use std::path::PathBuf;

pub const MSG_POST: &str = "wv2.post";
pub const MSG_EXEC: &str = "wv2.exec";
pub const MSG_RESULT: &str = "wv2.result";
pub const MSG_CLOSE: &str = "wv2.close";
pub const MSG_HOST: &str = "wv2.host";
pub const MSG_HOST_RESULT: &str = "wv2.hostresult";
pub const SCRIPTS_DIR_SWITCH: &str = "wv2-scripts-dir";

/// Installs window.chrome.webview on top of the native __wv2_post and __wv2_host functions, which
/// it then deletes.
///
/// hostObjects: every member access gives a proxy for a path of names below a root (a named host
/// object or an object id from the host). Calling it invokes the method, awaiting it (`then`) reads
/// the property, assigning sets it; all asynchronous, as WebView2's default async proxies. Results
/// that are objects come back as {o: id} and become new proxies; {u: 1} is undefined.
const BOOTSTRAP: &str = r#"(function () {
  if (window.chrome && window.chrome.webview) return;
  var post = window.__wv2_post, hostCall = window.__wv2_host;
  delete window.__wv2_post;
  delete window.__wv2_host;
  var target = new EventTarget();
  var pending = new Map();
  var REF = Symbol("wv2 host object");
  function encode(v) {
    if (Array.isArray(v)) return v.map(encode);
    if ((typeof v === "object" || typeof v === "function") && v !== null) {
      var ref = v[REF];
      if (ref && ref.root && !ref.path.length) return { __wv2obj: ref.root };
      return undefined;  // FIXME: plain script objects and functions are not passed yet
    }
    return v;
  }
  function decode(v) {
    if (Array.isArray(v)) return v.map(decode);
    if (v !== null && typeof v === "object") {
      if ("o" in v) return remote(v.o, "", []);
      return undefined;
    }
    return v;
  }
  function call(op, root, name, path, args) {
    return new Promise(function (resolve, reject) {
      var id = hostCall(op, root, name, path, args.map(encode));
      pending.set(id, { resolve: resolve, reject: reject });
    });
  }
  function remote(root, name, path) {
    return new Proxy(function () {}, {
      get: function (t, prop) {
        if (prop === REF) return { root: root, name: name, path: path };
        if (typeof prop === "symbol") return undefined;
        if (prop === "then") {
          if (!path.length) return undefined;  // objects are not thenable: awaiting gives the proxy
          var p = call(0, root, name, path, []);
          return p.then.bind(p);
        }
        return remote(root, name, path.concat([prop]));
      },
      set: function (t, prop, value) {
        if (typeof prop === "symbol") return false;
        call(1, root, name, path.concat([prop]), [value]).catch(function (e) { console.error(e); });
        return true;
      },
      apply: function (t, self, args) {
        if (!path.length) return Promise.reject(new TypeError("host object is not a function"));
        return call(2, root, name, path, Array.prototype.slice.call(args));
      }
    });
  }
  var options = { forceLocalProperties: ["then", "toJSON", "Symbol(Symbol.toPrimitive)", "Symbol(Symbol.toStringTag)"],
                  log: function () {}, shouldSerializeDates: false, defaultSyncProxy: false,
                  forceAsyncMethodMatches: [/Async$/], ignoreMemberNotFoundError: false, shouldPassTypedArraysAsArrays: false };
  var hostObjects = new Proxy({}, {
    get: function (t, prop) {
      if (typeof prop === "symbol") return undefined;
      if (prop === "options") return options;
      if (prop === "sync") throw new Error("chrome.webview.hostObjects.sync is not supported yet, use the asynchronous proxies");
      if (prop === "cleanupSome") return function () {};
      return remote(0, prop, []);
    }
  });
  var webview = {
    postMessage: function (message) {
      var json = JSON.stringify(message);
      if (json === undefined) throw new TypeError("chrome.webview.postMessage: message is not serializable");
      post(json);
    },
    hostObjects: hostObjects,
    releaseObject: function (proxy) {
      var ref = proxy && proxy[REF];
      if (ref && ref.root && !ref.path.length) hostCall(3, ref.root, "", [], []);
    },
    addEventListener: target.addEventListener.bind(target),
    removeEventListener: target.removeEventListener.bind(target),
    dispatchEvent: target.dispatchEvent.bind(target),
    __dispatch: function (data) {
      target.dispatchEvent(new MessageEvent("message", { data: data }));
    },
    __hostResult: function (id, ok, value) {
      var p = pending.get(id);
      if (!p) return;
      pending.delete(id);
      if (ok) p.resolve(decode(value)); else p.reject(new Error(value));
    }
  };
  Object.defineProperty(webview, "__hostResult", { enumerable: false });
  Object.defineProperty(webview, "__dispatch", { enumerable: false });
  if (!window.chrome) window.chrome = {};
  Object.defineProperty(window.chrome, "webview", { value: webview, enumerable: true });
})();"#;

pub fn settings_file(dir: &str, browser_id: i32) -> PathBuf {
    PathBuf::from(dir).join(format!("{browser_id}.settings"))
}

/// WV2_SETTING_* flags written by the browser process (missing file = defaults).
fn read_settings(dir: &str, browser_id: i32) -> u32 {
    std::fs::read(settings_file(dir, browser_id))
        .ok()
        .and_then(|d| d.get(..4).map(|b| u32::from_le_bytes(b.try_into().unwrap())))
        .unwrap_or(crate::protocol::SETTINGS_DEFAULT)
}

pub fn scripts_file(dir: &str, browser_id: i32) -> PathBuf {
    PathBuf::from(dir).join(format!("{browser_id}.scripts"))
}

fn read_scripts(dir: &str, browser_id: i32) -> Vec<String> {
    let Ok(data) = std::fs::read(scripts_file(dir, browser_id)) else { return Vec::new() };
    let mut out = Vec::new();
    let mut rest = &data[..];
    while rest.len() >= 4 {
        let len = u32::from_le_bytes(rest[..4].try_into().unwrap()) as usize;
        if rest.len() < 4 + len {
            break;
        }
        out.push(String::from_utf8_lossy(&rest[4..4 + len]).into_owned());
        rest = &rest[4 + len..];
    }
    out
}

/// Encodes a string as a JSON literal (also a valid JS literal).
pub fn json_string(s: &str) -> String {
    let mut out = String::with_capacity(s.len() + 2);
    out.push('"');
    for c in s.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            '\u{2028}' => out.push_str("\\u2028"),
            '\u{2029}' => out.push_str("\\u2029"),
            c if (c as u32) < 0x20 => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
    out.push('"');
    out
}

fn eval(context: &V8Context, code: &str) -> Result<Option<V8Value>, String> {
    let mut retval = None;
    let mut exception = None;
    let ok = context.eval(Some(&code.into()), Some(&"".into()), 0, Some(&mut retval), Some(&mut exception));
    if ok != 0 {
        Ok(retval)
    } else {
        Err(exception.map(|e| CefString::from(&e.message()).to_string()).unwrap_or_default())
    }
}

fn send_to_browser(frame: &Frame, name: &str, args: &[&str]) {
    let Some(mut msg) = process_message_create(Some(&name.into())) else { return };
    if let Some(list) = msg.argument_list() {
        for (i, a) in args.iter().enumerate() {
            list.set_string(i, Some(&(*a).into()));
        }
    }
    frame.send_process_message(ProcessId::BROWSER, Some(&mut msg));
}

#[derive(Clone)]
struct PostHandler;

wrap_v8_handler! {
    struct PostHandlerBuilder {
        handler: PostHandler,
    }

    impl V8Handler {
        fn execute(
            &self,
            _name: Option<&CefString>,
            _object: Option<&mut V8Value>,
            arguments: Option<&[Option<V8Value>]>,
            _retval: Option<&mut Option<V8Value>>,
            exception: Option<&mut CefString>,
        ) -> ::std::os::raw::c_int {
            let json = arguments
                .and_then(|a| a.first().cloned().flatten())
                .filter(|v| v.is_string() != 0)
                .map(|v| CefString::from(&v.string_value()).to_string());
            let frame = v8_context_get_current_context().and_then(|c| c.frame());
            match (json, frame) {
                (Some(json), Some(frame)) => send_to_browser(&frame, MSG_POST, &[&json]),
                _ => {
                    if let Some(e) = exception {
                        *e = "postMessage: invalid call".into();
                    }
                }
            }
            1
        }
    }
}

/// Script value -> entry `i` of a CEF list (undefined = {"u": 1}, host object proxy = {"o": id}).
fn v8_to_list(value: Option<&V8Value>, list: &ListValue, i: usize, depth: u32) {
    let marker = |key: &str, v: i32| {
        if let Some(d) = dictionary_value_create() {
            d.set_int(Some(&key.into()), v);
            list.set_dictionary(i, Some(&mut d.clone()));
        }
    };
    let Some(v) = value else { return marker("u", 1) };
    if v.is_null() != 0 {
        list.set_null(i);
    } else if v.is_bool() != 0 {
        list.set_bool(i, v.bool_value());
    } else if v.is_int() != 0 {
        list.set_int(i, v.int_value());
    } else if v.is_double() != 0 || v.is_uint() != 0 {
        list.set_double(i, v.double_value());
    } else if v.is_string() != 0 {
        list.set_string(i, Some(&CefString::from(&v.string_value()).to_string().as_str().into()));
    } else if v.is_array() != 0 && depth < 32 {
        let Some(items) = list_value_create() else { return };
        let n = v.array_length().max(0) as usize;
        items.set_size(n);
        for j in 0..n {
            v8_to_list(v.value_byindex(j as i32).as_ref(), &items, j, depth + 1);
        }
        list.set_list(i, Some(&mut items.clone()));
    } else if let Some(id) = v.value_bykey(Some(&"__wv2obj".into())).filter(|id| id.is_int() != 0 || id.is_uint() != 0) {
        marker("o", id.int_value());
    } else {
        marker("u", 1);
    }
}

/// Entry `i` of a CEF list -> script value ({"o": id} / {"u": 1} stay objects, decoded in JS).
fn list_to_v8(list: &ListValue, i: usize, depth: u32) -> Option<V8Value> {
    let ty = list.get_type(i);
    if ty == ValueType::BOOL {
        v8_value_create_bool(list.bool(i))
    } else if ty == ValueType::INT {
        v8_value_create_int(list.int(i))
    } else if ty == ValueType::DOUBLE {
        v8_value_create_double(list.double(i))
    } else if ty == ValueType::STRING {
        v8_value_create_string(Some(&CefString::from(&list.string(i)).to_string().as_str().into()))
    } else if ty == ValueType::LIST && depth < 32 {
        let items = list.list(i)?;
        let array = v8_value_create_array(items.size() as i32)?;
        for j in 0..items.size() {
            let mut item = list_to_v8(&items, j, depth + 1);
            array.set_value_byindex(j as i32, item.as_mut());
        }
        Some(array)
    } else if ty == ValueType::DICTIONARY {
        let d = list.dictionary(i)?;
        let object = v8_value_create_object(None, None)?;
        for key in ["o", "u"] {
            if d.has_key(Some(&key.into())) != 0 {
                let mut v = v8_value_create_int(d.int(Some(&key.into())));
                object.set_value_bykey(Some(&key.into()), v.as_mut(), V8Propertyattribute::default());
            }
        }
        Some(object)
    } else {
        v8_value_create_null()
    }
}

/// __wv2_host(op, root, name, path, args) -> request id; the result arrives as "wv2.hostresult".
#[derive(Clone)]
struct HostHandler;

static NEXT_HOST_REQUEST: std::sync::atomic::AtomicI32 = std::sync::atomic::AtomicI32::new(1);

wrap_v8_handler! {
    struct HostHandlerBuilder {
        handler: HostHandler,
    }

    impl V8Handler {
        fn execute(
            &self,
            _name: Option<&CefString>,
            _object: Option<&mut V8Value>,
            arguments: Option<&[Option<V8Value>]>,
            retval: Option<&mut Option<V8Value>>,
            exception: Option<&mut CefString>,
        ) -> ::std::os::raw::c_int {
            let args = arguments.unwrap_or(&[]);
            let arg = |i: usize| args.get(i).cloned().flatten();
            let frame = v8_context_get_current_context().and_then(|c| c.frame());
            let (Some(frame), Some(mut msg)) = (frame, process_message_create(Some(&MSG_HOST.into()))) else {
                if let Some(e) = exception {
                    *e = "hostObjects: no frame".into();
                }
                return 1;
            };
            let request = NEXT_HOST_REQUEST.fetch_add(1, std::sync::atomic::Ordering::Relaxed);
            if let Some(list) = msg.argument_list() {
                list.set_size(6);
                list.set_int(0, request);
                list.set_int(1, arg(0).map(|v| v.int_value()).unwrap_or(0));
                list.set_int(2, arg(1).map(|v| v.int_value()).unwrap_or(0));
                let name = arg(2).filter(|v| v.is_string() != 0).map(|v| CefString::from(&v.string_value()).to_string());
                list.set_string(3, Some(&name.unwrap_or_default().as_str().into()));
                v8_to_list(arg(3).as_ref(), &list, 4, 0);
                v8_to_list(arg(4).as_ref(), &list, 5, 0);
            }
            frame.send_process_message(ProcessId::BROWSER, Some(&mut msg));
            if let Some(r) = retval {
                *r = v8_value_create_int(request);
            }
            1
        }
    }
}

/// window.close() in the main frame -> WindowCloseRequested. Chromium only closes script-opened
/// windows; WebView2 always raises the event and leaves the decision to the app.
#[derive(Clone)]
struct CloseHandler;

wrap_v8_handler! {
    struct CloseHandlerBuilder {
        handler: CloseHandler,
    }

    impl V8Handler {
        fn execute(
            &self,
            _name: Option<&CefString>,
            _object: Option<&mut V8Value>,
            _arguments: Option<&[Option<V8Value>]>,
            _retval: Option<&mut Option<V8Value>>,
            _exception: Option<&mut CefString>,
        ) -> ::std::os::raw::c_int {
            if let Some(frame) = v8_context_get_current_context().and_then(|c| c.frame()) {
                send_to_browser(&frame, MSG_CLOSE, &[]);
            }
            1
        }
    }
}

/// "wv2.hostresult" -> chrome.webview.__hostResult(request, ok, value).
fn host_result(frame: &Frame, message: &ProcessMessage) {
    let Some(args) = message.argument_list() else { return };
    let Some(context) = frame.v8_context() else { return };
    if context.enter() == 0 {
        return;
    }
    let deliver = context
        .global()
        .and_then(|g| g.value_bykey(Some(&"chrome".into())))
        .and_then(|c| c.value_bykey(Some(&"webview".into())))
        .and_then(|w| w.value_bykey(Some(&"__hostResult".into())).map(|f| (w, f)));
    if let Some((webview, deliver)) = deliver.filter(|(_, f)| f.is_function() != 0) {
        let call_args = [v8_value_create_int(args.int(0)), v8_value_create_bool(args.bool(1)), list_to_v8(&args, 2, 0)];
        deliver.execute_function(Some(&mut webview.clone()), Some(&call_args));
    }
    context.exit();
}

#[derive(Clone)]
pub struct Renderer;

wrap_render_process_handler! {
    pub struct RendererBuilder {
        renderer: Renderer,
    }

    impl RenderProcessHandler {
        fn on_context_created(
            &self,
            browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            context: Option<&mut V8Context>,
        ) {
            let (Some(browser), Some(context)) = (browser, context) else { return };
            let Some(global) = context.global() else { return };
            if frame.is_some_and(|f| f.is_main() != 0) {
                let mut handler = CloseHandlerBuilder::new(CloseHandler);
                let mut func = v8_value_create_function(Some(&"close".into()), Some(&mut handler));
                global.set_value_bykey(Some(&"close".into()), func.as_mut(), V8Propertyattribute::default());
            }
            let dir = command_line_get_global()
                .map(|cl| CefString::from(&cl.switch_value(Some(&SCRIPTS_DIR_SWITCH.into()))).to_string())
                .unwrap_or_default();
            let settings = if dir.is_empty() { crate::protocol::SETTINGS_DEFAULT } else { read_settings(&dir, browser.identifier()) };
            // IsWebMessageEnabled = FALSE: no window.chrome.webview at all, as in WebView2.
            if settings & crate::protocol::SETTING_WEB_MESSAGE != 0 {
                let mut handler = PostHandlerBuilder::new(PostHandler);
                let mut func = v8_value_create_function(Some(&"__wv2_post".into()), Some(&mut handler));
                global.set_value_bykey(Some(&"__wv2_post".into()), func.as_mut(), V8Propertyattribute::default());
                let mut handler = HostHandlerBuilder::new(HostHandler);
                let mut func = v8_value_create_function(Some(&"__wv2_host".into()), Some(&mut handler));
                global.set_value_bykey(Some(&"__wv2_host".into()), func.as_mut(), V8Propertyattribute::default());
                if let Err(e) = eval(context, BOOTSTRAP) {
                    eprintln!("helper(renderer): bootstrap failed: {e}");
                }
            }
            if dir.is_empty() {
                return;
            }
            for script in read_scripts(&dir, browser.identifier()) {
                if let Err(e) = eval(context, &script) {
                    eprintln!("helper(renderer): document script failed: {e}");
                }
            }
        }

        fn on_process_message_received(
            &self,
            _browser: Option<&mut Browser>,
            frame: Option<&mut Frame>,
            _source_process: ProcessId,
            message: Option<&mut ProcessMessage>,
        ) -> ::std::os::raw::c_int {
            let (Some(frame), Some(message)) = (frame, message) else { return 0 };
            let name = CefString::from(&message.name()).to_string();
            if name == MSG_HOST_RESULT {
                host_result(frame, message);
                return 1;
            }
            if name != MSG_EXEC {
                return 0;
            }
            let Some(args) = message.argument_list() else { return 1 };
            let id = CefString::from(&args.string(0)).to_string();
            let script = CefString::from(&args.string(1)).to_string();
            // Exceptions (including SyntaxError from eval) are caught in JS and returned as
            // {ok, r | name, message, line, col, json} so ExecuteScriptWithResult gets the details.
            let mut fields: [String; 6] = ["0".into(), "null".into(), String::new(), String::new(), "0".into(), "0".into()];
            let mut exception_json = String::from("{}");
            match frame.v8_context() {
                Some(context) if context.enter() != 0 => {
                    let wrapper = format!("(function(){{try{{var r=(0,eval)({});var j=JSON.stringify(r);\
return {{ok:true,r:j===undefined?'null':j}};}}catch(e){{var m=/<anonymous>:(\\d+):(\\d+)/.exec(e&&e.stack||'');\
var d;try{{d=JSON.stringify({{className:e&&e.constructor?e.constructor.name:typeof e,description:String(e&&e.stack||e),type:typeof e}});}}catch(x){{d='{{}}';}}\
return {{ok:false,name:(e&&e.name)||'Error',message:(e&&e.message!==undefined)?String(e.message):String(e),\
line:m?m[1]-1:0,col:m?m[2]-1:0,json:d}};}}}})()", json_string(&script));
                    let result = eval(&context, &wrapper);
                    let get = |v: &V8Value, k: &str| v.value_bykey(Some(&k.into()));
                    let text = |v: Option<V8Value>| v.filter(|v| v.is_string() != 0).map(|v| CefString::from(&v.string_value()).to_string());
                    let int = |v: Option<V8Value>| v.filter(|v| v.is_int() != 0 || v.is_double() != 0).map(|v| v.int_value().max(0).to_string());
                    match result {
                        Ok(Some(v)) if v.is_object() != 0 => {
                            if get(&v, "ok").is_some_and(|o| o.bool_value() != 0) {
                                fields[0] = "1".into();
                                fields[1] = text(get(&v, "r")).unwrap_or_else(|| "null".into());
                            } else {
                                fields[2] = text(get(&v, "name")).unwrap_or_default();
                                fields[3] = text(get(&v, "message")).unwrap_or_default();
                                fields[4] = int(get(&v, "line")).unwrap_or_else(|| "0".into());
                                fields[5] = int(get(&v, "col")).unwrap_or_else(|| "0".into());
                                exception_json = text(get(&v, "json")).unwrap_or_else(|| "{}".into());
                                eprintln!("helper(renderer): ExecuteScript exception: {}: {}", fields[2], fields[3]);
                            }
                        }
                        Ok(_) => {}
                        Err(e) => eprintln!("helper(renderer): ExecuteScript failed: {e}"),
                    }
                    context.exit();
                }
                _ => {}
            }
            send_to_browser(frame, MSG_RESULT, &[&id, &fields[1], &fields[0], &fields[2], &fields[3], &fields[4], &fields[5], &exception_json]);
            1
        }
    }
}
