#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <cv.h>
#include <highgui.h>

#include <time.h>

#include "resize.h"
#include "simple.h"
#include "psnr.h"


using namespace std;
using namespace cv;


class TResizeFile {
public:
    TResizeFile(const string & fname, int w, int h) {
        string bigFname = string(fname, 0, fname.length() - 4);
        string ext = string(fname, fname.length() - 4);

        bigFname += "_big" + ext;

        Mat img = imread(fname);

        if (!img.data) {
            fprintf(stderr, "There is no src file");
            abort();
            //throw std::exception("There is no src file");
        }

        //You have to implement your CreateYOURNAMEResizer();
        IImageResizer * imgResizer = CreateSimpleResizer();
        Size newSize(w, h);

        Mat dst;
        imgResizer->Resize(img, dst, newSize);

        imwrite(bigFname, dst);

        //PrintError(fname, ext, dst);

    }

    ~TResizeFile () {
    }
private:
    void PrintError(const string &fname, const string &, const Mat & dst ) const {
        //print PSNR if possible
        string origFname = string(fname, 0, fname.length() - 4);
        origFname += "_orig.jpg";

        Mat origImage = imread(origFname);
        cout << origImage.size().width << "\t" << origImage.size().height << endl;
        if (!origFname.data()) {
            cerr << "warning: no original file" << endl;
        } else {
            float err = TErrorCalculator::PSNR(dst, origImage);
            cout << "psnr:" << err << endl;
        }
    }
};

void Usage() {
    cout << "resize filename new_width new_height" << endl;
}

//void testPSNR(const string & fname1, const string & fname2) {
//    DWORD t1 = GetTickCount();
//
//    float err = TErrorCalculator::NormalizationPSNR(imread(fname1), imread(fname2));
//    
//    DWORD t2 = GetTickCount();
//
//    float err2 = TErrorCalculator::PSNR(imread(fname1), imread(fname2));
//    DWORD t3 = GetTickCount();
//
//    cout << "psnr between:" << fname1 << " and " << fname2 << ":" <<  err << "\t" << err2 << endl;
//
//    cout <<" time:" << t2-t1 << "\t" <<  t3-t2 << endl;
//
//}

int main(int argc,char** argv) 
{ 
    //testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/2.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/1.jpg");
    //testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/2.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/3.jpg");
    //testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/1.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/3.jpg");

    //
   /* testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_big.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_orig.jpg");
    testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_orig.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_big.jpg");
    testPSNR("C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_big.jpg", "C:/toxa/articles/seminars/seminar2/assign_1_test/resize/sln/Debug/zabor_big.jpg");*/

    if (argc != 2) {
        Usage();
        return 1;
    }

    try {
       TResizeFile resizeFile(argv[1], 2048, 1536); //atoi(argv[2]), atoi(argv[3]));
    } catch (exception & e) {
        cerr << "Exception caught: " << e.what() << endl;
    }
        
    return 0;
}
