#!/usr/bin/env python3
import struct

with open('out1.dat', 'rb') as fp:
    d = fp.read(4)
    count, = struct.unpack('I', d)
    for i in range(count):
        x, y, z, c = struct.unpack('fffI', fp.read(16))
        print((x, y, z))
