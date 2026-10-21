# -*- coding: utf-8 -*-
"""Upscale the simulated 128x64 PGM frames into readable PNGs."""
import os
from PIL import Image

root = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), "sim_out")
scale = 6
for name in sorted(os.listdir(root)):
    if not name.endswith(".pgm"):
        continue
    im = Image.open(os.path.join(root, name)).convert("L")
    big = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    # 反色：OLED 黑底白点，这里输出黑底白点更直观
    big.save(os.path.join(root, name.replace(".pgm", ".png")))
    print(name, im.size)
