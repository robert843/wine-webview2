//! Lustro `dlls/embeddedbrowserwebview/protocol.h` — trzymać oba pliki w zgodzie.

#![allow(dead_code)]

pub const PROTOCOL_VERSION: u32 = 1;
pub const FRAME_SLOTS: u32 = 2;
pub const MAX_MESSAGE: usize = 1024 * 1024;
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

// helper -> DLL
pub const EVT_HELLO: u32 = 0x1001;
pub const EVT_VIEW_CREATED: u32 = 0x1002;
pub const EVT_FRAME_BUFFER: u32 = 0x1003;
pub const EVT_FRAME_READY: u32 = 0x1004;
pub const EVT_LOAD_STATE: u32 = 0x1005;
pub const EVT_SCRIPT_RESULT: u32 = 0x1006;
pub const EVT_ERROR: u32 = 0x1007;

/// Prosty zapis pól little-endian do bufora komunikatu.
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
}

/// Odczyt pól little-endian z ładunku komunikatu.
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
    pub fn rest_str(&self) -> String {
        String::from_utf8_lossy(self.0).into_owned()
    }
}

pub struct Message {
    pub ty: u32,
    pub view: u32,
    pub payload: Vec<u8>,
}
