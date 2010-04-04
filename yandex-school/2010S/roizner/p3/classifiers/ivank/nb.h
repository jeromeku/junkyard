#include "../../core/factories.h"

#include <vector>

namespace mll {
namespace ivank {

#define PARAMS \
    X(bool, UseNormalForDiscrete, false, "Use normal distribution to model discrete features") \
    X(double, Smoothing, 0.1, "Smoothing constant for estimates of probability of discrete features' values") \
    X(std::string, IgnoredFeatures, "", "Comma-separated names of features to be ignored")

class NaiveBayes: public Classifier<NaiveBayes> {
    int class_freq_[2];
    double bias_;
    std::vector< std::vector<double> > weights_;

    void EstimateContinuous(IDataSet *data, int f);
    void EstimateDiscrete(IDataSet *data, int f, int num_values);

public:
    //! Learn data
    virtual void Learn(IDataSet* data);
    //! Classify data
    virtual void Classify(IDataSet* data) const;

    NaiveBayes() :
#define X(type, name, def, desc) name##_(def),
        PARAMS
#undef X
        bias_(0)
    {
#define X(type, name, def, desc) AddParameter(#name, name##_, &NaiveBayes::Get##name, &NaiveBayes::Set##name, desc);
        PARAMS
#undef X
    }

#define X(type, name, def, desc) type Get##name() const { return name##_; } void Set##name(type arg) { name##_ = arg; }
        PARAMS
#undef X

private:
#define X(type, name, def, desc) type name##_;
    PARAMS
#undef X
};

#undef PARAMS

}
}
