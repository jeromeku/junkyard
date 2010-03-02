#!/usr/bin/env python
# Gradient boosting in 100 lines or less.
import random, math, sys

class Node(object):
    def __init__(self, instances, nminobs):
        self.instances = instances                          # (x,y,w,a) list: x=features, y=target, w=weight, a=aux data
        W = float(sum(w for (x, y, w, a) in instances))
        self.var, self.left, self.right = None, None, None  # split variable (None if leaf), and child nodes
        self.value = sum(w*y for (x, y, w, a) in instances) / (1 if (W == 0) else W)  # split point, or output of a leaf
        self.best_split = (0.0, None, 0.0)                  # best split's (gain, split variable, split value)

        if W < 2 * nminobs: return

        # find the split which produces largest reduction in CART training error (weighted sum of squared errors)
        for var in xrange(len(instances[0][0])):
            values = sorted([(x[var], y, w) for (x, y, w, a) in instances if w > 0])
            total_sum = float(sum(w*y for (x, y, w) in values))
            lwei, rwei = 0.0, W
            lsum, rsum = 0.0, total_sum

            # consider all split points in the middle between two adjacent values of attribute var in the training set
            for i, (x, y, w) in enumerate(values):
                lwei, rwei = lwei + w, rwei - w        # shift one observation from right node to left
                lsum, rsum = lsum + w*y, rsum - w*y
                if lwei < nminobs or rwei < nminobs: continue

                next_x = values[i+1][0]
                if next_x == x: continue  # instances with equal attribute should all go to the same child node

                gain = -total_sum**2/W + lsum**2/lwei + rsum**2/rwei  # decrease in training error
                if gain > self.best_split[0]:
                    self.best_split = (gain, var, (x + next_x) / 2)

    def do_split(self, nminobs):  # called when we decide to proceed with the best split (which was found in __init__)
        self.var, self.value = self.best_split[1:]
        self.left = Node([x for x in self.instances if x[0][self.var] < self.value], nminobs)
        self.right = Node([x for x in self.instances if not (x[0][self.var] < self.value)], nminobs)
        self.instances = None
        return (self.left, self.right)

class RegressionTree(object):
    """CART regression tree, without pruning and missing-attributes stuff. Handles just real-valued attributes."""

    def __init__(self, instances, nleaves, nminobs):
        self.root = Node(instances, nminobs)
        self.leaves = [self.root]
        while len(self.leaves) < nleaves:
            node = max(self.leaves, key=lambda n: n.best_split[0])
            if node.best_split[0] <= 0: return  # no more splits are possible
            self.leaves.remove(node)
            self.leaves += node.do_split(nminobs)

    def __call__(self, x):  # evaluation
        node = self.root
        while node.var is not None:
            if x[node.var] < node.value:
                node = node.left
            else:
                node = node.right
        return node.value

class TreeBoost(object):
    """Stochastic gradient tree boosting method."""

    # Default parameters and loss function specification
    L2_LOSS = dict(deriv=lambda y,f: f-y, gamma=lambda y,f: sum(y[i]-f[i] for i in xrange(len(y)))/float(len(y)))
    L1_LOSS = dict(deriv=lambda y,f: cmp(f,y), gamma=lambda y,f: sorted([y[i]-f[i] for i in xrange(len(y))])[len(y)//2])
    DEFAULTS = dict(ntrees=20, nleaves=5, shrinkage=0.1, nminobs=10, loss=L2_LOSS)

    def __init__(self, instances, **p):
        p = dict(self.DEFAULTS.items() + p.items())           # set missing parameters to defaults
        loss_deriv, argmin_gamma = p['loss']['deriv'], p['loss']['gamma']  # I feel uncomfortable with 2D indices

        self.trees = []
        self.bias = argmin_gamma([y for (x, y) in instances], [0] * len(instances))  # best constant approximation
        F = [self.bias] * len(instances)                      # vector of current predictions at each instance

        while len(self.trees) < p['ntrees']:
            # build the tree on a random subsample - here we subsample by assigning 0 weights
            subsample = [(x, -loss_deriv(y, F[i]), random.randint(0, 1), i) for i, (x, y) in enumerate(instances)]
            tree = RegressionTree(subsample, nleaves=p['nleaves'], nminobs=p['nminobs'])

            # optimize loss in each tree's region (hence this is gradient *tree* boost)
            for leaf in tree.leaves:
                idx = [a for (x, y, w, a) in leaf.instances]  # indices of instances (including 0-weighted) in this leaf
                leaf.instances = None                         # free memory
                leaf.value = p['shrinkage'] * argmin_gamma([instances[i][1] for i in idx], [F[i] for i in idx])
                for i in idx:
                    F[i] += leaf.value                        # update predictions at instances

            self.trees.append(tree)

    def __call__(self, x):  # evaluation
        return self.bias + sum(tree(x) for tree in self.trees)

### This is line 97 -- End of main code. -- Test stuff follows. -- Documentation at the end. ###

def read_libsvm(fp):
    """Reads dataset in libsvm format."""
    res = []
    for line in fp:



def MSE(h, test_set):
    """Returns mean squared error of hypothesis h on a given test set."""
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
    CV(concrete, 4, lambda s: TreeBoost(s, ntrees=20, shrinkage=0.5, nleaves=10, loss=TreeBoost.LAD_LOSS))  # => 25.96 average MSE

if __name__ == '__main__':
    main()

# == Documentation ==
#
# Description of parameters and their equivalents in R's gbm package:
#   * ntrees: a positive integer (= gbm's n.trees)
#       Number of trees in the model.
#   * shrinkage: a positive real number (= gbm's shrinkage)
#       Learning rate.
#       Hastie et al. recommend setting it to a small value < 0.1.
#       Decrease of shrinkage should be compensated by an increase of ntrees.
#   * nleaves: a positive integer, at least 2. (= gbm's interaction.depth + 1)
#       Number of leaves (terminal nodes) in each tree,
#       For example, nleaves=2 produces decision stumps.
#       Hastie et al. recommend nleaves=4..8, and note that "it is unlikely that
#       [values of ntrees] > 10 will be required".
#   * nminobs: a positive integer (= gbm's n.minobsinnode)
#       Minimum number of training instances that should be contained in each
#       tree leaf (i.e. CART code will ignore splits leading to nodes with
#       fewer than nminobs observations)
#   * loss: specification of the loss function L(y, y'), where y' = f(x) is
#       the predicted value. This is a Python dict with two function objects:
#         deriv(y, f) = derivative of L(y, f) with respect to f  (y, f are reals)
#         gamma(y, F) = arg min_{\gamma} \sum_i L(y[i], F[i] + \gamma)  (y, F are vectors)
#       See Friedman's original paper for some worked-out examples.
#       TreeBoost.L2_LOSS is least squares loss L(y, y') = 1/2 (y - y')^2
#       TreeBoost.L1_LOSS is least absolute deviation loss L(y, y') = |y - y'|
#
# Notes about this particular implementation of gradient boosting:
#   * subsampling is done by flipping a fair coin once for each instance.
#     Note that this is different from Friedman's pseudocode.
#     It's more compact python-wise, and seems to produce a bit better models.
#   * CART code sorts training set once for each feature on each split.
#     This adds a rather wasteful O(log n) factor to the runtime as compared
#     to industrial implementations.
#   * CART code is written for real-values attributes only. It'll naturally
#     handle binary attributes though, but you'll likely need to manually
#     preprocess more general categorical attributes.
#
# References:
#   * Friedman J. (1999). Greedy Function Approximation: A Gradient Boosting machine.  (the original paper)
#   * Friedman J. (1999). Stochastic Gradient Boosting.  (describes the important subsampling trick for g.b.)
#   * Hastie T., Tibshirani R., Friedman J. (2009). The Elements of Statistical Learning.  (a good, modern textbook)
#   * Ridgeway, G. ( ...)  (overview of boosting models)
#   * http://cran.r-project.org/web/packages/gbm/index.html  (an open-source industrial implementation of g.b.)
#
# Author: Ivan Krasilnikov, February 2010.
# Copyright status: code in this file is released into the public domain by the author.
