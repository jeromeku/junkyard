#!/usr/bin/env python
# Finds largest blobs by compressed size in git repository's packs

import sys, glob, struct

THRESHOLD = 1048576  # 1 Mb

def main():
    for filename in glob.glob('.git/objects/pack/*.idx'):
        f = file(filename, 'rb')
        assert f.read(4) == '\377tOc'  # only v2 is supported

        f.seek(8 + 255*4)
        num_blobs = struct.unpack('>i', f.read(4))[0]

        sys.stderr.write('%s: %d blobs\n' % (filename, num_blobs))

        sha_offs = 8 + 256*4
        crc_offs = sha_offs + 20 * num_blobs
        offs32_offs = crc_offs + 4 * num_blobs

        f.seek(sha_offs)
        shas = f.read(20 * num_blobs)

        f.seek(offs32_offs)
        offs32 = f.read(4 * num_blobs)
        offs64 = f.read(8 * num_blobs)

        blobs = []
        for i in xrange(num_blobs):
            k = struct.unpack('>I', offs32[4*i:4*i+4])[0]
            if (k & 0x80000000) != 0:
                k ^= 0x80000000
                k = struct.unpack('>Q', offs64[8*k:8*k+8])[0]
            blobs.append((k, i))

        blobs.sort()

        for i in xrange(len(blobs) - 1):
            size = blobs[i+1][0] - blobs[i][0]
            if size >= THRESHOLD:
                sha = ''.join('%.2x' % ord(c) for c in shas[20*blobs[i][1]:20*blobs[i][1]+20])
                print '%s\t%d' % (sha, size)

if __name__ == '__main__':
    main()
