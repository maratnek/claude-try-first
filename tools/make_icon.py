#!/usr/bin/env python3
import math
import struct
import sys
import zlib

SIZE = 1024
SUB = 4


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def new_canvas():
    top, bottom = (28, 76, 160), (255, 190, 120)
    rows = []
    for y in range(SIZE):
        c = lerp(top, bottom, (y / (SIZE - 1)) ** 1.4)
        rows.append([[c[0]] * SIZE, [c[1]] * SIZE, [c[2]] * SIZE])
    return rows


def fill(rows, poly, color):
    ys = [p[1] for p in poly]
    y0, y1 = max(0, int(math.floor(min(ys)))), min(SIZE - 1, int(math.ceil(max(ys))))
    n = len(poly)
    for y in range(y0, y1 + 1):
        cov = {}
        for s in range(SUB):
            sy = y + (s + 0.5) / SUB
            xs = []
            for i in range(n):
                ax, ay = poly[i]
                bx, by = poly[(i + 1) % n]
                if (ay <= sy < by) or (by <= sy < ay):
                    xs.append(ax + (sy - ay) / (by - ay) * (bx - ax))
            xs.sort()
            for k in range(0, len(xs) - 1, 2):
                a, b = max(0.0, xs[k]), min(float(SIZE), xs[k + 1])
                if b <= a:
                    continue
                ia, ib = int(a), int(b)
                if ia == ib:
                    cov[ia] = cov.get(ia, 0.0) + (b - a) / SUB
                    continue
                cov[ia] = cov.get(ia, 0.0) + (ia + 1 - a) / SUB
                for x in range(ia + 1, ib):
                    cov[x] = cov.get(x, 0.0) + 1.0 / SUB
                if ib < SIZE:
                    cov[ib] = cov.get(ib, 0.0) + (b - ib) / SUB
        r, g, b_ = rows[y]
        for x, c in cov.items():
            c = min(c, 1.0)
            r[x] += (color[0] - r[x]) * c
            g[x] += (color[1] - g[x]) * c
            b_[x] += (color[2] - b_[x]) * c


def xform(points, angle, cx, cy, scale):
    ca, sa = math.cos(angle), math.sin(angle)
    return [(cx + (x * ca - y * sa) * scale, cy + (x * sa + y * ca) * scale) for x, y in points]


def rect(x0, y0, x1, y1):
    return [(x0, y0), (x1, y0), (x1, y1), (x0, y1)]


def draw_facets(rows):
    facets = [
        ([(0, 1024), (0, 700), (360, 1024)], (255, 160, 100)),
        ([(0, 700), (520, 1024), (360, 1024)], (240, 140, 100)),
        ([(1024, 1024), (1024, 640), (560, 1024)], (250, 170, 110)),
        ([(1024, 640), (700, 1024), (560, 1024)], (230, 135, 105)),
        ([(0, 0), (1024, 0), (0, 380)], (22, 62, 140)),
        ([(1024, 0), (1024, 300), (640, 0)], (36, 90, 176)),
        ([(0, 380), (300, 0), (0, 0)], (30, 72, 156)),
    ]
    for poly, color in facets:
        fill(rows, poly, color)


def draw_plane(rows):
    body = (34, 30, 52)
    accent = (200, 60, 50)
    angle = math.radians(-14)
    cx, cy, k = 512, 520, 1.0

    def T(pts):
        return xform(pts, angle, cx, cy, k)

    parts = [
        (T([(-300, -10), (-120, -70), (170, -60), (300, -10), (170, 50), (-120, 60)]), body),
        (T([(-300, -10), (-420, -20), (-440, -150), (-380, -150)]), accent),
        (T([(-300, 0), (-440, 20), (-410, 60), (-290, 30)]), body),
        (T([(-120, -70), (170, -60), (150, -90), (-100, -100)]), accent),
        (T(rect(-130, -190, 190, -160)), body),
        (T(rect(-100, 85, 170, 115)), body),
        (T(rect(-90, -160, -70, 85)), body),
        (T(rect(130, -160, 150, 85)), body),
        (T([(-60, -160), (60, 85), (80, 85), (-40, -160)]), body),
        (T([(300, -10), (330, -40), (345, -10), (330, 20)]), (235, 235, 240)),
        (T([(330, -230), (340, -230), (350, 220), (340, 220)]), (235, 235, 240)),
        (T([(-20, 50), (-10, 130), (30, 130), (40, 50)]), body),
        (T([(-60, 130), (-60, 160), (80, 160), (80, 130)]), (60, 56, 76)),
    ]
    for poly, color in parts:
        fill(rows, poly, color)


def write_png(path, rows):
    raw = bytearray()
    for r, g, b in rows:
        raw.append(0)
        for x in range(SIZE):
            raw += bytes((max(0, min(255, round(r[x]))), max(0, min(255, round(g[x]))), max(0, min(255, round(b[x])))))

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", SIZE, SIZE, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "ios/Assets.xcassets/AppIcon.appiconset/AppIcon-1024.png"
    rows = new_canvas()
    draw_facets(rows)
    draw_plane(rows)
    write_png(out, rows)


main()
