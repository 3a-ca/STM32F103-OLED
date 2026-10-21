# -*- coding: utf-8 -*-
"""Repair double-encoded Chinese in text files: the writer treated UTF-8 bytes as GBK,
so text now reads as mojibake.  Reversing  (s.encode('gbk').decode('utf-8')) restores it.
ASCII-only script so it survives the same toolchain.
"""
import io
import os
import sys

ROOT = r"C:\Users\30477\Documents\deepseek-harness\default-workspace\stm32_oled"
EXTS = (".c", ".h", ".py", ".md", ".txt", ".s", ".ld")


def repair(text):
    if not any(ord(ch) > 127 for ch in text):
        return text, False
    try:
        fixed = text.encode("gbk").decode("utf-8")
        return fixed, True
    except Exception:
        return text, False


changed = []
for dirpath, dirnames, filenames in os.walk(ROOT):
    if "Drivers" in dirpath.split(os.sep) or "build" in dirpath.split(os.sep):
        continue
    for name in filenames:
        if not name.endswith(EXTS):
            continue
        path = os.path.join(dirpath, name)
        raw = open(path, "rb").read()
        try:
            text = raw.decode("utf-8")
        except UnicodeDecodeError:
            text = raw.decode("gbk", "replace")
        fixed, ok = repair(text)
        if ok and fixed != text:
            open(path, "wb").write(fixed.encode("utf-8"))
            changed.append(name)

print("repaired:", len(changed))
for c in changed:
    print("  ", c)
