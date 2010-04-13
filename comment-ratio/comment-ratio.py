#!/usr/bin/env python
# Computes ratio of comment size to total code size in your code base.
# Only C/C++ sources are considered. Whitespaces are ignored.
import os, sys, subprocess, optparse

# C program to do all the parsing at which Python is way too slow
HELPER_PROGRAM = r"""
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

enum { LOOK_AHEAD = 4, BUF_SIZE = 1024 };
enum { CODE = 0, COMMENT = 1 };

FILE *fp;
int buffer[BUF_SIZE], *head, *tail, line_len, line_nonws;
long long stats[2], total_size, whitespace_size, num_whitespace_lines, line_num;

void consume(int n, int account)
{
    while (n > 0 || tail != buffer + BUF_SIZE) {
        while (tail < buffer + BUF_SIZE)
            *tail++ = getc(fp);

        for (; tail - head > LOOK_AHEAD && n > 0; head++, n--) {
            assert(*head != EOF);
            total_size++;
            if (*head == '\n') {
                if (line_nonws == 0)
                    num_whitespace_lines++;
                line_len = 0;
                line_nonws = 0;
                line_num++;
                whitespace_size++;
            } else {
                if (isspace(*head)) {
                    whitespace_size++;
                } else {
                    stats[account]++;
                    line_nonws++;
                }
                line_len++;
            }
        }

        if (tail - head == LOOK_AHEAD) {
            memmove(buffer, head, LOOK_AHEAD * sizeof(buffer[0]));
            head = buffer;
            tail = buffer + LOOK_AHEAD;
        }
    }
}

int match(const char *str)
{
    int n;
    for (n = 0; str[n]; n++)
        if (head[n] != str[n])
            return 0;
    return 1;
}

/* Parse C/C++ code for comments */
int parse(const char *filename)
{
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to open \"%s\"\n", filename);
        return 0;
    }
    head = tail = buffer;
    line_num = 1;
    memset(stats, 0, sizeof(stats));
    total_size = 0;
    whitespace_size = 0;
    num_whitespace_lines = 0;
    line_len = 0;
    line_nonws = 0;
    consume(0, CODE);

    while (*head != EOF) {
        if (match("//")) {
            consume(2, COMMENT);
            while (*head != EOF) {
                if (match("\\\r\n")) {
                    consume(3, COMMENT);
                } else if (match("\\\n")) {
                    consume(2, COMMENT);
                } else if (*head == '\n') {
                    consume(1, COMMENT);
                    break;
                } else {
                    consume(1, COMMENT);
                }
            }
        } else if (match("/*")) {
            consume(2, COMMENT);
            while (*head != EOF) {
                if (match("*/")) {
                    consume(2, COMMENT);
                    break;
                } else {
                    consume(1, COMMENT);
                }
            }
        } else if (*head == '"' || *head == '\'') {
            int quote = *head;
            consume(1, CODE);
            while (*head != EOF) {
                if (head[0] == '\\' && head[1] != EOF) {
                    consume(2, CODE);
                } else if (*head == quote) {
                    consume(1, CODE);
                    break;
                } else {
                    if (*head == '\n') {
                        fprintf(stderr, "Warning: newline in a quoted string at %s:%lld\n", filename, line_num);
                        break;
                    }
                    consume(1, CODE);
                }
            }
        } else {
            consume(1, CODE);
        }
    }

    line_num--;
    if (line_len != 0) {
        line_num++;
        if (line_nonws == 0)
            num_whitespace_lines++;
    }

    fclose(fp);
    return 1;
}

int main()
{
    static char path[10000];

    printf("Comment bytes\tNon-comment bytes\tWhitespace bytes\tTotal bytes\tLines of code\tWhitespace lines\tFile\n");
    fflush(stdout);

    while (fgets(path, sizeof(path), stdin)) {
        int n;
        for (n = strlen(path); n > 0 && (path[n-1] == '\n' || path[n-1] == '\r');)
            path[--n] = 0;

        if (parse(path)) {
            printf("%lld\t%lld\t%lld\t%lld\t%lld\t%lld\t%s\n",
                stats[COMMENT], stats[CODE], whitespace_size, total_size,
                line_num - num_whitespace_lines, num_whitespace_lines,
                path);
        } else {
            printf("\n");
        }
        fflush(stdout);
    }

    return 0;
}
"""

def good_filename(filename):
    if filename.startswith('.'):
        return False

    for ext in ['.c', '.cc', '.cpp', '.h', '.hpp']:
        if filename.endswith(ext):
            return True

    return False

def walk_codebase(base):
    for dir, subdirs, files in os.walk(base):
        subdirs.sort()
        for s in list(subdirs):
            if s.startswith('.'):
                subdirs.remove(s)

        for filename in sorted(files):
            if not good_filename(filename):
                continue

            filepath = os.path.join(dir, filename)
            if not os.path.exists(filepath):
                continue

            yield filepath


def main():
    parser = optparse.OptionParser(usage='%prog [options] <path to codebase>')
    parser.add_option('-v', '--verbose', action='store_true', dest='verbose', help='print detailed info')
    (options, args) = parser.parse_args()

    if len(args) != 1:
        parser.print_help()
        return

    compiler = subprocess.Popen('gcc -O2 -pipe -x c -o /tmp/c-comment-ratio -', stdin=subprocess.PIPE, shell=True)
    compiler.stdin.write(HELPER_PROGRAM)
    compiler.stdin.close()
    if os.waitpid(compiler.pid, 0)[1] != 0:
        print 'C compiler failed'
        return

    helper = subprocess.Popen('/tmp/c-comment-ratio', stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    headers = helper.stdout.readline().strip().split('\t')[:-1]
    totals = { 'Number of files': 0 }

    for filepath in walk_codebase(args[0]):
        helper.stdin.write(filepath + '\n')
        helper.stdin.flush()
        line = helper.stdout.readline().strip().split('\t')
        if len(line) == 0:
            continue

        for i, h in enumerate(headers):
            totals[h] = totals.get(h, 0) + int(line[i])
        totals['Number of files'] += 1

    helper.stdin.close()

    if totals['Number of files'] == 0:
        print 'No C/C++ files were found in that path'
        return

    ratio = totals['Comment bytes'] * 100.0 / (totals['Comment bytes'] + totals['Non-comment bytes'])

    if not options.verbose:
        print '%.3f' % ratio
    else:
        totals['Comment ratio'] = '%.3f%%' % ratio
        headers = ['Number of files'] + headers
        headers += [s for s in totals.keys() if s not in headers]
        for key in headers:
            print '%*s: %s' % (max(len(s) for s in headers), key, totals[key])

if __name__ == '__main__':
    main()
