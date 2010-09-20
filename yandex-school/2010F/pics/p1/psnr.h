#ifndef psnr_h
#define psnr_h

#include <cv.h>

using namespace cv;
using namespace std;

class TErrorCalculator {
public:
    static void Normalization(const Mat & src,  Mat & dst) {
        vector<Mat> planes;
        vector<Mat> normPlanes(3);
        split(src, planes);

        for (int idx = 0; idx < 3; idx++) {
            normPlanes[idx] = Mat(src.size(), DataType<float>::type);

            Scalar meanval, stddev;
            meanStdDev(planes[idx], meanval, stddev);

            MatIterator_<uchar> it1 = planes[idx].begin<uchar>(),
                it1_end = planes[idx].end<uchar>();

            MatIterator_<float> it2 = normPlanes[idx].begin<float>(),
                it2_end = normPlanes[idx].end<float>();


            for(; it1 != it1_end; ++it1, ++it2)
            {
                float div = 1.0;
                if (stddev[0] > 0)
                    div = (float)(1.0f/ stddev[0]);
                *it2 = (float)((*it1 - meanval[0]) / div);
            }
        }
        merge(normPlanes, dst);
    }

    static double PSNR(const Mat& lhs, const Mat& rhs)
    {
        assert(lhs.cols == rhs.cols && lhs.rows == rhs.rows);
        int rows = lhs.cols;
        int cols = rhs.rows;

        // Reshaping use for represent planes as one big plane
        // Mat_<int> use for correct calculating
        Mat_<int> difference;
        Mat_<int> lhsReshape = lhs.reshape(1);
        Mat_<int> rhsReshape = rhs.reshape(1);
        absdiff(lhsReshape, rhsReshape, difference);

        Mat_<int> squareDifference;
        multiply(difference, difference, squareDifference);

        double MSE = sum(squareDifference)[0] / double(cols * rows);

        const double MAX = 255.0;
        return 20.0 * std::log10(MAX / std::sqrt(std::fabs(MSE)));
    } 

    static double PSNRWithNormalization(const Mat & img1, const Mat & img2) {
       Mat imgNorm1, imgNorm2;
       Normalization(img1, imgNorm1);
       Normalization(img2, imgNorm2);

       return PSNR(img1, img2);
    }

    //static float CalcPSNR(const Mat & img1, const Mat & img2) {
    //    assert(img1.size() == img2.size());
    //    assert(img1.data != 0);
    //    assert(img2.data != 0);

    //    //Mat imgNorm1, imgNorm2;
    //    //Normalization(img1, imgNorm1);
    //    //Normalization(img2, imgNorm2);

    //    vector<Mat> planes1, planes2;
    //    split(img1, planes1);
    //    split(img2, planes2);

    //    float errorSum = 0;
    //    for (int idx = 0; idx < 3; idx++) {
    //        MatIterator_<uchar> it1 = planes1[idx].begin<uchar>(),
    //            it1_end = planes1[idx].end<uchar>();

    //        MatIterator_<uchar> it2 = planes2[idx].begin<uchar>();
    //        //                it2_end = planes2[idx].end<uchar>();

    //        for(; it1 != it1_end; ++it1, ++it2) {
    //            float d = (*it1 - *it2);
    //            errorSum += d*d;
    //        }
    //    }

    //    float rmse = sqrt(errorSum / (3 * img1.size().height *  img1.size().width));

    //    if (rmse == 0)
    //        rmse = 0.000000000000000000000000001f;

    //    return 20 * log10(255.0f / rmse);
    //}

};
#endif 