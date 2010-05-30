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
    def __init__(self, callback, nextDirIdFunc):
        handler.ContentHandler.__init__(self)
        self.callback = callback
        self.dirStack = [-1]
        self.nextDirIdFunc = nextDirIdFunc

    def startElement(self, name, attrs):
        if name == u'Directory':
            filename = attrs[u'Name']
            self.callback(filename, u'dir', 0, self.dirStack[-1])
            self.dirStack.append(self.nextDirIdFunc())
        elif name == u'File':
            filename = attrs[u'Name']
            components = filename.split(u'.')
            ext = u''
            if len(components) >= 2 and len(components[-1]) <= 8:
                ext = components[-1]
            size = int(attrs[u'Size'])
            self.callback(filename, ext, size, self.dirStack[-1])

    def endElement(name):
        if name == u'Directory':
            if len(self.dirStack) > 1:
                self.dirStack.pop()

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
    c.execute('CREATE TABLE files (name TEXT, name_low TEXT, ext TEXT, size INTEGER, userid INTEGER, dirid INTEGER)')
    c.execute('CREATE TABLE users (userid INTEGER PRIMARY KEY, name TEXT, name_low TEXT, seen INTEGER)')
    c.execute('CREATE TABLE dirs (dirid INTEGER PRIMARY KEY, parent INTEGER, name TEXT)')
    # dirid=-1 means no parent directory

    c.execute('CREATE INDEX idx1 ON files (name_low)')
    c.execute('CREATE INDEX idx2 ON files (ext)')
    c.execute('CREATE INDEX idx3 ON files (size)')
    c.execute('CREATE INDEX idx4 ON users (name_low)')

    next_dir_id = [0]
    def alloc_dir_id():
        next_dir_id[0] += 1
        return next_dir_id[0] - 1
    
    user_list = [s.strip().split('\t') for s in file(os.path.join(datadir, 'index'))]
    for userid, (username, seentime, filename) in enumerate(user_list):
        try:
            username.decode('utf-8')
        except:
            username = username.decode('cp1251').encode('utf-8')

        seentime = int(seentime)
        username_u = username.decode('utf-8')
        c.execute('INSERT INTO users VALUES(%s,%s,%s,%s)',
            (userid, username, username_u.lower().encode('utf-8'), seentime))

        def callback(filename_u, ext_u, size, dirid):
            c.execute('INSERT INTO files VALUES(%s,%s,%s,%s,%s,%s)', (
                filename_u.encode('utf-8'),
                filename_u.lower().encode('utf-8'),
                ext_u.lower().encode('utf-8'),
                size, userid, dirid))

        try:
            log('Parsing %s\n' % filename)
            bzfile = bz2.BZ2File(os.path.join(datadir, filename), 'r')
            parser = make_parser()
            parser.setContentHandler(DcXmlFilesHandler(callback, alloc_dir_id))
            parser.parse(bzfile)
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
