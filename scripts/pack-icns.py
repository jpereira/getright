#!/usr/bin/env python3
# SPDX-License-Identifier: LGPL-3.0-only
# Copyright (C) 2026 Jorge Pereira
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Pack PNG icon sizes into a macOS ICNS file.

"""Pack PNG icon representations into a macOS ICNS container."""

from pathlib import Path
import struct
import sys


def main() -> None:
    iconset = Path(sys.argv[1])
    output = Path(sys.argv[2])
    representations = [
        (16, "icp4"),
        (32, "icp5"),
        (64, "icp6"),
        (128, "ic07"),
        (256, "ic08"),
        (512, "ic09"),
        (1024, "ic10"),
    ]
    chunks = []
    for size, chunk_type in representations:
        name = f"icon_{size}x{size}.png" if size < 128 else None
        if size == 1024:
            name = "icon_512x512@2x.png"
        elif size == 64:
            name = "icon_32x32@2x.png"
        elif size == 128:
            name = "icon_128x128.png"
        elif size == 256:
            name = "icon_256x256.png"
        elif size == 512:
            name = "icon_512x512.png"
        payload = (iconset / name).read_bytes()
        chunks.append(chunk_type.encode("ascii") +
                      struct.pack(">I", len(payload) + 8) + payload)
    body = b"".join(chunks)
    output.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)


if __name__ == "__main__":
    main()
