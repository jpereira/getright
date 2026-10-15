#!/bin/sh
# SPDX-License-Identifier: LGPL-3.0-only
# Copyright (C) 2026 Jorge Pereira
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Generate macOS icon sizes from the application logo.

set -eu

source_image=$1
output_icon=$2
iconset="${output_icon%.icns}-v2.iconset"

mkdir -p "$iconset"
for size in 16 32 64 128 256 512; do
    double_size=$((size * 2))
    sips -s format png -z "$size" "$size" "$source_image" \
        --out "$iconset/icon_${size}x${size}.png" >/dev/null
    sips -s format png -z "$double_size" "$double_size" "$source_image" \
        --out "$iconset/icon_${size}x${size}@2x.png" >/dev/null
done
python3 "$(dirname "$0")/pack-icns.py" "$iconset" "$output_icon"
