#!/usr/bin/env python3
"""Creates symlinks with the letter case used by #include (xwin writes some headers in
lower case and Linux file systems are case sensitive).
    fix-case.py <include_dir> [<source_dir>...]"""
import os, re, sys

root = sys.argv[1]
pattern = re.compile(rb'#\s*include\s*[<"]([^>"]+)[>"]')
wanted = set()
for top in [root] + sys.argv[2:]:
    for dirpath, _, files in os.walk(top):
        for f in files:
            if f.lower().endswith((".h", ".hpp", ".cpp", ".inl", ".idl", "")):
                try:
                    data = open(os.path.join(dirpath, f), "rb").read()
                except OSError:
                    continue
                for m in pattern.finditer(data):
                    wanted.add(m.group(1).decode(errors="ignore").replace("\\", "/"))

# lower-case path -> existing path
index = {}
for dirpath, _, files in os.walk(root):
    for f in files:
        rel = os.path.relpath(os.path.join(dirpath, f), root)
        index.setdefault(rel.lower(), rel)

made = 0
for inc in wanted:
    if os.path.exists(os.path.join(root, inc)):
        continue
    real = index.get(inc.lower())
    if not real:
        continue
    dst = os.path.join(root, inc)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    rel = os.path.relpath(os.path.join(root, real), os.path.dirname(dst))
    try:
        os.symlink(rel, dst)
        made += 1
    except FileExistsError:
        pass
print(f"fix-case: {made} symlinks in {root}")
