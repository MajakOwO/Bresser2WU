#!/usr/bin/env python3
"""
Make a single flashable ESP32 image by placing bootloader, partition table
and application binaries at their flash offsets and filling gaps with 0xFF.

Typical offsets (Arduino/PlatformIO default):
 - bootloader: 0x1000
 - partitions: 0x8000
 - application: 0x10000

Usage:
 python scripts/make_esp32_combined.py \
   --bootloader .pio/build/esp32dev/bootloader.bin \
   --partitions .pio/build/esp32dev/partitions.bin \
   --app .pio/build/esp32dev/firmware.bin \
   -o combined.bin

Then flash with esptool (or provide `combined.bin` in GitHub release):
 esptool.py --chip esp32 write_flash 0x0 combined.bin

The script is conservative: it fills gaps with 0xFF (erased flash) and preserves
existing data ranges if offsets overlap (overwrites in order bootloader, parts, app).
"""
import argparse
import os
import sys


def read_file(path):
    with open(path, 'rb') as f:
        return f.read()


def write_combined(out_path, components):
    # components: list of (offset, data)
    end = 0
    for off, data in components:
        end = max(end, off + len(data))

    # Fill with 0xFF (erased flash state)
    buf = bytearray([0xFF]) * end

    for off, data in components:
        buf[off:off+len(data)] = data

    with open(out_path, 'wb') as f:
        f.write(buf)


def parse_hex(s):
    return int(s, 0)


def main():
    p = argparse.ArgumentParser(description='Create combined ESP32 flash image')
    p.add_argument('--bootloader', help='path to bootloader.bin')
    p.add_argument('--bootloader-offset', default='0x1000', help='bootloader offset (default 0x1000)')
    p.add_argument('--partitions', help='path to partitions.bin')
    p.add_argument('--partitions-offset', default='0x8000', help='partitions offset (default 0x8000)')
    p.add_argument('--app', help='path to app/firmware bin (e.g. firmware.bin, app.bin)')
    p.add_argument('--app-offset', default='0x10000', help='app offset (default 0x10000)')
    p.add_argument('-o', '--output', default='combined.bin', help='output file')

    args = p.parse_args()

    comps = []
    if args.bootloader:
        data = read_file(args.bootloader)
        comps.append((parse_hex(args.bootloader_offset), data))
    if args.partitions:
        data = read_file(args.partitions)
        comps.append((parse_hex(args.partitions_offset), data))
    if args.app:
        data = read_file(args.app)
        comps.append((parse_hex(args.app_offset), data))

    if not comps:
        print('Nothing to do: provide at least one of --bootloader, --partitions, --app', file=sys.stderr)
        sys.exit(2)

    # Sort by offset to apply in order (lower offsets first)
    comps.sort(key=lambda x: x[0])

    out = args.output
    write_combined(out, comps)
    print(f'Wrote combined image: {out} (size {os.path.getsize(out)} bytes)')


if __name__ == '__main__':
    main()
