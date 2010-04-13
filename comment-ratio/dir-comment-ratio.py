#!/usr/bin/env python
# Computes ratio of comment size to total code size in your code base.
# Only C/C++ sources are considered. Whitespaces are ignored.
import os, sys, subprocess

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
    if len(sys.argv) != 2:
        print 'Usage: %s <path to codebase>' % sys.args[0]
        return

    subproc = subprocess.Popen('./c-comment-ratio', stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    num_files = 0
    total_size = 0

    for filepath in walk_codebase(sys.argv[1]):
        data = file(filepath, 'rb').read()
        subproc.stdin.write(data)
        num_files += 1
        total_size += len(data)

    subproc.stdin.close()

    comment_size, code_size = map(int, subproc.stdout.read().split())

    print 'Files: %d.  Comment size: %d.  Code size: %d.  Total size with whitespace: %d.' % (num_files, comment_size, code_size, total_size)


if __name__ == '__main__':
    main()
