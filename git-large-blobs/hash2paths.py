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
        if unhex(commit.hexsha) in visited_commits:
            continue
        visited_commits.add(unhex(commit.hexsha))

        if commit.hexsha in queries:
            print '%s COMMIT' % (queries[commit.hexsha])

        for p in commit.parents:
            if p.hexsha not in visited_commits:
                stack.append(p)

        if commit.tree in visited_trees:
            continue

        tree_stack = [ (commit.tree, '') ]
        while len(tree_stack) != 0:
            tree, path = tree_stack.pop()
            visited_trees.add(unhex(tree.hexsha))

            if tree.hexsha in queries:
                print '%s TREE %s:%s' % (queries[tree.hexsha], commit.hexsha, path)

            tree_id = tree.hexsha

            for item in tree.blobs:
                if item.hexsha in queries:
                    print '%s BLOB %s:%s' % (queries[item.hexsha], commit.hexsha, (path + item.name).encode('utf-8'))

            for item in tree.trees:
                if unhex(item.hexsha) not in visited_trees:
                    tree_stack.append((item, path + item.name + '/'))

if __name__ == '__main__':
    main()
