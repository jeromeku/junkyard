#!/usr/bin/env python
# Gradient boosting in 100 lines or less.
import random, math, sys

class Node(object):
    def __init__(self, instances, nminobs):  # nminobs - minimum allowed sum of weights of instances in each node
        self.instances = instances                               # x=features, y=target, w=weight, a=aux data
        W = float(sum(w for (x, y, w, a) in instances))
        self.var, self.left, self.right = None, None, None       # split variable (None if leaf) and child nodes
        self.value = sum(w*y for (x, y, w, a) in instances) / W  # split point, or output value if leaf
        self.best_split = (0.0, None, 0.0)                       # best split's (gain, split variable, split value)

        if W < 2 * nminobs:
            return

        # Find best split which gives largest decrease in the training error: \sum w[i]*(y[i] - h(x[i]))^2,
        # where h(x[i]) is prediction at x[i]: h(x[i]) = (\sum w[j] y[j])/(\sum w[j]) over all instances j
        # which are in the same node as i-th instance.
        # Note that we do not need to keep track of sum of squares because they're canceled in the end.
        for var in xrange(len(instances[0][0])):
            values = sorted([(x[var], y, w) for (x, y, w, a) in instances if w > 0])
            total_sum = float(sum(w*y for (x, y, w) in values))
            lwei, rwei = 0.0, W
            lsum, rsum = 0.0, total_sum

            for i, (x, y, w) in enumerate(values):
                lwei, rwei = lwei + w, rwei - w        # shift one observation from right node to left
                lsum, rsum = lsum + w*y, rsum - w*y
                if lwei < nminobs or rwei < nminobs:
                    continue

                next_x = values[i+1][0]
                if next_x == x:
                    continue    # instances with equal attribute should all go to the same child node

                gain = -total_sum**2/W + lsum**2/lwei + rsum**2/rwei  # decrease in training error
                if gain > self.best_split[0]:
                    self.best_split = (gain, var, (x + next_x) / 2)

    def do_split(self, nminobs):
        self.var, self.value = self.best_split[1:]
        self.left = Node([x for x in self.instances if x[0][self.var] < self.value], nminobs)
        self.right = Node([x for x in self.instances if not x[0][self.var] < self.value], nminobs)
        self.instances = None
        return (self.left, self.right)

class RegressionTree(object):
    """CART regression tree, without pruning, handles real-valued non-missing attributes only."""

    def __init__(self, instances, nleaves, nminobs=10):  # nleaves and nminobs are gbm's interaction.depth+1 and n.minobsinnode
        self.root = Node(instances, nminobs)
        self.leaves = [self.root]
        while len(self.leaves) < nleaves:
            node = max(self.leaves, key=lambda n: n.best_split[0])
            if node.best_split[0] <= 0:
                return  # no more splits are possible
            self.leaves.remove(node)
            self.leaves += node.do_split(nminobs)

    def __call__(self, x):
        node = self.root
        while node.var is not None:
            if x[node.var] < node.value:
                node = node.left
            else:
                node = node.right
        return node.value

class TreeBoost(object):
    """Stochastic gradient tree boosting.  This implementation is limited to regression with real-valued input."""

    def __init__(self, instances, ntrees, nleaves, shrinkage=0.01, nminobs=10):
        # loss function specific components of gradient boosting.  These two are for L(y, f(x)) = 1/2 (y - f(x))^2
        deriv = lambda y, f: f - y  # derivative of L(y, f) with respect to f (prediction)
        gamma = lambda y, f: sum(y[i]-f[i] for i in xrange(len(y)))/float(len(y))  # arg min_{\gamma} \sum_i L(y[i], f[i] + gamma)

        self.trees = []
        self.bias = gamma([y for (x, y) in instances], [0] * len(instances))
        F = [self.bias] * len(instances)     # vector of current predictions at each instance

        for each in range(ntrees):
            # we subsample by assigning 0 weights; this way we'll later get a list of all instances assigned to each leaf.
            subsample = [(x, -deriv(y, F[i]), random.randint(0, 1), i) for i, (x, y) in enumerate(instances)]
            tree = RegressionTree(subsample, nleaves=nleaves, nminobs=nminobs)

            # optimize loss function separately in each region of the tree
            for leaf in tree.leaves:
                idx = [a for (x, y, w, a) in leaf.instances]  # indices of instances which fell into this leaf
                leaf.instances = None
                leaf.value = shrinkage * gamma([instances[i][1] for i in idx], [F[i] for i in idx])
                for i in idx:
                    F[i] += leaf.value

            self.trees.append(tree)

    def __call__(self, x):
        return self.bias + sum(tree(x) for tree in self.trees)

### This is line 99 - End of main code.  Auxiliary stuff follows. ###

def MSE(h, test_set):
    """Returns mean squared error of hypothesis h on given test set."""
    return sum((y - h(x))**2 for (x, y) in test_set) / float(len(test_set))

def CV(instances, k, model_builder):
    """k-fold cross validation"""
    avg = 0
    for fold in range(k):
        train_set = [x for (i, x) in enumerate(instances) if i % k != fold]
        test_set = [x for (i, x) in enumerate(instances) if i % k == fold]
        mse = MSE(model_builder(train_set), test_set)
        print 'Fold %d: %.9g' % (fold + 1, mse)
        avg += mse
    print 'Average MSE: %.9g' % (avg / k)

def main():
    def parse(s):
        vec = [float(s.strip()) for s in s.split(',')]
        assert len(vec) == 9
        return (vec[:8], vec[8])

    # sample regression dataset from UCI, converted to .csv
    concrete = [parse(s) for s in file('concrete.csv') if s[0] != '#']

    random.seed(int(sys.argv[1]) if (len(sys.argv) > 1) else 53387)
    CV(concrete, 4, lambda s: TreeBoost(s, ntrees=20, shrinkage=0.5, nleaves=10))  # => 25.96 average MSE

if __name__ == '__main__':
    main()

# References:
#   * Hastie T., Tibshirani R., Friedman J. (2009). The Elements of Statistical Learning.
#   * Friedman J. (1999). Stochastic Gradient Boosting.
#
# Author: Ivan Krasilnikov, February 2010.
# Copyright status: code in this file is released into the public domain by the author.
