#!/usr/bin/env python
import os, sys, optparse, socket

def main():
    os.chdir('/var/tmp/zealot')

    hostname = socket.gethostname()
    assert hostname.startswith('dech')
    hostid = int(hostname[4:7])

    chunks = file('chunks.txt').readlines()
    chunks = [s for (i, s) in enumerate(chunks) if i % 100 == hostid]
    print len(chunks)

    makefilename = 'Makefile.%s' % hostid

    f = file(makefilename, 'w')
    f.write('all: ' + ' '.join(['out.%s-%s' % tuple(ch.split()) for ch in chunks]) + '\n')
    for ch in chunks:
        a, b = ch.split()
        f.write('out.%s-%s:\n\t./p %s %s >out.%s-%s\n' % (a, b, a, b, a, b))
    f.close()

    os.system('nice -20 nohup gmake -j 4 -f %s all >gmake.out 2>gmake.err &' % makefilename)

if __name__ == '__main__':
    main()
