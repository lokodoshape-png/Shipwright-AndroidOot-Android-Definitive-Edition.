#!/usr/bin/env python3
"""OOT D.E.: add the custom textures that are missing from the prebuilt soh.otr of the Android app.

The Android build ships Android/app/src/main/assets/soh.otr as a committed file; it is not generated from
soh/assets/custom/ by the build. This script copies every file of that archive as it is and adds, as OTR texture
resources, the RGBA32 PNGs of soh/assets/custom/ that it does not have yet (the same resources that OTRExporter
writes for them). Run it after adding a texture under soh/assets/custom/:

    python3 scripts/otr_add_custom.py            # update the archive
    python3 scripts/otr_add_custom.py --check    # only list what is missing; exit 1 if anything is

It needs Pillow.
"""
import argparse
import bz2
import os
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OTR = os.path.join(ROOT, "Android", "app", "src", "main", "assets", "soh.otr")
CUSTOM = os.path.join(ROOT, "soh", "assets", "custom")

# --- MPQ (the format of .otr files) ---------------------------------------------------------------------------------

FILE_EXISTS = 0x80000000
FILE_SINGLE_UNIT = 0x01000000
FILE_SECTOR_CRC = 0x04000000
FILE_COMPRESS = 0x00000200
FILE_IMPLODE = 0x00000100
FILE_ENCRYPTED = 0x00010000


def _crypt_table():
    table = [0] * 0x500
    seed = 0x00100001
    for i in range(0x100):
        index = i
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            hi = (seed & 0xFFFF) << 16
            seed = (seed * 125 + 3) % 0x2AAAAB
            table[index] = hi | (seed & 0xFFFF)
            index += 0x100
    return table


TABLE = _crypt_table()


def mpq_hash(name, kind):
    s1, s2 = 0x7FED7FED, 0xEEEEEEEE
    for ch in name.upper().encode():
        s1 = (TABLE[(kind << 8) + ch] ^ (s1 + s2)) & 0xFFFFFFFF
        s2 = (ch + s1 + s2 + (s2 << 5) + 3) & 0xFFFFFFFF
    return s1


def _crypt(words, key, decrypt):
    s2 = 0xEEEEEEEE
    out = []
    for v in words:
        s2 = (s2 + TABLE[0x400 + (key & 0xFF)]) & 0xFFFFFFFF
        r = (v ^ (key + s2)) & 0xFFFFFFFF
        out.append(r)
        key = ((((~key) << 0x15) + 0x11111111) | (key >> 0x0B)) & 0xFFFFFFFF
        s2 = ((r if decrypt else v) + s2 + (s2 << 5) + 3) & 0xFFFFFFFF
    return out


def read_mpq(path):
    data = open(path, "rb").read()
    if data[:4] != b"MPQ\x1a":
        sys.exit(f"{path}: not an MPQ archive")
    _, _, _, _, shift, ht_pos, bt_pos, ht_n, bt_n = struct.unpack("<4sIIHHIIII", data[:32])
    sector = 512 << shift
    ht = _crypt(struct.unpack(f"<{ht_n * 4}I", data[ht_pos:ht_pos + ht_n * 16]), mpq_hash("(hash table)", 3), True)
    bt = _crypt(struct.unpack(f"<{bt_n * 4}I", data[bt_pos:bt_pos + bt_n * 16]), mpq_hash("(block table)", 3), True)

    def block_of(name):
        a, b = mpq_hash(name, 1), mpq_hash(name, 2)
        i = mpq_hash(name, 0) % ht_n
        for _ in range(ht_n):
            e = ht[i * 4:i * 4 + 4]
            if e[3] == 0xFFFFFFFF:
                return None
            if e[0] == a and e[1] == b and e[3] != 0xFFFFFFFE:
                return bt[e[3] * 4:e[3] * 4 + 4]
            i = (i + 1) % ht_n
        return None

    def raw_block(name):
        blk = block_of(name)
        if blk is None:
            return None
        pos, csize, fsize, flags = blk
        if flags & (FILE_ENCRYPTED | FILE_IMPLODE):
            sys.exit(f"{name}: encrypted or imploded files are not supported")
        return data[pos:pos + csize], fsize, flags

    def unpack(name):
        raw, fsize, flags = raw_block(name)
        if not flags & FILE_COMPRESS:
            return raw[:fsize]
        if flags & FILE_SINGLE_UNIT:
            return raw if len(raw) >= fsize else _decompress(raw)
        count = (fsize + sector - 1) // sector
        offs = struct.unpack(f"<{count + 1}I", raw[:4 * (count + 1)])
        out = b""
        for k in range(count):
            part = raw[offs[k]:offs[k + 1]]
            out += part if len(part) >= min(sector, fsize - k * sector) else _decompress(part)
        return out

    names = unpack("(listfile)").decode().split()
    files = {}
    for name in names:
        blk = raw_block(name)
        if blk is not None:
            files[name] = blk
    return shift, files


def _decompress(part):
    if part[0] == 0x02:
        return zlib.decompress(part[1:])
    if part[0] == 0x10:
        return bz2.decompress(part[1:])
    sys.exit(f"unsupported MPQ compression {part[0]:#x}")


def write_mpq(path, shift, files):
    """files: name -> (raw bytes, file size, flags). Writes a version 0 archive."""
    names = sorted(files)
    ht_n = 1
    while ht_n < (len(names) + 1) * 2:
        ht_n *= 2
    ht_n = max(ht_n, 4096)
    body = bytearray()
    blocks = []
    pos = 32
    for name in names:
        raw, fsize, flags = files[name]
        blocks.append((pos + len(body), len(raw), fsize, flags))
        body += raw
    ht = [0xFFFFFFFF] * (ht_n * 4)
    for index, name in enumerate(names):
        i = mpq_hash(name, 0) % ht_n
        while ht[i * 4 + 3] != 0xFFFFFFFF:
            i = (i + 1) % ht_n
        # Locale 0, platform 0.
        ht[i * 4:i * 4 + 4] = [mpq_hash(name, 1), mpq_hash(name, 2), 0, index]
    bt = [w for b in blocks for w in b]
    ht_pos = 32 + len(body)
    bt_pos = ht_pos + ht_n * 16
    size = bt_pos + len(blocks) * 16
    out = bytearray(struct.pack("<4sIIHHIIII", b"MPQ\x1a", 32, size, 0, shift, ht_pos, bt_pos, ht_n, len(blocks)))
    out += body
    out += struct.pack(f"<{len(ht)}I", *_crypt(ht, mpq_hash("(hash table)", 3), False))
    out += struct.pack(f"<{len(bt)}I", *_crypt(bt, mpq_hash("(block table)", 3), False))
    tmp = path + ".tmp"
    with open(tmp, "wb") as f:
        f.write(out)
    os.replace(tmp, path)


# --- OTR texture resource (as written by OTRExporter_Texture) ----------------------------------------------------

TEXTURE_RGBA32 = 1


def texture_resource(png):
    from PIL import Image

    image = Image.open(png).convert("RGBA")
    w, h = image.size
    pixels = image.tobytes()
    header = bytearray(64)
    header[0] = 0  # little endian
    header[4:8] = b"XETO"  # resource type OTEX
    struct.pack_into("<I", header, 8, 0)  # version
    struct.pack_into("<Q", header, 12, 0xDEADBEEFDEADBEEF)  # id
    return bytes(header) + struct.pack("<IIII", TEXTURE_RGBA32, w, h, len(pixels)) + pixels


def missing_textures(files):
    missing = []
    for folder, _, names in os.walk(CUSTOM):
        for fn in sorted(names):
            parts = fn.split(".")
            if len(parts) < 3 or parts[-1] != "png":
                continue
            rel = os.path.relpath(os.path.join(folder, fn), CUSTOM).replace(os.sep, "/")
            name = rel.rsplit(".", 2)[0]
            if name not in files:
                if parts[-2] != "rgba32":
                    sys.exit(f"{rel}: only rgba32 PNGs can be added by this script")
                missing.append((name, os.path.join(folder, fn)))
    return missing


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="only report missing textures")
    parser.add_argument("--otr", default=OTR)
    args = parser.parse_args()

    shift, files = read_mpq(args.otr)
    missing = missing_textures(files)
    for name, _ in missing:
        print(("missing: " if args.check else "adding: ") + name)
    if args.check:
        sys.exit(1 if missing else 0)
    if not missing:
        print("soh.otr already has every custom texture.")
        return
    for name, png in missing:
        res = texture_resource(png)
        files[name] = (res, len(res), FILE_EXISTS | FILE_SINGLE_UNIT)
    # The attributes hold checksums of the old blocks; StormLib rebuilds without them.
    files.pop("(attributes)", None)
    listing = "\r\n".join(sorted(n for n in files if n != "(listfile)")).encode() + b"\r\n"
    files["(listfile)"] = (listing, len(listing), FILE_EXISTS | FILE_SINGLE_UNIT)
    write_mpq(args.otr, shift, files)
    print(f"wrote {args.otr} ({len(files)} files)")


if __name__ == "__main__":
    main()
