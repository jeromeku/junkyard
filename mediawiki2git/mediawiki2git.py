#!/usr/bin/env python
# Converts a mediawiki dump into a git fast-import stream

import calendar
import collections
import sys
import optparse
import os
import subprocess
import time
import xml.sax
import xml.sax.handler

Revision = collections.namedtuple('Revision', 'page_title page_id revision_id timestamp contributor_name contributor_ip contributor_id comment text')

def mwtime2unix(timestamp):
    return int(calendar.timegm(time.strptime(timestamp, "%Y-%m-%dT%H:%M:%SZ")))

def git_config(id):
    return os.popen('git config \'%s\'' % id, 'r').read().rstrip('\n')

def get_default_committer():
    return '%s <%s>' % (git_config('user.name'), git_config('user.email'))

class Handler(xml.sax.handler.ContentHandler):
    def __init__(self, exporter):
        self.revision = None
        self.revision_dict = None
        self.content_buf = None
        self.page_title = None
        self.page_id = None
        self.exporter = exporter

    def startElement(self, name, attr):
        if name == u'revision':
            self.revision_dict = dict()
        elif self.revision_dict is not None:
            if name != u'contributor':
                self.content_buf = list()
                self.revision_dict[name] = self.content_buf
        elif name == u'title' or name == u'id' or name == u'base':
            self.content_buf = list()

    def characters(self, content):
        if self.content_buf is not None:
            self.content_buf.append(content.encode('utf-8'))

    def endElement(self, name):
        if name == u'revision':
            def get(name):
                return ''.join(self.revision_dict[name]) if name in self.revision_dict else None

            revision = Revision(
                page_title = self.page_title,
                page_id = self.page_id,
                revision_id = int(get(u'id')),
                timestamp = mwtime2unix(get(u'timestamp')),
                contributor_name = get(u'username'),
                contributor_ip = get(u'ip'),
                contributor_id = get(u'id'),
                comment = get(u'comment'),
                text = get(u'text')
            )

            self.revision_dict = None
            self.exporter.export_revision(revision)

        elif self.revision_dict is None:
            if name == u'title':
                self.page_title = ''.join(self.content_buf)
            elif name == u'id':
                self.page_id = int(''.join(self.content_buf))
            elif name == u'base':
                self.exporter.set_base_url(''.join(self.content_buf))

        self.content_buf = None

class Exporter(object):
    def __init__(self, options):
        self.options = options
        self.base_host = ''
        if not self.options.no_new_branch:
            sys.stdout.write('reset %s\n' % self.options.ref)

    def export_revision(self, rev):
        if rev.contributor_name is not None:
            author = '%s <%s@%s>' % (rev.contributor_name, rev.contributor_id, self.base_host)
        elif rev.contributor_ip is not None:
            author = '%s <@%s>' % (rev.contributor_ip, rev.contributor_ip)

        sys.stdout.write('commit %s\nauthor %s %d +0000\ncommitter %s %d +0000\n' %
                (self.options.ref, author, rev.timestamp, self.options.committer, rev.timestamp))
        if rev.comment is None or len(rev.comment) == 0:
            sys.stdout.write('data 0\n')
        else:
            sys.stdout.write('data %d\n%s\n' % (len(rev.comment), rev.comment))

        filename = self.options.filename_format % dict(title=rev.page_title)
        sys.stdout.write('M 644 inline %s\ndata %d\n%s\n\n' % (filename, len(rev.text), rev.text))

    def set_base_url(self, url):
        assert url.startswith('http://')
        self.base_host = url[7:url.index('/', 7)]

def main():
    optparser = optparse.OptionParser()
    optparser.add_option('-c', dest='committer', default=get_default_committer(), help='committer (default: "%default")')
    optparser.add_option('-r', dest='ref', default='refs/heads/master', help='branch (default: "%default")')
    optparser.add_option('-f', dest='filename_format', metavar='FORMAT', default='%(title)s', help='Filename format string (default: "%default")')
    optparser.add_option('-n', dest='no_new_branch', action='store_true', help='do not issue a "reset" command to create branch')
    (options, args) = optparser.parse_args()

    xmlparser = xml.sax.make_parser()
    xmlparser.setContentHandler(Handler(Exporter(options)))
    xmlparser.parse(sys.stdin)

if __name__ == '__main__':
    main()
