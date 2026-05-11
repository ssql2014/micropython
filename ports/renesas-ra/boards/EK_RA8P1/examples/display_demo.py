# GLCDC display demo using the ra8p1_display module.
# Paints a moving 8-band color stripe with diagonal brightness sweep,
# double-buffered via flip(). Mirrors the C-side bring-up animation but
# from Python.
#
# Pixel format is XRGB8888 (32 bits per pixel: 0x00RRGGBB).

import time
import ra8p1_display as disp

BANDS = (
    0x00FF0000,  # red
    0x00FF8000,  # orange
    0x00FFFF00,  # yellow
    0x0000FF00,  # green
    0x0000FFFF,  # cyan
    0x000000FF,  # blue
    0x008000FF,  # indigo
    0x00FF00FF,  # magenta
)

def paint(buf, phase):
    W = disp.WIDTH
    H = disp.HEIGHT
    band_px = W // 8
    for y in range(H):
        row_base = y * (disp.STRIDE // 4)
        for x in range(W):
            band = ((x + phase) // band_px) & 7
            c = BANDS[band]
            s = ((x + y + (phase << 1)) >> 2) & 0xFF
            shade = (s * 2) if s < 128 else ((255 - s) * 2)
            r = (((c >> 16) & 0xFF) * shade) >> 8
            g = (((c >> 8) & 0xFF) * shade) >> 8
            b = ((c & 0xFF) * shade) >> 8
            buf[row_base + x] = (r << 16) | (g << 8) | b

def main(frames=60):
    disp.init()
    fb0 = disp.framebuffer(0)
    fb1 = disp.framebuffer(1)
    paint(fb0, 0)
    disp.flip(0)
    for f in range(1, frames):
        back = fb1 if (f & 1) else fb0
        paint(back, f * 6)
        disp.flip(1 if (f & 1) else 0)
        time.sleep_ms(15)

if __name__ == "__main__":
    main()
