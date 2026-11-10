#!/usr/bin/env python3
"""Test protokołu pomocnika bez Wine: udaje stronę uniksową EmbeddedBrowserWebView.

    tests/helper_ipc_test.py <wine-webview2-helper> <katalog CEF> <wyjście.png>
"""
import mmap
import os
import socket
import struct
import subprocess
import sys
import zlib

CMD_HELLO, CMD_CREATE_VIEW, CMD_NAVIGATE_TO_STRING, CMD_FRAME_DONE, CMD_SHUTDOWN = 1, 2, 7, 10, 11
EVT_HELLO, EVT_VIEW_CREATED, EVT_FRAME_BUFFER, EVT_FRAME_READY, EVT_LOAD_STATE = 0x1001, 0x1002, 0x1003, 0x1004, 0x1005
VERSION = 1


def send(sock, ty, view, payload=b""):
    sock.send(struct.pack("<IIII", ty, 16 + len(payload), view, 0) + payload)


def recv(sock):
    data, fds, _, _ = socket.recv_fds(sock, 1 << 20, 1)
    ty, size, view, _ = struct.unpack_from("<IIII", data)
    assert size == len(data), (size, len(data))
    return ty, view, data[16:], fds


def write_png(path, w, h, bgra):
    rows = b"".join(b"\0" + bytes(x for px in range(w) for x in (
        bgra[(y * w + px) * 4 + 2], bgra[(y * w + px) * 4 + 1], bgra[(y * w + px) * 4])) for y in range(h))

    def chunk(t, b):
        return struct.pack(">I", len(b)) + t + b + struct.pack(">I", zlib.crc32(t + b))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(rows)) + chunk(b"IEND", b""))


def main():
    helper, cef_dir, out = sys.argv[1:4]
    parent, child = socket.socketpair(socket.AF_UNIX, socket.SOCK_SEQPACKET)
    env = dict(os.environ, LD_LIBRARY_PATH=cef_dir, WINE_WEBVIEW2_CEF_DIR=cef_dir)
    proc = subprocess.Popen([helper, f"--ipc-fd={child.fileno()}"], pass_fds=(child.fileno(),), env=env)
    child.close()
    parent.settimeout(30)

    send(parent, CMD_HELLO, 0, struct.pack("<II", VERSION, os.getpid()))
    ty, _, payload, _ = recv(parent)
    assert ty == EVT_HELLO and struct.unpack_from("<I", payload)[0] == VERSION
    print("hello ok, helper pid", struct.unpack_from("<II", payload)[1])

    send(parent, CMD_CREATE_VIEW, 1, struct.pack("<IIII", 640, 400, 100, 0))
    html = "<body style='background:#1b5e20;color:#fff;font:36px sans-serif'><h1>IPC ✔ zażółć</h1></body>"
    buf = None
    loaded = False
    frames = 0
    while True:
        ty, view, payload, fds = recv(parent)
        if ty == EVT_VIEW_CREATED:
            print("view created")
            send(parent, CMD_NAVIGATE_TO_STRING, 1, html.encode())
        elif ty == EVT_LOAD_STATE:
            loaded = struct.unpack_from("<I", payload)[0] == 0
            print("load state:", "loaded" if loaded else "loading")
        elif ty == EVT_FRAME_BUFFER:
            w, h, stride, slots, slot_size, gen = struct.unpack_from("<IIIIQQ", payload)
            buf = (w, h, stride, slot_size, gen, mmap.mmap(fds[0], slot_size * slots, prot=mmap.PROT_READ))
            os.close(fds[0])
            print(f"frame buffer {w}x{h} stride {stride} slots {slots} gen {gen}")
        elif ty == EVT_FRAME_READY:
            gen, slot = struct.unpack_from("<QI", payload)
            frames += 1
            w, h, stride, slot_size, bgen, m = buf
            pixels = m[slot * slot_size: slot * slot_size + stride * h] if gen == bgen else None
            send(parent, CMD_FRAME_DONE, 1, struct.pack("<QII", gen, slot, 0))
            if loaded and pixels:
                write_png(out, w, h, pixels)
                print(f"frame {frames} (slot {slot}) saved to {out}")
                break
    send(parent, CMD_SHUTDOWN, 0)
    parent.close()
    proc.wait(timeout=30)
    print("helper exit code", proc.returncode)


main()
