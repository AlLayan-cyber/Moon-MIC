"""Extract embedded raster images from a PDF (no external renderer needed).

Handles /DCTDecode (JPEG, written as-is) and /FlateDecode (8-bit Gray/RGB/CMYK, decoded with zlib).
Vector graphics are NOT extracted. Read-only on the input PDF.

Usage: .venv/Scripts/python.exe tools/manufacturer/extract_pdf_images.py <pdf> <out_dir> [min_side_px]
"""
from __future__ import annotations

import re
import sys
import zlib
from pathlib import Path

from PIL import Image

OBJ_RE = re.compile(rb"(\d+)\s+0\s+obj(.*?)endobj", re.S)


def num(d: bytes, key: bytes) -> int | None:
    m = re.search(rb"/" + key + rb"\s+(\d+)", d)
    return int(m.group(1)) if m else None


def main() -> int:
    pdf, out = Path(sys.argv[1]), Path(sys.argv[2])
    min_side = int(sys.argv[3]) if len(sys.argv) > 3 else 150
    out.mkdir(parents=True, exist_ok=True)
    data = pdf.read_bytes()
    n = 0
    for m in OBJ_RE.finditer(data):
        body = m.group(2)
        if b"/Subtype" not in body or not re.search(rb"/Subtype\s*/Image", body):
            continue
        s = body.find(b"stream")
        if s < 0:
            continue
        s = body.find(b"\n", s) + 1
        e = body.rfind(b"endstream")
        raw = body[s:e].rstrip(b"\r\n")
        head = body[:s]
        w, h, bpc = num(head, b"Width"), num(head, b"Height"), num(head, b"BitsPerComponent")
        if not w or not h or min(w, h) < min_side:
            continue
        name = f"obj{m.group(1).decode()}_{w}x{h}"
        try:
            if b"/DCTDecode" in head:
                (out / f"{name}.jpg").write_bytes(raw)
            elif b"/FlateDecode" in head and bpc == 8:
                px = zlib.decompress(raw)
                cs = {w * h: "L", w * h * 3: "RGB", w * h * 4: "CMYK"}.get(len(px))
                if cs is None:
                    continue
                Image.frombytes(cs, (w, h), px).convert("RGB").save(out / f"{name}.png")
            else:
                continue
            n += 1
        except Exception as exc:  # malformed stream: skip, report
            print(f"skip {name}: {exc}")
    print(f"{n} images -> {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
