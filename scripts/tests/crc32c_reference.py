#!/usr/bin/env python3
"""OOT D.E.: CRC32C (Castagnoli) of a file, written independently of the Java code, as a reference for its tests.

Same algorithm as soh/soh/Extractor/FastCrc32C.c (table version): start 0xFFFFFFFF, reflected polynomial 0x82F63B78,
inverted result. Prints the checksum in hex.
"""
import sys

TABLE = []
for i in range(256):
    c = i
    for _ in range(8):
        c = (c >> 1) ^ 0x82F63B78 if c & 1 else c >> 1
    TABLE.append(c)


def crc32c(data):
    crc = 0xFFFFFFFF
    for b in data:
        crc = TABLE[(crc ^ b) & 0xFF] ^ (crc >> 8)
    return crc ^ 0xFFFFFFFF


if __name__ == "__main__":
    assert crc32c(b"123456789") == 0xE3069283
    with open(sys.argv[1], "rb") as f:
        print(f"{crc32c(f.read()):08X}")
