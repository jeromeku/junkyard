#!/usr/bin/env python
# coding: utf8
from numpy import *
import matplotlib.pyplot as pyplot
import sys, math, random

def delta(val):
    if abs(val) < 1e-9:
        return 1
    else:
        return 0

# функция ковариации из задания
def R_eps(x, y, eps):
    N = len(x)
    dist2 = sum((x[i] - y[i])**2 for i in range(N))
    return (1 - eps) * math.exp(-0.3 * dist2) + eps * delta(dist2)

# считает матрицу ковариации для набора точек
def CovarianceMatrix(points, cov_func):
    N = len(points)
    res = matrix(zeros((N, N)))
    for i in range(N):
        for j in range(N):
            res[i, j] = cov_func(points[i], points[j])
    return res

# генерация многомерной нормальной с нулевым средним
def MultivariateNormal(cov_matrix):
    N = cov_matrix.shape[0]
    L = linalg.cholesky(cov_matrix)  # cov_matrix = L * L^T
    X = matrix(zeros((N, 1)))        # вектор со стандартными нормальными
    for i in range(N):
        X[i, 0] = random.normalvariate(0, 1)
    return L * X

# генерация случайной реализации поля в заданном наборе точек
def GenerateField(points, cov_func):
    R = CovarianceMatrix(points, cov_func)
    return MultivariateNormal(R)

# Предсказание значения поля в новой точке методом кригинга.
# R, u - матрица ковариации и значения поля для точек обучающей выборки
# r - вектор ковариаций точек обучающей выборки с новой точкой.
def Kriging(R, u, r):
    Rinv = linalg.inv(R)
    a = r.transpose() * Rinv       # a = r^T R^{-1}
    return a * u

X =  [[-1,-1,0], [-1,0,0], [-1,1,0], [0,-1,0], [0,0,0], [0,1,0], [1,-1,0], [1,0,0], [1,1,0],
      [-1,-1,1], [-1,0,1], [-1,1,1], [0,-1,1], [0,0,1], [0,1,1], [1,-1,1], [1,0,1], [1,1,1]]
assert len(X) == 18

fig = pyplot.figure()
fig_plot = fig.add_subplot(111)
pyplot.xlabel('eps')
pyplot.ylabel('F(eps)')

random.seed(2308)

for inst in range(10):
    U = GenerateField(X, lambda x, y: R_eps(x, y, 0.1))

    plot_x = []
    plot_y = []

    for i in range(100):
        eps = i / 100.0;
        R = CovarianceMatrix(X, lambda x, y: R_eps(x, y, eps))

        U2 = zeros((9, 1))
        Ueps = zeros((9, 1))
        for i in range(9, 18):
            U2[i - 9] = U[i]
            Ueps[i - 9] = Kriging(R[0:9, 0:9], U[0:9], R[0:9, i])

        F = sum((U2[i] - Ueps[i])**2 for i in range(9))

        plot_x.append(eps)
        plot_y.append(F)

    fig_plot.plot(plot_x, plot_y)

fig.savefig('figure2.pdf')
