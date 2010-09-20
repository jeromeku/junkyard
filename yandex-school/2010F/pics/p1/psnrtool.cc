#include "psnr.h"
#include <assert.h>
#include <cv.h>
#include <highgui.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    assert(argc == 3);

    Mat m1 = imread(argv[1]);
    Mat m2 = imread(argv[2]);

    double psnr = TErrorCalculator::PSNR(m1, m2);
    printf("%.15g\n", psnr);

    return 0;
}
