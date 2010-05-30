#!/usr/bin/env python
# coding: utf-8
# A cgi search front-end to indexbot's index.
import os, sys, cgi, cgitb, re, time
import sqlite3
sqlite = sqlite3

HUB_NAME = 'server.lan'
INDEX_FILE = '/tmp/dcindexbot/index.sqlite'

def rewrite_query(user_query, page_no):
    """Parses and rewrites user query as an SQL query on index database."""

    terms = set()
    exts = set()
    users = set()
    min_size = None
    max_size = None

    def bad(s):
        for c in ['"', "'", '?', '\\', ';']:
            if c in s:
                return True
        return False

    for word in user_query.lower().split():
        if word.startswith('ext:') or word.startswith('type:') or word.startswith('filetype:') or word.startswith('fileext:'):
            word = word[(word.index(':') + 1):]
            for e in word.split(','):
                if len(e) == 0 or bad(e):
                    continue
                exts.add(e)
        elif word.startswith('user:'):
            word = word[5:]
            if len(word) != 0 and not bad(word):
                users.add(word)
        elif word.startswith('size:'):
            m = re.match('size:([0-9]+)[.][.]([0-9]+)', word)
            if m is None:
                raise 'Ошибка в операторе size. Используйте синтаксис: size:min..max.'
            min_size = int(m.group(1))
            max_size = int(m.group(2))
        else:
            if len(word) != 0 and not bad(word):
                terms.add(word)

    def term_clauses():
        c = []
        for term in terms:
            c.append('files.name_low LIKE "%%%s%%"' % term)
        return c

    def ext_clauses():
        c = []
        for ext in exts:
            c.append('files.ext = "%s"' % ext)
        if len(c) == 0:
            return []
        else:
            return ['(%s)' % ' OR '.join(c)]
    
    def user_clauses():
        c = []
        for u in users:
            c.append('users.name_low LIKE "%%%s%%"' % u)
        if len(c) == 0:
            return []
        else:
            return ['(%s)' % ' OR '.join(c)]
    
    def size_clauses():
        if min_size is None:
            return []
        else:
            return ['files.size >= %d' % min_size, 'files.size <= %d' % max_size]

    sql = 'SELECT files.name, files.size, users.name, users.seen FROM files JOIN users ON files.userid = users.userid'
    sql += ' WHERE %s' % ' AND '.join(term_clauses() + ext_clauses() + size_clauses() + user_clauses())
    sql += ' LIMIT %d,101' % (page_no * 100)
    return sql

def do_search(fp, cgi, user_query, page_no):
    try:
        sql = rewrite_query(user_query, page_no)
    except:
        if cgi:
            fp.write('<div style="color:red; font-weight:bold">В заданном запросе ошибка:<br />%s</div>' %
                cgi.escape(str(sys.exc_info()[0]), True))
            return
        else:
            raise

    if not cgi:
        fp.write('SQL query: %s\n' % sql)

    conn = sqlite.Connection(INDEX_FILE)
    cursor = conn.cursor()
    cursor.execute(sql)

    row = cursor.fetchone()
    if row is None:
        if cgi:
            fp.write('Ничего не найдено\n')
        else:
            fp.write('No results found\n')
        return

    if cgi:
        fp.write('<table><tr><td><b>Имя файла</b></td><td><b>Размер</b></td><td><b>Пользователь</b></td><td><b>Последний раз в сети был:</b></td></tr>\n')

    while row is not None:
        filename, filesize, username, userseen = row

        filesize = int(filesize)
        if filesize < 0:
            filesize = ''
        elif filesize > 100*1024:
            filesize = '%.2f Mb' % (filesize / 1048576.0)
        else:
            filesize = str(filesize)

        if cgi:
            fp.write('%s<br/>' % str(row))
        else:
            fp.write('"%s", user: %s, size: %s, last seen: %s\n' %
                (filename.encode('utf-8'), username.encode('utf-8'), filesize, time.ctime(userseen)))
        row = cursor.fetchone()

    if cgi:
        fp.write('</table>\n')


def cgi_header(fp, user_query):
    escaped_query = cgi.escape(user_query, True)
    if len(user_query) != 0:
        title = '%s - DcIndexBot Search on %s' % (escaped_query, HUB_NAME)
    else:
        title = 'IndexBot'

    template = '''<html>
<head>
<title>%(title)s</title>
</head>
<body>
<form method="GET">
Поисковый запрос:
<input type="text" name="q" id="q" size="50" value="%(escaped_query)s"/>
<input type="submit" value="Поиск" />
</form>
<script>document.getElementById("q").focus()</script>

    '''
    
    fp.write(template % locals())
    fp.flush()

def cgi_footer(fp):
    fp.write('''</body>\n</html>\n''')

def cgi_main():
    fp = sys.stdout
    fp.write('Content-Type: text/html\r\n\r\n')
    fp.flush()

    form = cgi.FieldStorage()
    query = form.getvalue('q', '').strip()
    try:
        page = max(0, int(form.getvalue('p', '0')))
        if page > 100:
            page = 0
    except:
        page = 0

    cgi_header(fp, query)
    if len(query) != 0:
        do_search(fp, True, query, page)
    cgi_footer(fp)


if __name__ == '__main__':
    if 'GATEWAY_INTERFACE' in os.environ:
        cgitb.enable()
        cgi_main()
    else:
        query = ' '.join(sys.argv[1:])
        do_search(sys.stdout, False, query, 0)
