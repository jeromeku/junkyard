#ifndef resize_h_234244
#define resize_h_234244

#include <cv.h>

//base class 
class IImageResizer {
public:
    IImageResizer() {}
    virtual ~IImageResizer() {}

    virtual void Resize(const cv::Mat & src, cv::Mat & dst, cv::Size newSize) const = 0;
};


#endif 