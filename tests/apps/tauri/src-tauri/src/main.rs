// Test wine-webview2: aplikacja Tauri 2 (wry -> WebView2) zbudowana pod Windows.
// Sprawdza: zasoby z własnego schematu, invoke (IPC JS -> Rust), zdarzenia Rust -> JS, zamknięcie.
use tauri::{Emitter, Manager};

#[tauri::command]
fn greet(name: &str) -> String {
    eprintln!("tauri: greet({name})");
    format!("Hello, {name}! (z Rusta)")
}

#[tauri::command]
fn report(msg: &str) {
    eprintln!("tauri: report: {msg}");
}

#[tauri::command]
fn ping_event(app: tauri::AppHandle) {
    eprintln!("tauri: ping_event");
    app.emit("pong", serde_json::json!({ "value": 42 })).unwrap();
}

#[tauri::command]
fn finish(app: tauri::AppHandle) {
    eprintln!("tauri: finish");
    app.exit(0);
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![greet, report, ping_event, finish])
        .setup(|app| {
            eprintln!("tauri: setup, windows: {:?}", app.webview_windows().keys().collect::<Vec<_>>());
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("tauri run failed");
}
