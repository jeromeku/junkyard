#!/usr/bin/env python
# Takes a list of sha1 hashes, one per line, possibly with extra info after the hash,
# traverses trees in git history and reports locations of each mentioned hash.

import sys, git

def unhex(s):
    return int(s, 16)

def main():
    queries = dict()
    for line in sys.stdin:
        sha = line.split()[0]
        queries[sha] = line.rstrip('\r\n')

    repo = git.Repo()

    visited_commits = set()
    visited_trees = set()
    stack = [ h.commit for h in repo.heads ]

    while len(stack) != 0:
        commit = stack.pop()
        if unhex(commit.id) in visited_commits:
            continue
        visited_commits.add(unhex(commit.id))

        if commit.id in queries:
            print '%s COMMIT' % (queries[commit.id])

        for p in commit.parents:
            if p.id not in visited_commits:
                stack.append(p)

        if commit.tree in visited_trees:
            continue

        tree_stack = [ (commit.tree, '') ]
        while len(tree_stack) != 0:
            tree, path = tree_stack.pop()
            visited_trees.add(unhex(tree.id))

            if tree.id in queries:
                print '%s TREE %s:%s' % (queries[tree.id], commit.id, path)

            tree_id = tree.id
            try:
                items = tree.items()
            except:
                sys.stderr.write('Failure while reading tree %d\n' % tree_id)
                raise

            for filename, item in items:
                if type(item) is git.Tree:
                    if unhex(item.id) not in visited_trees:
                        tree_stack.append((item, path + filename + '/'))
                elif item.id in queries:
                    print '%s BLOB %s:%s' % (queries[item.id], commit.id, path + filename)

if __name__ == '__main__':
    main()
