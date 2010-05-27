#!/usr/bin/env python
# -*- coding: utf8 -*-
#
# A daemon that periodically parses downloaded files.xml.bz2
# and stores results in an sqlite database.
#
import os, sys, time, traceback, bz2, optparse, sqlite
from xml.sax import handler, make_parser

def log(msg):
    sys.stderr.write(time.ctime() + ' ' + msg)
    sys.stderr.flush()

class MyHandler(handler.ContentHandler):
    def __init__(self, callback):
        self.callback = callback

    def startElement(self, name, attrs):
        if name == u'Directory':
            filename = attrs[u'Name']
            ext = u'dir'
            size = 0
        elif name == u'File':
            filename = attrs[u'Name']
            components = filename.split(u'.')
            ext = u''
            if len(components) >= 2 and len(components[-1]) <= 8:
                ext = components[-1]
            size = int(attrs.get(u'Size', u'0'))
        else:
            return
        self.callback(filename, ext, size)

def parse(path, callback):
    bzfile = bz2.BZ2File(path, 'r')
    parser = make_parser()
    parser.setContentHandler(MyHandler(callback))
    parser.parse(bzfile)

def reindex(options):
    temp_file = os.path.join(options.dir, 'index.sqlite.temp')
    final_file = os.path.join(options.dir, 'index.sqlite')
    if os.path.exists(temp_file):
        os.unlink(temp_file)
    conn = sqlite.Connection(temp_file)

    c = conn.cursor()
    c.execute('CREATE TABLE files (name TEXT, ext TEXT, size INTEGER, user TEXT, seentime INTEGER, cname TEXT, cuser TEXT)')
    c.execute('CREATE INDEX idx1 ON files (name)')
    c.execute('CREATE INDEX idx2 ON files (ext)')
    c.execute('CREATE INDEX idx3 ON files (size)')

    user_index = [s.strip().split('\t') for s in file(os.path.join(options.dir, 'index'))]
    for username, seentime, filename in user_index:
        seentime = int(seentime)
        try:
            username = username.decode('cp1251').encode('utf-8')
        except:
            username = username.decode('utf-8').encode('utf-8')
        username_low = username.decode('utf-8').lower().encode('utf-8')

        def callback(filename, ext, size):
            t = (filename.lower().encode('utf-8'), ext.lower().encode('utf-8'), size, username_low, seentime,
                 filename.encode('utf-8'), username)
            print t
            c.execute('INSERT INTO files VALUES (?,?,?,?,?,?,?)', t)

        try:
            path = os.path.join(options.dir, filename)
            log('Parsing %s\n' % path)
            parse(path, callback)
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()
            raise

    c = None
    conn.commit()
    conn.close()

    os.rename(temp_file, final_file)


def main():
    parser = optparse.OptionParser()
    parser.add_option('--loop-delay', dest='loop_delay', default='0')
    parser.add_option('--dir', dest='dir', default='/tmp/dcindexbot')
    (options, args) = parser.parse_args()

    while True:
        try:
            reindex(options)
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()
        if int(options.loop_delay) <= 0:
            break
        log('Parsing completed\n')
        time.sleep(int(options.loop_delay))


if __name__ == '__main__':
    main()
