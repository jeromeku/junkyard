#!/usr/bin/env python
# -*- coding: utf8 -*-
# Parses all downloaded files.xml.bz2, stores filelists
# in an sqlite database.
import os, sys, time, traceback, bz2, optparse
import sqlite  # TODO: use sqlite3 when it's available on the system
from xml.sax import handler, make_parser

def log(msg):
    sys.stderr.write(time.ctime() + ' ' + msg)
    sys.stderr.flush()

class DcXmlFilesHandler(handler.ContentHandler):
    def __init__(self, callback):
        handler.ContentHandler.__init__(self)
        self.callback = callback

    def startElement(self, name, attrs):
        if name == u'Directory':
            filename = attrs[u'Name']
            self.callback(filename, u'dir', 0)
        elif name == u'File':
            filename = attrs[u'Name']
            components = filename.split(u'.')
            ext = u''
            if len(components) >= 2 and len(components[-1]) <= 8:
                ext = components[-1]
            size = int(attrs[u'Size'])
            self.callback(filename, ext, size)

def parse(path, callback):
    bzfile = bz2.BZ2File(path, 'r')
    parser = make_parser()
    parser.setContentHandler(DcXmlFilesHandler(callback))
    parser.parse(bzfile)

def main():
    parser = optparse.OptionParser(usage='Usage: %prog <data directory>')
    (options, args) = parser.parse_args()

    if len(args) != 1:
        parser.print_usage()
        sys.exit(1)

    datadir = args[0]

    temp_file = os.path.join(datadir, 'index.sqlite.temp')
    final_file = os.path.join(datadir, 'index.sqlite')
    if os.path.exists(temp_file):
        log('Removing %s\n' % temp_file)
        os.unlink(temp_file)
    conn = sqlite.Connection(temp_file)

    c = conn.cursor()
    c.execute('CREATE TABLE files (name TEXT, name_low TEXT, ext TEXT, size INTEGER, userid INTEGER)')
    c.execute('CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, name_low TEXT, seen INTEGER)')
    c.execute('CREATE INDEX idx1 ON files (name_low)')
    c.execute('CREATE INDEX idx2 ON files (ext)')
    c.execute('CREATE INDEX idx3 ON files (size)')
    c.execute('CREATE INDEX idx4 ON files (userid)')
    c.execute('CREATE INDEX idx5 ON users (name_low)')

    user_index = [s.strip().split('\t') for s in file(os.path.join(datadir, 'index'))]

    for userid, (username, seentime, filename) in enumerate(user_index):
        try:
            username.decode('utf-8')
        except:
            username = username.decode('cp1251').encode('utf-8')

        seentime = int(seentime)
        username_u = username.decode('utf-8')
        c.execute('INSERT INTO users VALUES(%s,%s,%s,%s)',
            (userid, username, username_u.lower().encode('utf-8'), seentime))

        def callback(filename_u, ext_u, size):
            c.execute('INSERT INTO files VALUES(%s,%s,%s,%s,%s)', (
                filename_u.encode('utf-8'),
                filename_u.lower().encode('utf-8'),
                ext_u.lower().encode('utf-8'),
                size,
                userid))

        try:
            log('Parsing %s\n' % filename)
            parse(os.path.join(datadir, filename), callback)
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()

        conn.commit()

    c = None
    conn.close()

    os.rename(temp_file, final_file)
    log('Parsing completed\n')

if __name__ == '__main__':
    main()
