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
    p.add_argument('--bootloader-offset', default=None, help='bootloader offset (default depends on --arch: esp32 -> 0x1000, esp32s3 -> 0x0)')
    p.add_argument('--partitions', help='path to partitions.bin')
    p.add_argument('--partitions-offset', default=None, help='partitions offset (default depends on --arch: 0x8000)')
    p.add_argument('--app', help='path to app/firmware bin (e.g. firmware.bin, app.bin)')
    p.add_argument('--app-offset', default=None, help='app offset (default depends on --arch: 0x10000)')
    p.add_argument('--arch', choices=['esp32', 'esp32s3'], default=None,
                   help='Target architecture; when set, default offsets are: esp32 -> bootloader 0x1000, esp32s3 -> bootloader 0x0')
    p.add_argument('-o', '--output', default='combined.bin', help='output file')

    args = p.parse_args()
    # If arch not provided, try to infer from filenames (simple heuristic)
    if args.arch is None and args.bootloader:
        bl = args.bootloader.lower()
        if 's3' in bl or 'esp32s3' in bl:
            args.arch = 'esp32s3'
        else:
            args.arch = 'esp32'

    # If user didn't provide specific offsets, choose sensible defaults by architecture
    if args.bootloader_offset is None:
        args.bootloader_offset = '0x1000' if args.arch == 'esp32' else '0x0'
    if args.partitions_offset is None:
        args.partitions_offset = '0x8000'
    if args.app_offset is None:
        args.app_offset = '0x10000'
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
