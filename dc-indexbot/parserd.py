#!/usr/bin/env python
# -*- coding: utf8 -*-
#
# A daemon that periodically parses downloaded files.xml.bz2
# and stores results in an sqlite database.
#
import os, sys, time, traceback, bz2, optparse, sqlite3
from xml.sax import handler, make_parser

def log(msg):
    sys.stderr.write(time.ctime() + ' ' + msg)
    sys.stderr.flush()

class MyHandler(handler.ContentHandler):
    def __init__(self, username, seentime, cursor):
        self.username = username
        self.seentime = seentime
        self.cursor = cursor
        try:
            self.username = self.username.decode('cp1251')
        except:
            self.username = self.username.decode('utf-8')

    def startElement(self, name, attrs):
        if 'Name' not in attrs: return
        filename = attrs['Name']
        ext = ''
        if name == 'Directory':
            ext = u'dir'
            size = 0
        elif name == 'File':
            components = filename.split('.')
            if len(components) >= 2 and len(components[-1]) <= 8:
                ext = components[-1]
            size = int(attrs.get('Size', '0'))
        self.cursor.execute('INSERT INTO files VALUES (?,?,?,?,?,?)',
            (filename, filename.lower(), ext, size, self.username, self.seentime))

def parse(username, seentime, path, cursor):
    bzfile = bz2.BZ2File(path, 'r')
    parser = make_parser()
    parser.setContentHandler(MyHandler(username, seentime, cursor))
    parser.parse(bzfile)

def reindex(options):
    temp_file = os.path.join(options.dir, 'index.sqlite.temp')
    final_file = os.path.join(options.dir, 'index.sqlite')
    if os.path.exists(temp_file):
        os.unlink(temp_file)
    conn = sqlite3.Connection(temp_file)

    c = conn.cursor()
    c.execute('CREATE TABLE files (origname TEXT, name TEXT, ext TEXT, size INTEGER, user TEXT, seentime INTEGER)')
    c.execute('CREATE INDEX idx1 ON files (name)')
    c.execute('CREATE INDEX idx2 ON files (ext)')
    c.execute('CREATE INDEX idx3 ON files (size)')
    del c

    user_index = [s.strip().split('\t') for s in file(os.path.join(options.dir, 'index'))]
    for username, seentime, filename in user_index:
        path = os.path.join(options.dir, filename)
        log('Parsing %s\n' % path)
        try:
            parse(username, int(seentime), path, conn.cursor())
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()

    conn.commit()
    conn.close()

    os.rename(temp_file, final_file)


def main():
    parser = optparse.OptionParser()
    parser.add_option('--delay', dest='delay', default='3600')
    parser.add_option('--dir', dest='dir', default='/tmp/dc-indexbot')
    (options, args) = parser.parse_args()

    while True:
        try:
            reindex(options)
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()
        if int(options.delay) <= 0:
            break
        log('Parsing completed\n')
        time.sleep(int(options.delay))


if __name__ == '__main__':
    main()
