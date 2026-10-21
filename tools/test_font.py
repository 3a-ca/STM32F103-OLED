# -*- coding: utf-8 -*-
"""Test glyph rasterisation from SimSun for the OLED dot-matrix fonts."""
from PIL import Image, ImageDraw, ImageFont

FONT = "C:/Windows/Fonts/simsun.ttc"
f16 = ImageFont.truetype(FONT, 16)

for ch in ["A", "g", "0", "重", "欢"]:
    im = Image.new("L", (20, 20), 0)
    d = ImageDraw.Draw(im)
    d.text((0, 0), ch, font=f16, fill=255)
    pix = im.load()
    rows = []
    for y in range(18):
        rows.append("".join("#" if pix[x, y] > 110 else "." for x in range(18)))
    print("char", ch, "bbox", d.textbbox((0, 0), ch, font=f16))
    print("\n".join(rows))
    print()
