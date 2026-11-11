//! Windows <-> unix path conversion. The application speaks Windows paths ("C:\users\..."),
//! the helper (native) needs unix ones. The DLL sends the drive map when the environment is created
//! (wine_get_unix_file_name of every drive root), so no Wine internals are guessed here.

use std::cell::RefCell;
use std::path::{Path, PathBuf};

thread_local! {
    /// (drive letter, unix directory of the drive root, canonical form for reverse lookups)
    static DRIVES: RefCell<Vec<(char, PathBuf, PathBuf)>> = const { RefCell::new(Vec::new()) };
}

/// Lines "C:/home/user/.wine/dosdevices/c:/".
pub fn set_drive_map(map: &str) {
    let drives = map
        .lines()
        .filter_map(|line| {
            let mut chars = line.chars();
            let letter = chars.next()?.to_ascii_uppercase();
            if !letter.is_ascii_alphabetic() || chars.next()? != ':' {
                return None;
            }
            let dir = PathBuf::from(chars.as_str());
            let canonical = std::fs::canonicalize(&dir).unwrap_or_else(|_| dir.clone());
            Some((letter, dir, canonical))
        })
        .collect::<Vec<_>>();
    if !drives.is_empty() {
        DRIVES.with(|d| *d.borrow_mut() = drives);
    }
}

/// "C:\dir\file.txt" -> "/.../dosdevices/c:/dir/file.txt". Without a drive map only Z: (= /) works.
pub fn to_unix(windows: &str) -> Option<PathBuf> {
    let path = windows.strip_prefix(r"\\?\").unwrap_or(windows);
    let mut chars = path.chars();
    let letter = chars.next()?.to_ascii_uppercase();
    if !letter.is_ascii_alphabetic() || chars.next()? != ':' {
        return None;
    }
    let rest = chars.as_str().trim_start_matches(['\\', '/']);
    let root = DRIVES
        .with(|d| d.borrow().iter().find(|(l, _, _)| *l == letter).map(|(_, dir, _)| dir.clone()))
        .or_else(|| (letter == 'Z').then(|| PathBuf::from("/")))?;
    let mut out = root;
    for part in rest.split(['\\', '/']).filter(|p| !p.is_empty()) {
        out.push(part);
    }
    Some(out)
}

/// "/home/user/file" -> "Z:\home\user\file" (or the most specific drive containing it).
pub fn to_windows(unix: &Path) -> String {
    // The file itself may not exist yet: canonicalise the directory.
    let canonical = match (unix.parent(), unix.file_name()) {
        (Some(dir), Some(name)) => std::fs::canonicalize(dir).map(|d| d.join(name)).unwrap_or_else(|_| unix.to_path_buf()),
        _ => unix.to_path_buf(),
    };
    let best = DRIVES.with(|d| {
        d.borrow()
            .iter()
            .filter_map(|(letter, _, root)| canonical.strip_prefix(root).ok().map(|rest| (root.as_os_str().len(), *letter, rest.to_path_buf())))
            .max_by_key(|(len, _, _)| *len)
    });
    let (letter, rest) = match best {
        Some((_, letter, rest)) => (letter, rest),
        None => ('Z', canonical.strip_prefix("/").map(Path::to_path_buf).unwrap_or(canonical.clone())),
    };
    let rest = rest.to_string_lossy().replace('/', "\\");
    format!("{letter}:\\{rest}")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn round_trip() {
        set_drive_map("C:/tmp/\nZ:/\n");
        assert_eq!(to_unix(r"C:\a\b.txt").unwrap(), PathBuf::from("/tmp/a/b.txt"));
        assert_eq!(to_unix(r"z:\home\x").unwrap(), PathBuf::from("/home/x"));
        assert_eq!(to_unix("relative"), None);
        let tmp = std::fs::canonicalize("/tmp").unwrap();
        assert_eq!(to_windows(&tmp.join("new.bin")), r"C:\new.bin");
    }
}
