//! Gniazdo SOCK_SEQPACKET do EmbeddedBrowserWebView.dll (strona uniksowa) i bufory klatek memfd.

use crate::protocol::*;
use std::io;
use std::os::fd::{AsRawFd, FromRawFd, OwnedFd, RawFd};

pub struct Channel {
    fd: OwnedFd,
}

impl Channel {
    /// # Safety
    /// `fd` musi być otwartym gniazdem przekazanym przez proces rodzica.
    pub unsafe fn from_raw(fd: RawFd) -> Self {
        // Nie dziedziczyć gniazda do podprocesów CEF.
        libc::fcntl(fd, libc::F_SETFD, libc::FD_CLOEXEC);
        Channel { fd: OwnedFd::from_raw_fd(fd) }
    }

    pub fn try_clone(&self) -> io::Result<Self> {
        Ok(Channel { fd: self.fd.try_clone()? })
    }

    /// Wysyła komunikat; treść większa niż INLINE_MAX idzie w zapieczętowanym memfd.
    pub fn send(&self, ty: u32, view: u32, payload: &[u8], pass_fd: Option<RawFd>) -> io::Result<()> {
        let big = payload.len() > INLINE_MAX;
        let payload_fd = if big { Some(payload_memfd(payload)?) } else { None };
        let flags = if big { MSG_PAYLOAD_IN_FD } else { 0 };
        let header = Writer::default().u32(ty).u32((HEADER_SIZE + payload.len()) as u32).u32(view).u32(flags).0;
        let mut iov = [
            libc::iovec { iov_base: header.as_ptr() as *mut _, iov_len: header.len() },
            libc::iovec { iov_base: payload.as_ptr() as *mut _, iov_len: payload.len() },
        ];
        // Kolejność deskryptorów: najpierw memfd z treścią (jeśli jest), potem dołączony.
        let fds: Vec<RawFd> = payload_fd.iter().map(|f| f.as_raw_fd()).chain(pass_fd).collect();
        let mut control = [0u8; 64];
        let mut msg: libc::msghdr = unsafe { std::mem::zeroed() };
        msg.msg_iov = iov.as_mut_ptr();
        msg.msg_iovlen = if payload.is_empty() || big { 1 } else { 2 };
        if !fds.is_empty() {
            let bytes = (fds.len() * std::mem::size_of::<RawFd>()) as u32;
            unsafe {
                msg.msg_control = control.as_mut_ptr() as *mut _;
                msg.msg_controllen = libc::CMSG_SPACE(bytes) as _;
                let cmsg = libc::CMSG_FIRSTHDR(&msg);
                (*cmsg).cmsg_level = libc::SOL_SOCKET;
                (*cmsg).cmsg_type = libc::SCM_RIGHTS;
                (*cmsg).cmsg_len = libc::CMSG_LEN(bytes) as _;
                std::ptr::copy_nonoverlapping(fds.as_ptr(), libc::CMSG_DATA(cmsg) as *mut RawFd, fds.len());
            }
        }
        loop {
            let ret = unsafe { libc::sendmsg(self.fd.as_raw_fd(), &msg, libc::MSG_NOSIGNAL) };
            if ret >= 0 {
                return Ok(());
            }
            let err = io::Error::last_os_error();
            if err.kind() != io::ErrorKind::Interrupted {
                return Err(err);
            }
        }
    }

    /// Blokuje do następnego komunikatu; `Ok(None)` = druga strona zamknęła połączenie.
    pub fn recv(&self) -> io::Result<Option<Message>> {
        let mut buf = vec![0u8; HEADER_SIZE + INLINE_MAX];
        let mut control = [0u8; 64];
        let mut iov = libc::iovec { iov_base: buf.as_mut_ptr() as *mut _, iov_len: buf.len() };
        let mut msg: libc::msghdr = unsafe { std::mem::zeroed() };
        msg.msg_iov = &mut iov;
        msg.msg_iovlen = 1;
        msg.msg_control = control.as_mut_ptr() as *mut _;
        msg.msg_controllen = control.len() as _;
        let ret = loop {
            let ret = unsafe { libc::recvmsg(self.fd.as_raw_fd(), &mut msg, libc::MSG_CMSG_CLOEXEC) };
            if ret >= 0 {
                break ret as usize;
            }
            let err = io::Error::last_os_error();
            if err.kind() != io::ErrorKind::Interrupted {
                return Err(err);
            }
        };
        if ret == 0 {
            return Ok(None);
        }
        let mut fds: Vec<OwnedFd> = Vec::new();
        unsafe {
            let mut cmsg = libc::CMSG_FIRSTHDR(&msg);
            while !cmsg.is_null() {
                if (*cmsg).cmsg_level == libc::SOL_SOCKET && (*cmsg).cmsg_type == libc::SCM_RIGHTS {
                    let n = ((*cmsg).cmsg_len as usize - libc::CMSG_LEN(0) as usize) / std::mem::size_of::<RawFd>();
                    let data = libc::CMSG_DATA(cmsg) as *const RawFd;
                    for i in 0..n {
                        fds.push(OwnedFd::from_raw_fd(std::ptr::read_unaligned(data.add(i))));
                    }
                }
                cmsg = libc::CMSG_NXTHDR(&msg, cmsg);
            }
        }
        let bad = |what: &str| io::Error::new(io::ErrorKind::InvalidData, what.to_string());
        if msg.msg_flags & (libc::MSG_TRUNC | libc::MSG_CTRUNC) != 0 {
            return Err(bad("truncated message"));
        }
        let mut r = Reader(&buf[..ret]);
        let (Some(ty), Some(size), Some(view), Some(flags)) = (r.u32(), r.u32(), r.u32(), r.u32()) else {
            return Err(bad("short message"));
        };
        let size = size as usize;
        if size < HEADER_SIZE {
            return Err(bad("bad message size"));
        }
        let payload = if flags & MSG_PAYLOAD_IN_FD != 0 {
            let fd = fds.first().ok_or_else(|| bad("missing payload descriptor"))?;
            if ret != HEADER_SIZE || size - HEADER_SIZE > MAX_PAYLOAD {
                return Err(bad("bad payload descriptor message"));
            }
            read_memfd(fd, size - HEADER_SIZE)?
        } else {
            if size != ret {
                return Err(bad("bad message size"));
            }
            buf[HEADER_SIZE..ret].to_vec()
        };
        Ok(Some(Message { ty, view, payload }))
    }
}

fn payload_memfd(data: &[u8]) -> io::Result<OwnedFd> {
    let raw = unsafe { libc::memfd_create(c"wine-webview2-msg".as_ptr(), libc::MFD_CLOEXEC | libc::MFD_ALLOW_SEALING) };
    if raw < 0 {
        return Err(io::Error::last_os_error());
    }
    let fd = unsafe { OwnedFd::from_raw_fd(raw) };
    let mut file = std::fs::File::from(fd.try_clone()?);
    std::io::Write::write_all(&mut file, data)?;
    unsafe {
        libc::fcntl(raw, libc::F_ADD_SEALS, libc::F_SEAL_SHRINK | libc::F_SEAL_GROW | libc::F_SEAL_WRITE | libc::F_SEAL_SEAL)
    };
    Ok(fd)
}

fn read_memfd(fd: &OwnedFd, size: usize) -> io::Result<Vec<u8>> {
    use std::os::unix::fs::FileExt;
    let file = std::fs::File::from(fd.try_clone()?);
    let mut data = vec![0u8; size];
    file.read_exact_at(&mut data, 0)?;
    Ok(data)
}

/// Bufor klatek: memfd z `FRAME_SLOTS` slotami BGRA, zmapowany do zapisu.
pub struct FrameBuffer {
    pub fd: OwnedFd,
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub slot_size: u64,
    pub generation: u64,
    base: *mut u8,
    len: usize,
    pub busy: [bool; FRAME_SLOTS as usize],
}

impl FrameBuffer {
    pub fn new(width: u32, height: u32, generation: u64) -> io::Result<Self> {
        let stride = width * 4;
        let slot_size = stride as u64 * height as u64;
        let len = (slot_size * FRAME_SLOTS as u64) as usize;
        let name = c"wine-webview2-frames";
        let raw = unsafe { libc::memfd_create(name.as_ptr(), libc::MFD_CLOEXEC) };
        if raw < 0 {
            return Err(io::Error::last_os_error());
        }
        let fd = unsafe { OwnedFd::from_raw_fd(raw) };
        if unsafe { libc::ftruncate(raw, len as libc::off_t) } < 0 {
            return Err(io::Error::last_os_error());
        }
        let base = unsafe {
            libc::mmap(std::ptr::null_mut(), len, libc::PROT_READ | libc::PROT_WRITE, libc::MAP_SHARED, raw, 0)
        };
        if base == libc::MAP_FAILED {
            return Err(io::Error::last_os_error());
        }
        Ok(FrameBuffer {
            fd,
            width,
            height,
            stride,
            slot_size,
            generation,
            base: base as *mut u8,
            len,
            busy: [false; FRAME_SLOTS as usize],
        })
    }

    pub fn slot_mut(&mut self, slot: usize) -> &mut [u8] {
        unsafe {
            std::slice::from_raw_parts_mut(self.base.add(slot * self.slot_size as usize), self.slot_size as usize)
        }
    }

    pub fn describe(&self) -> Vec<u8> {
        Writer::default()
            .u32(self.width)
            .u32(self.height)
            .u32(self.stride)
            .u32(FRAME_SLOTS)
            .u64(self.slot_size)
            .u64(self.generation)
            .0
    }
}

impl Drop for FrameBuffer {
    fn drop(&mut self) {
        unsafe { libc::munmap(self.base as *mut _, self.len) };
    }
}
