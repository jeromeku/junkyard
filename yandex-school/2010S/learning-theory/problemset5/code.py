#!/usr/bin/env python
# coding: utf-8
import numpy
import matplotlib.pyplot as pyplot

def conformal_algorithm(train_points, test_point):
    # построение многочлена f(x)=a+bx+cx^2 по обучающей выборке включая
    # проверяемую точку
    xs = [x for (x, y) in train_points] + [test_point[0]]
    ys = [y for (x, y) in train_points] + [test_point[1]]
    N = len(xs)
    c, b, a = numpy.polyfit(xs, ys, 2)

    # вычисление меры странности - отклонения y от f(x)=a+bx+cx^2
    alpha = []
    for i in range(N):
        x, y = xs[i], ys[i]
        alpha.append(abs(a + b*x + c*x*x - y))

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
    print '%.9g + %.9gx + %.9gx^2' % (a, b, c)  # точечное предсказание в x=0

    plot_y = []; plot_p = []
    for i in range(5000):
        y = i / 10000.0
        p = conformal_algorithm(sample, (0.0, y))
        if p >= 0.95:
            print y,
        plot_y.append(y); plot_p.append(p)

    fig = pyplot.figure()
    fig_plot = fig.add_subplot(111)
    pyplot.xlabel('y')
    pyplot.ylabel('p')
    fig_plot.plot(plot_y, plot_p)
    fig.savefig('figure2.pdf')

main()
