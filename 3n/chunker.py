#!/usr/bin/env python
# On Xeon E5620 @ 2.40 GHz chunk size 10^10 is about 15.5-15.6s
# Scales linearly: 10 min - chunk size ~384615384614
import sys

def main():
    start_num, chunk_size, num_chunks = map(int, sys.argv[1:])

    a = start_num
    b = start_num - 1
    cost = 0

    while num_chunks > 0:
        b += 1
        cost += b
        if cost >= chunk_size:
            print '%d %d' % (a, b);
            a = b + 1
            cost = 0
            num_chunks -= 1

if __name__ == '__main__':
    main()
