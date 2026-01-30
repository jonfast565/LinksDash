#!/usr/bin/env python3
"""Pack multiple PNG files into a single .ico (PNG-compressed) file."""
import struct
import sys
from pathlib import Path

PNG_SIG = b"\x89PNG\r\n\x1a\n"

def read_png(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    if not data.startswith(PNG_SIG):
        raise ValueError(f"{path} is not a PNG")
    width, height = struct.unpack(">II", data[16:24]
    )
    return width, height, data


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: png2ico.py output.ico input1.png [input2.png ...]", file=sys.stderr)
        return 2
    out_path = Path(sys.argv[1])
    png_paths = [Path(p) for p in sys.argv[2:]]

    images = []
    for path in png_paths:
        width, height, data = read_png(path)
        images.append((width, height, data))

    count = len(images)
    header = struct.pack("<HHH", 0, 1, count)
    dir_entries = []
    offset = 6 + (16 * count)

    for width, height, data in images:
        width_byte = 0 if width >= 256 else width
        height_byte = 0 if height >= 256 else height
        dir_entries.append(
            struct.pack(
                "<BBBBHHII",
                width_byte,
                height_byte,
                0,
                0,
                0,
                0,
                len(data),
                offset,
            )
        )
        offset += len(data)

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("wb") as f:
        f.write(header)
        for entry in dir_entries:
            f.write(entry)
        for _, _, data in images:
            f.write(data)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
