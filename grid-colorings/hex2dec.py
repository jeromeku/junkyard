#!/usr/bin/env python
import sys

for line in sys.stdin:
    for value in line.strip().split():
        assert value.startswith('0x'), value
        print int(value[2:], 16)
