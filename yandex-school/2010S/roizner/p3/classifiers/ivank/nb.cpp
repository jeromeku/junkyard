#include "nb.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdio>

#include <set>
#include <stdexcept>

REGISTER_CLASSIFIER(mll::ivank::NaiveBayes, "NaiveBayes", "ivank", "Naive Bayes");

namespace mll {
namespace ivank {

static double sqr(double x) { return x * x; }

void NaiveBayes::Learn(IDataSet *data) {
    if (data->GetClassCount() != 2)
        throw std::logic_error("NaiveBayes implements only binary classification");

    class_freq_[0] = 0;
    class_freq_[1] = 0;
    for (int i = 0; i < data->GetObjectCount(); i++) {
        int c = data->GetTarget(i);
        if (c != 0 && c != 1)
            throw std::logic_error("Expected class label 0 or 1");
        class_freq_[c]++;
    }

    weights_.clear();
    if (class_freq_[0] == 0) {
        bias_ = 1;
        return;
    } else if (class_freq_[1] == 0) {
        bias_ = -1;
        return;
    } else {
        bias_ = log(class_freq_[1]) - log(class_freq_[0]);
    }

    weights_.resize(data->GetFeatureCount());
    for (int i = 0; i < data->GetFeatureCount(); i++) {
        FeatureInfo info = data->GetMetaData().GetFeatureInfo(i);
        if (!GetUseNormalForDiscrete() && (info.Type == Binary || info.Type == Nominal)) {
            EstimateDiscrete(data, i, info.Type == Binary ? 2 : info.NominalValues.size());
        } else {
            EstimateContinuous(data, i);
        }
    }

    std::string ignored_features = GetIgnoredFeatures();
    for (const char *s = ignored_features.c_str(); *s;) {
        if (*s == ',') { s++; continue; }

        const char *begin = s;
        while (*s && *s != ',') s++;

        std::string name(begin, s - begin);

        bool found = false;
        for (int i = 0; i < data->GetFeatureCount(); i++) {
            FeatureInfo info = data->GetMetaData().GetFeatureInfo(i);
            if (info.Name == name) {
                weights_[i].clear();
                found = true;
            }
        }

        if (!found)
            throw std::logic_error("feature \"" + name + "\" doesn't exist");
    }
}

void NaiveBayes::EstimateContinuous(IDataSet *data, int f) {
    // assumptions:
    //   x|y=0 ~ N(μ0, σ^2)
    //   x|y=1 ~ N(μ1, σ^2)

    // estimate means
    double mu[2] = { 0, 0 };
    for (int i = 0; i < data->GetObjectCount(); i++) {
        int c = data->GetTarget(i);
        mu[c] += data->GetFeature(i, f);
    }
    for (int i = 0; i < 2; i++)
        mu[i] /= class_freq_[i];

    // estimate common variance
    double var = 0;
    for (int i = 0; i < data->GetObjectCount(); i++) {
        int c = data->GetTarget(i);
        var += sqr(data->GetFeature(i, f) - mu[c]);
    }
    var /= data->GetObjectCount() - 1;  // unbiased estimate

    // log p(x|y=1)/p(x|y=0) = 1/(2σ^2) (2(μ1-μ0)x + (μ0^2 - μ1^2)) = wx + b
    double w = (mu[1] - mu[0]) / var;
    double b = (mu[0]*mu[0] - mu[1]*mu[1]) / (2 * var);

    // add the above log odds to the formula
    bias_ += b;
    weights_[f].assign(1, w);
}

void NaiveBayes::EstimateDiscrete(IDataSet *data, int f, int num_values) {
    std::vector<double> p(num_values, 0), q(num_values, 0);  // probabilities for class 0 and 1

    assert(num_values >= 2);

    // compute frequencies
    for (int i = 0; i < data->GetObjectCount(); i++) {
        double x = data->GetFeature(i, f);
        int c = int(x);
        assert(c == x && 0 <= c && c < num_values);
        if (data->GetTarget(i) == 0)
            p[c] += 1;
        else
            q[c] += 1;
    }

    // obtain probability estimates
    double denom0 = class_freq_[0] + num_values * GetSmoothing();
    double denom1 = class_freq_[1] + num_values * GetSmoothing();
    for (int i = 0; i < num_values; i++) {
        p[i] = (p[i] + GetSmoothing()) / denom0;
        q[i] = (q[i] + GetSmoothing()) / denom1;
    }

    // compute log odds
    weights_[f].resize(num_values);
    for (int i = 0; i < num_values; i++)
        weights_[f][i] = (log(q[i]) - log(p[i]));
}

void NaiveBayes::Classify(IDataSet *data) const {
    assert(weights_.empty() || data->GetFeatureCount() == weights_.size());

    for (int i = 0; i < data->GetObjectCount(); i++) {
        double log_odds = bias_;

        if (!weights_[i].empty()) {
            for (int j = 0; j < data->GetFeatureCount(); j++) {
                double x = data->GetFeature(i, j);
                if (weights_[j].size() == 1) {  // continuous
                    log_odds += weights_[j][0] * x;
                } else if (weights_[j].size() >= 2) {  // discrete
                    int c = int(x);
                    assert(c == x && 0 <= c && c < weights_[j].size());
                    log_odds += weights_[j][c];
                }
            }
        }

        data->SetTarget(i, log_odds > 0 ? 1 : 0);
    }
}

} // namespace your_name
} // namespace mll
