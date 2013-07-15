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
        assert len(sha) == 40
        queries[sha] = line.rstrip('\r\n')

    repo = git.Repo()

    visited_commits = set()
    visited_trees = set()
    visited_paths = set()
    commit_stack = [ h.commit for h in repo.heads ]

    def output(commit, hexsha, path):
        key = unhex(hexsha), path
        if key in visited_paths:
            return
        visited_paths.add(key)

        if first[0]:
            print '--- commit %s ---' % commit.hexsha
            first[0] = 0

        print '%s\t%s' % (queries[hexsha], path)

    while len(commit_stack) != 0:
        commit = commit_stack.pop()
        if unhex(commit.hexsha) in visited_commits:
            continue
        visited_commits.add(unhex(commit.hexsha))

        if commit.hexsha in queries:
            print '%s\tCOMMIT' % queries[commit.hexsha]

        for p in commit.parents:
            if p.hexsha not in visited_commits:
                commit_stack.append(p)

        first = [1]
        tree_stack = [ (commit.tree, '') ]

        while len(tree_stack) != 0:
            tree, path = tree_stack.pop()
            key = unhex(tree.hexsha), path
            if key in visited_trees:
                continue
            visited_trees.add(key)

            if tree.hexsha in queries:
                output(commit, tree.hexsha, path + '/')

            for item in tree.blobs:
                if item.hexsha in queries:
                    output(commit, item.hexsha, path + item.name.encode('utf-8'))

            for item in tree.trees:
                tree_stack.append((item, path + item.name.encode('utf-8') + '/'))

        if not first[0]:
            sys.stdout.flush()

        if len(visited_commits) % 10 == 0:
            sys.stderr.write('\rProcessed %d commits' % len(visited_commits))

    sys.stderr.write('\rProcessed %d commits\n' % len(visited_commits))

if __name__ == '__main__':
    main()
