#!/usr/bin/env python
import sys

def orbits(elements, actions):
    def orbit(e):
        stk = [e]
        res = set([e])

        while len(stk) > 0:
            x = stk.pop()
            yield x

            for act in actions:
                y = act(x)
                if y not in res:
                    stk.append(y)
                    res.add(y)

    seen = set()
    for x in elements:
        if x not in seen:
            orb = list(sorted(orbit(x)))
            for y in orb:
                seen.add(y)
            yield orb

def grid_orbits(N):
    def points(N):
        xy = [(x+1, y+1) for x in range(N) for y in range(N)]
        return [(p, q) for p in xy for q in xy]

    def symmetries(N):
        def rot(p):
            return (p[1], N+1-p[0])

        def flip(p):
            return (p[0], N+1-p[1])

        def rot2(pq):
            return tuple(map(rot, pq))

        def flip2(pq):
            return tuple(map(flip, pq))

        def swap2(pq):
            return (pq[1], pq[0])

        return [ swap2, rot2, flip2 ]

    return orbits(points(N), symmetries(N))

if __name__ == '__main__':
    assert len(sys.argv) == 2, 'Specify N'
    N = int(sys.argv[1])
    for orbit in grid_orbits(N):
        print orbit
