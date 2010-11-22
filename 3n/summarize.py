#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys

res = {}
checked = set()
total_elapsed = 0.0

for filename in sys.argv[1:]:
    for line in file(filename):
        line = line.strip()
        if line.startswith('elapsed'):
            total_elapsed += float(line.split()[1])
            continue

        if line.startswith('checked'):
            a, b = map(int, line.split()[1:])
            for n in range(a, b+1):
                checked.add(n)
            continue

        exp, zeroes, seconds = line.split()

        exp = int(exp)
        zeroes = int(zeroes)

        for z in range(1, zeroes+1):
            if (z not in res) or (exp < res[z][0]):
                res[z] = (exp, filename, seconds)

max_checked = 0
while (max_checked + 1) in checked:
    max_checked += 1

for key in sorted(res.keys()):
    if key >= 2:  # p.cc will miscompute n=1
        exp, filename, seconds = res[key]
        print '%s: %s %s' % (key, exp, ' *PRELIMINARY*' if (max_checked < exp) else '')

print 'checked up to n=%d' % max_checked
