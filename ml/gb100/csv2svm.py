#!/usr/bin/env python
# Converts forestfires.csv to libsvm format
import sys

def scale(s, values):
    res = [int(s == v) for v in values]
    assert sum(res) == 1
    return res

for line in sys.stdin.readlines()[1:]:
    vec = line.strip().split(',')
    y = float(vec[12])
    x = map(float,
                vec[0:2] +
                scale(vec[2], 'apr aug dec feb jan jul jun mar may nov oct sep'.split()) +
                scale(vec[3], 'mon tue wed thu fri sat sun'.split()) +
                vec[4:12])
    print '%.9g ' % y + ' '.join('%d:%.9g' % (i+1, z) for (i, z) in enumerate(x) if z != 0)
