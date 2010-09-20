#include <cmath>
#include "simple.h"
#include "resize.h"

using namespace std;

// Clamps x into range [0, 255]
double Clamp(double x) {
    return max(0.0, min(255.0, x));
}

// Unsharpen filter of size (2r+1)x(2r+1) and a given sigma
template<int r>
void Unsharpen(const cv::Mat &src, cv::Mat &dst, double sigma) {
    const cv::Size src_size = src.size();
    const cv::Size dst_size = dst.size();

    // precompute gaussian kernel
    double h[2*r+1][2*r+1], normcoef = 0;
    for (int i = -r; i <= r; i++) {
        for (int j = -r; j <= r; j++) {
            h[r+i][r+j] = exp(-(i*i + j*j)/(2*sigma*sigma));
            normcoef += h[r+i][r+j];
        }
    }
    for (int i = -r; i <= r; i++)
        for (int j = -r; j <= r; j++)
            h[r+i][r+j] /= normcoef;

    for (int y = 0; y < dst_size.height; ++y) {
        for (int x = 0; x < dst_size.width; ++x) {
            double src_value = src.at<uchar>(y, x);
            double value = 0;

            if (y < r || x < r || y + r > dst_size.height || x + r > dst_size.width) {
                value = src_value;
            } else {
                // convolution with gaussian
                for (int i = -r; i <= r; i++)
                    for (int j = -r; j <= r; j++)
                        value += h[r+i][r+j] * src.at<uchar>(y + i, x + j);
            }

            dst.at<uchar>(y, x) = Clamp(src_value - (value - src_value));
        }
    }
}

class TSimpleResizer : public IImageResizer {
  public:
    TSimpleResizer() {};
    virtual ~TSimpleResizer() {};

    virtual void Resize(const cv::Mat &src, cv::Mat &dst, cv::Size newSize) const {
        vector<cv::Mat> srcPlanes;
        cv::split(src, srcPlanes);
        vector<cv::Mat> dstPlanes(srcPlanes.size());
        for (size_t iPlane = 0; iPlane < dstPlanes.size(); ++iPlane) {
            dstPlanes[iPlane] = cv::Mat(newSize, srcPlanes[iPlane].type());
            const cv::Mat& srcp = srcPlanes[iPlane];
            cv::Mat& dstp = dstPlanes[iPlane];
            ResizePlane(srcp, dstp);
        }
        cv::merge(dstPlanes, dst);
    }

  private:

    // Mitchell-Netravali filter
    // Per http://www.control.auc.dk/~awkr00/graphics/filtering/filtering.html
    // Kernel support: 2
    static double MitchellNetravali(double x) {
        const double b = 1 / 3.0;
        const double c = 1 / 3.0;
        const double eps = 1e-9;

        x = fabs(x);

        if (x <= 1 + eps) {
            return
                (12 - 9*b - 6*c) * x*x*x +
                (-18 + 12*b + 6*c) * x*x +
                (6 - 2*b);
        } else if (x <= 2 + eps) {
            return
                (-b - 6*c) * x*x*x +
                (6*b + 30*c) * x*x +
                (-12*b - 48*c) * x +
                (8*b + 24*c);
        } else {
            return 0;
        }
    }

    // Lanczos filter
    template<int n>
    static double LanczosKernel(double x) {
        static double pi = 2*acos(0.0);
        x = fabs(x);
        if (x < 1e-9) {
            return 1;
        } else if (x < n + 1e-9) {
          double xpi = x * pi;
          return (sin(xpi) / xpi) * sin(xpi / n) / (xpi / n);
        }
        return 0;
    }

    static double Kernel(double x) {
        return MitchellNetravali(x);
    }

    // Kernel()'s radius of support.
    enum { kKernelSupport = 2 };

    // Resize a single color plane
    static void ResizePlane(const cv::Mat &src, cv::Mat &dst) {
        const cv::Size src_size = src.size();
        const cv::Size dst_size = dst.size();
        const double x_ratio = 1.0 * src_size.width / dst_size.width;
        const double y_ratio = 1.0 * src_size.height / dst_size.height;

        for (int y = 0; y < dst_size.height; ++y) {
            for (int x = 0; x < dst_size.width;  ++x) {
                // real coordinates of dst point in source image
                double sx = x_ratio * x;
                double sy = y_ratio * y;

                // bouding box of support in source image for this point
                int sminx = (int)max(0, (int)floor(sx - kKernelSupport));
                int sminy = (int)max(0, (int)floor(sy - kKernelSupport));
                int smaxx = (int)min(src_size.width - 1, (int)ceil(sx + kKernelSupport));
                int smaxy = (int)min(src_size.height - 1, (int)ceil(sy + kKernelSupport));

                double value = 0;
                double total_weight = 0;

                // 2D convolution with the kernel
                for (int iy = sminy; iy <= smaxy; iy++) {
                    double ky = Kernel(iy - sy);
                    for (int ix = sminx; ix <= smaxx; ix++) {
                        double w = ky * Kernel(ix - sx);
                        value += w * src.at<uchar>(iy, ix);
                        total_weight += w;
                    }
                }
                value /= total_weight;

                dst.at<uchar>(y, x) = Clamp(value);
            }
        }

        // postprocessing with unsharpen filter 11x11, sigma=1.75
        cv::Mat tmp = dst;
        Unsharpen<5>(tmp, dst, 1.75);
    }
};

IImageResizer *CreateSimpleResizer() {
    return new TSimpleResizer();
}
