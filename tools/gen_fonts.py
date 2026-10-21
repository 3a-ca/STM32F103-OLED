# -*- coding: utf-8 -*-
"""Generate OLED dot-matrix fonts and animated bitmaps as C arrays.

ASCII : SimSun 16 px -> 8x16 cell, 16 bytes per glyph (one byte per row, MSB = leftmost pixel)
CJK   : SimSun 16 px -> 16x16 cell, 32 bytes per glyph (2 bytes per row)
Avatar: 48x48, 3 frames (open / closed / wink), page-major like u8g2 XBM
"""
import io
import os
from PIL import Image, ImageDraw, ImageFont

FONT_PATH = "C:/Windows/Fonts/simsun.ttc"
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "Core", "Src")

CJK_CHARS = (
    "欢迎来到重庆交通大学物联网实训室"
    "姓名覃晖杰学号班级电信类"
    "嵌入式系统设计图形库移植演示"
    "图案动画温度湿未连接"
    "水平垂直滑动"
    "\uff01"
)

font = ImageFont.truetype(FONT_PATH, 16)


def rows_bits(ch, w, h, thresh=110):
    im = Image.new("L", (w, h), 0)
    ImageDraw.Draw(im).text((0, 0), ch, font=font, fill=255)
    px = im.load()
    out = []
    for y in range(h):
        b = 0
        for x in range(w):
            if px[x, y] > thresh:
                b |= 0x80 >> x
        out.append(b)
    return out


def cjk_bits(ch):
    im = Image.new("L", (16, 16), 0)
    ImageDraw.Draw(im).text((0, 0), ch, font=font, fill=255)
    px = im.load()
    out = []
    for y in range(16):
        hi = 0
        lo = 0
        for x in range(8):
            if px[x, y] > 110:
                hi |= 0x80 >> x
        for x in range(8, 16):
            if px[x, y] > 110:
                lo |= 0x80 >> (x - 8)
        out.append(hi)
        out.append(lo)
    return out


def emit(f, arr, per_line=12):
    for i in range(0, len(arr), per_line):
        f.write("    " + " ".join("0x%02X," % b for b in arr[i:i + per_line]) + "\n")


ascii_data = []
for code in range(0x20, 0x7F):
    ascii_data.extend(rows_bits(chr(code), 8, 16))

with io.open(os.path.join(SRC, "oled_font8x16.c"), "w", encoding="utf-8", newline="\n") as f:
    f.write("/* Auto-generated: 8x16 ASCII dot matrix (SimSun 16px, 0x20..0x7E). */\n")
    f.write('#include "oled_fonts.h"\n\n')
    f.write("const uint8_t oled_ascii8x16[95 * 16] = {\n")
    emit(f, ascii_data)
    f.write("};\n")

chars = []
for ch in CJK_CHARS:
    if ch not in chars:
        chars.append(ch)

glyphs = [(ch, cjk_bits(ch)) for ch in chars]
with io.open(os.path.join(SRC, "oled_fontcjk.c"), "w", encoding="utf-8", newline="\n") as f:
    f.write("/* Auto-generated: 16x16 CJK dot matrix, %d glyphs used by the UI. */\n" % len(glyphs))
    f.write('#include "oled_fonts.h"\n\n')
    for idx, (ch, data) in enumerate(glyphs):
        f.write("static const uint8_t cjk_%02d[32] = {   /* '%s' */\n" % (idx, ch))
        emit(f, data, per_line=16)
        f.write("};\n\n")
    f.write("const oled_cjk_glyph_t oled_cjk_table[%d] = {\n" % len(glyphs))
    for idx, (ch, _) in enumerate(glyphs):
        f.write("    {0x%04X, cjk_%02d},\n" % (ord(ch), idx))
    f.write("};\n\n")
    f.write("const uint16_t oled_cjk_count = %d;\n" % len(glyphs))


def avatar(eyes):
    im = Image.new("1", (48, 48), 0)
    d = ImageDraw.Draw(im)
    # 脸
    d.ellipse([5, 7, 42, 44], outline=1)
    # 头发
    d.arc([7, 4, 40, 32], start=195, end=345, fill=1, width=4)
    # 眼睛：0 睁眼 / 1 全闭 / 2 单眼眨
    if eyes == 0:
        d.rectangle([14, 22, 18, 26], fill=1)
        d.rectangle([29, 22, 33, 26], fill=1)
    elif eyes == 1:
        d.line([14, 24, 18, 24], fill=1, width=2)
        d.line([29, 24, 33, 24], fill=1, width=2)
    else:
        d.rectangle([14, 22, 18, 26], fill=1)
        d.line([29, 24, 33, 24], fill=1, width=2)
    # 鼻子
    d.line([23, 29, 24, 32], fill=1)
    # 嘴
    d.arc([16, 30, 32, 40], start=20, end=160, fill=1)
    return im


def heart(size=16):
    im = Image.new("1", (size, size), 0)
    d = ImageDraw.Draw(im)
    d.ellipse([1, 2, 8, 9], fill=1)
    d.ellipse([8, 2, 15, 9], fill=1)
    d.polygon([(1, 7), (15, 7), (8, 15)], fill=1)
    return im


def xbm(im):
    w, h = im.size
    px = im.load()
    data = []
    for page in range((h + 7) // 8):
        for x in range(w):
            b = 0
            for bit in range(8):
                y = page * 8 + bit
                if y < h and px[x, y]:
                    b |= 1 << bit
            data.append(b)
    return data


frames = [xbm(avatar(0)), xbm(avatar(1)), xbm(avatar(2))]
hrt = xbm(heart())
with io.open(os.path.join(SRC, "oled_image.c"), "w", encoding="utf-8", newline="\n") as f:
    f.write("/* Auto-generated: 48x48 avatar (3 frames, blink) + 16x16 heart. */\n")
    f.write('#include "oled_fonts.h"\n\n')
    for i, fr in enumerate(frames):
        f.write("static const uint8_t avatar_f%d[48 * 6] = {\n" % i)
        emit(f, fr, per_line=16)
        f.write("};\n\n")
    f.write("const uint8_t *const oled_avatar_frames[3] = { avatar_f0, avatar_f1, avatar_f2 };\n\n")
    f.write("const uint8_t oled_heart16[16 * 2] = {\n")
    emit(f, hrt, per_line=16)
    f.write("};\n")

print("ascii bytes:", len(ascii_data), " cjk glyphs:", len(glyphs))
