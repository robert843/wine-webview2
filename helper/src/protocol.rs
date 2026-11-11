//! Mirror of `dlls/embeddedbrowserwebview/protocol.h` - keep both files in sync.

#![allow(dead_code)]

pub const PROTOCOL_VERSION: u32 = 10;
pub const FRAME_SLOTS: u32 = 2;
/// Payloads larger than this go in a sealed memfd (protocol.h: WV2_INLINE_MAX).
pub const INLINE_MAX: usize = 64 * 1024;
pub const MAX_PAYLOAD: usize = 256 * 1024 * 1024;
pub const MSG_PAYLOAD_IN_FD: u32 = 0x1;
pub const HEADER_SIZE: usize = 16;

// DLL -> helper
pub const CMD_HELLO: u32 = 1;
pub const CMD_CREATE_VIEW: u32 = 2;
pub const CMD_CLOSE_VIEW: u32 = 3;
pub const CMD_RESIZE: u32 = 4;
pub const CMD_SET_VISIBLE: u32 = 5;
pub const CMD_NAVIGATE: u32 = 6;
pub const CMD_NAVIGATE_TO_STRING: u32 = 7;
pub const CMD_EXECUTE_SCRIPT: u32 = 8;
pub const CMD_ADD_DOCUMENT_SCRIPT: u32 = 9;
pub const CMD_FRAME_DONE: u32 = 10;
pub const CMD_SHUTDOWN: u32 = 11;
pub const CMD_POST_WEB_MESSAGE: u32 = 12;
pub const CMD_MOUSE: u32 = 13;
pub const CMD_KEY: u32 = 14;
pub const CMD_FOCUS: u32 = 15;
pub const CMD_ADD_RESOURCE_FILTER: u32 = 16;
pub const CMD_REMOVE_RESOURCE_FILTER: u32 = 17;
pub const CMD_RESOURCE_RESPONSE: u32 = 18;
pub const CMD_RESOURCE_DATA: u32 = 19;
pub const CMD_RESOURCE_END: u32 = 20;
pub const CMD_GO_BACK: u32 = 21;
pub const CMD_GO_FORWARD: u32 = 22;
pub const CMD_RELOAD: u32 = 23;
pub const CMD_STOP: u32 = 24;
pub const CMD_OPEN_DEVTOOLS: u32 = 25;
pub const CMD_SET_BACKGROUND: u32 = 26;
pub const CMD_SET_USER_AGENT: u32 = 27;
pub const CMD_SET_SETTINGS: u32 = 28;
pub const CMD_CALL: u32 = 29;
pub const CMD_INVOKE_REPLY: u32 = 30;
pub const CMD_RELEASE: u32 = 31;
pub const CMD_SCRIPT_DIALOG_REPLY: u32 = 32;
pub const CMD_CONTEXT_MENU_REPLY: u32 = 33;
pub const CMD_PERMISSION_REPLY: u32 = 34;
pub const CMD_FILE_DIALOG_REPLY: u32 = 35;
pub const CMD_HOST_OBJECT_REPLY: u32 = 36;

pub const SETTING_SCRIPT: u32 = 0x0001;
pub const SETTING_WEB_MESSAGE: u32 = 0x0002;
pub const SETTING_SCRIPT_DIALOGS: u32 = 0x0004;
pub const SETTING_CONTEXT_MENUS: u32 = 0x0008;
pub const SETTING_DEVTOOLS: u32 = 0x0010;
pub const SETTING_ZOOM: u32 = 0x0020;
pub const SETTING_ACCELERATORS: u32 = 0x0040;
pub const SETTINGS_DEFAULT: u32 = 0x007f;

// helper -> DLL
pub const EVT_HELLO: u32 = 0x1001;
pub const EVT_VIEW_CREATED: u32 = 0x1002;
pub const EVT_FRAME_BUFFER: u32 = 0x1003;
pub const EVT_FRAME_READY: u32 = 0x1004;
pub const EVT_LOAD_STATE: u32 = 0x1005;
pub const EVT_SCRIPT_RESULT: u32 = 0x1006;
pub const EVT_ERROR: u32 = 0x1007;
pub const EVT_WEB_MESSAGE: u32 = 0x1008;
pub const EVT_CURSOR: u32 = 0x1009;
pub const EVT_RESOURCE_REQUEST: u32 = 0x100a;
pub const EVT_NAVIGATION_STARTING: u32 = 0x100b;
pub const EVT_SOURCE_CHANGED: u32 = 0x100c;
pub const EVT_CONTENT_LOADING: u32 = 0x100d;
pub const EVT_NAVIGATION_COMPLETED: u32 = 0x100e;
pub const EVT_TITLE_CHANGED: u32 = 0x100f;
pub const EVT_HISTORY_CHANGED: u32 = 0x1010;
pub const EVT_REPLY: u32 = 0x1011;
pub const EVT_INVOKE: u32 = 0x1012;
pub const EVT_RELEASE_APP: u32 = 0x1013;
pub const EVT_SCRIPT_DIALOG: u32 = 0x1014;
pub const EVT_CONTEXT_MENU: u32 = 0x1015;
pub const EVT_PERMISSION_PROMPT: u32 = 0x1016;
pub const EVT_FILE_DIALOG: u32 = 0x1017;
pub const EVT_HOST_OBJECT_CALL: u32 = 0x1018;

pub const MOUSE_MOVE: u32 = 0;
pub const MOUSE_DOWN: u32 = 1;
pub const MOUSE_UP: u32 = 2;
pub const MOUSE_WHEEL: u32 = 3;
pub const MOUSE_LEAVE: u32 = 4;

#[derive(Default)]
pub struct Writer(pub Vec<u8>);

impl Writer {
    pub fn u32(mut self, v: u32) -> Self {
        self.0.extend_from_slice(&v.to_le_bytes());
        self
    }
    pub fn i32(mut self, v: i32) -> Self {
        self.0.extend_from_slice(&v.to_le_bytes());
        self
    }
    pub fn u64(mut self, v: u64) -> Self {
        self.0.extend_from_slice(&v.to_le_bytes());
        self
    }
    pub fn bytes(mut self, b: &[u8]) -> Self {
        self.0.extend_from_slice(b);
        self
    }
    pub fn f64(mut self, v: f64) -> Self {
        self.0.extend_from_slice(&v.to_le_bytes());
        self
    }
    /// u32 length + UTF-8 (the DLL's wv2_get_str).
    pub fn str(self, s: &str) -> Self {
        self.u32(s.len() as u32).bytes(s.as_bytes())
    }
}

pub struct Reader<'a>(pub &'a [u8]);

impl<'a> Reader<'a> {
    pub fn u32(&mut self) -> Option<u32> {
        let (a, b) = self.0.split_at_checked(4)?;
        self.0 = b;
        Some(u32::from_le_bytes(a.try_into().ok()?))
    }
    pub fn u64(&mut self) -> Option<u64> {
        let (a, b) = self.0.split_at_checked(8)?;
        self.0 = b;
        Some(u64::from_le_bytes(a.try_into().ok()?))
    }
    pub fn i32(&mut self) -> Option<i32> {
        self.u32().map(|v| v as i32)
    }
    pub fn u16(&mut self) -> Option<u16> {
        let (a, b) = self.0.split_at_checked(2)?;
        self.0 = b;
        Some(u16::from_le_bytes(a.try_into().ok()?))
    }
    pub fn f64(&mut self) -> Option<f64> {
        self.u64().map(f64::from_bits)
    }
    /// u32 length + UTF-8 (the DLL's wv2_put_str); 0xffffffff = NULL string, read as empty.
    pub fn str(&mut self) -> Option<String> {
        let len = self.u32()?;
        if len == u32::MAX {
            return Some(String::new());
        }
        self.take(len as usize).map(|b| String::from_utf8_lossy(b).into_owned())
    }
    pub fn take(&mut self, n: usize) -> Option<&'a [u8]> {
        let (a, b) = self.0.split_at_checked(n)?;
        self.0 = b;
        Some(a)
    }
    pub fn rest(&self) -> &'a [u8] {
        self.0
    }
    pub fn rest_str(&self) -> String {
        String::from_utf8_lossy(self.0).into_owned()
    }
}

pub struct Message {
    pub ty: u32,
    pub view: u32,
    pub payload: Vec<u8>,
}

/// Values of host object calls (protocol.h, WV2_VALUE_*).
pub const VALUE_UNDEFINED: u32 = 0;
pub const VALUE_NULL: u32 = 1;
pub const VALUE_BOOL: u32 = 2;
pub const VALUE_INT: u32 = 3;
pub const VALUE_DOUBLE: u32 = 4;
pub const VALUE_STRING: u32 = 5;
pub const VALUE_OBJECT: u32 = 6;
pub const VALUE_ARRAY: u32 = 7;
/// Host object operations (WV2_HOST_*).
pub const HOST_GET: u32 = 0;
pub const HOST_SET: u32 = 1;
pub const HOST_CALL: u32 = 2;
pub const HOST_RELEASE: u32 = 3;
