#!/usr/bin/env python
# coding: utf-8
import numpy, math
import matplotlib.pyplot as pyplot

# мера странности - расстояние к линии регрессии
def nonconformity_measure(train_points, test_point):
    xs = [x for (x, y) in train_points]
    ys = [y for (x, y) in train_points]

    c, b, a = numpy.polyfit(xs, ys, 2)

    x, y = test_point
    return abs(a + b*x + c*x*x - y)

def conformal_algorithm(train_points, test_point):
    points = train_points + [test_point]
    N = len(points)

    alpha = []
    for i in range(N):
        alpha.append(nonconformity_measure(points[:i] + points[(i+1):], points[i]))

    result = 0.0
    for i in range(N):
        if alpha[i] >= alpha[N-1]:
            result += 1.0
    return result / N

def main():
    sample_x = [-1.0, -0.75, -0.5, -0.25, 0.25, 0.75, 1.0]
    sample_y = [1.0, 0.75, 0.5, 0.25, 0.25, 0.75, 1.0]
    sample = zip(sample_x, sample_y)

    c, b, a = numpy.polyfit(sample_x, sample_y, 2)
    print a  # точечное предсказание в x=0

    plot_y = []; plot_p = []
    for i in range(1000):
        y = i / 2000.0
        p = conformal_algorithm(sample, (0.0, y))
        if p >= 0.95:
            print y, p
        plot_y.append(y); plot_p.append(p)

    fig = pyplot.figure()
    fig_plot = fig.add_subplot(111)
    pyplot.xlabel('y')
    pyplot.ylabel('p(y)')
    fig_plot.plot(plot_y, plot_p)
    fig_plot.plot(plot_y, plot_p)
    fig.savefig('figure.pdf')

main()
