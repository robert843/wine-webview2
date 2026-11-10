//! Proces renderera CEF: most `window.chrome.webview`, skrypty startowe, ExecuteScript.
//!
//! Komunikaty procesowe (CefProcessMessage) między rendererem a procesem przeglądarki:
//!   renderer -> browser  "wv2.post"   [json]                       chrome.webview.postMessage
//!   browser  -> renderer "wv2.exec"   [id, skrypt]                 ExecuteScript
//!   renderer -> browser  "wv2.result" [id, json]                   wynik ExecuteScript
//! Skrypty AddScriptToExecuteOnDocumentCreated: plik `<--wv2-scripts-dir>/<browser_id>.scripts`
//! (rekordy: u32 LE długość + UTF-8), czytany przy każdym nowym kontekście JS — działa także
//! po zmianie procesu renderera przy nawigacji między witrynami.

use cef::rc::Rc as _;
use cef::*;
use std::path::PathBuf;

pub const MSG_POST: &str = "wv2.post";
pub const MSG_EXEC: &str = "wv2.exec";
pub const MSG_RESULT: &str = "wv2.result";
pub const SCRIPTS_DIR_SWITCH: &str = "wv2-scripts-dir";

/// Instaluje window.chrome.webview z użyciem natywnej funkcji __wv2_post (usuwanej zaraz potem).
const BOOTSTRAP: &str = r#"(function () {
  if (window.chrome && window.chrome.webview) return;
  var post = window.__wv2_post;
  delete window.__wv2_post;
  var target = new EventTarget();
  var webview = {
    postMessage: function (message) {
      var json = JSON.stringify(message);
      if (json === undefined) throw new TypeError("chrome.webview.postMessage: message is not serializable");
      post(json);
    },
    addEventListener: target.addEventListener.bind(target),
    removeEventListener: target.removeEventListener.bind(target),
    dispatchEvent: target.dispatchEvent.bind(target),
    __dispatch: function (data) {
      target.dispatchEvent(new MessageEvent("message", { data: data }));
    }
  };
  if (!window.chrome) window.chrome = {};
  Object.defineProperty(window.chrome, "webview", { value: webview, enumerable: true });
})();"#;

pub fn settings_file(dir: &str, browser_id: i32) -> PathBuf {
    PathBuf::from(dir).join(format!("{browser_id}.settings"))
}

/// Flagi WV2_SETTING_* zapisane przez proces przeglądarki (brak pliku = domyślne).
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

/// Kodowanie napisu jako literału JSON (poprawny także jako literał JS).
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
            _frame: Option<&mut Frame>,
            context: Option<&mut V8Context>,
        ) {
            let (Some(browser), Some(context)) = (browser, context) else { return };
            let Some(global) = context.global() else { return };
            let dir = command_line_get_global()
                .map(|cl| CefString::from(&cl.switch_value(Some(&SCRIPTS_DIR_SWITCH.into()))).to_string())
                .unwrap_or_default();
            let settings = if dir.is_empty() { crate::protocol::SETTINGS_DEFAULT } else { read_settings(&dir, browser.identifier()) };
            // IsWebMessageEnabled = FALSE: brak window.chrome.webview (jak w WebView2).
            if settings & crate::protocol::SETTING_WEB_MESSAGE != 0 {
                let mut handler = PostHandlerBuilder::new(PostHandler);
                let mut func = v8_value_create_function(Some(&"__wv2_post".into()), Some(&mut handler));
                global.set_value_bykey(Some(&"__wv2_post".into()), func.as_mut(), V8Propertyattribute::default());
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
            if CefString::from(&message.name()).to_string() != MSG_EXEC {
                return 0;
            }
            let Some(args) = message.argument_list() else { return 1 };
            let id = CefString::from(&args.string(0)).to_string();
            let script = CefString::from(&args.string(1)).to_string();
            let json = match frame.v8_context() {
                Some(context) if context.enter() != 0 => {
                    // Pośredni eval = zasięg globalny, jak w WebView2; wynik jako JSON ("null" dla undefined).
                    let wrapper = format!(
                        "(function(){{var r=(0,eval)({});var j=JSON.stringify(r);return j===undefined?'null':j;}})()",
                        json_string(&script)
                    );
                    let result = eval(&context, &wrapper);
                    context.exit();
                    match result {
                        Ok(Some(v)) if v.is_string() != 0 => CefString::from(&v.string_value()).to_string(),
                        Ok(_) => "null".to_string(),
                        Err(e) => {
                            eprintln!("helper(renderer): ExecuteScript exception: {e}");
                            "null".to_string()
                        }
                    }
                }
                _ => "null".to_string(),
            };
            send_to_browser(frame, MSG_RESULT, &[&id, &json]);
            1
        }
    }
}
