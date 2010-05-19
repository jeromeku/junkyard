#include "factories.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include "svm.h"

using namespace std;

#define ALGSYNONIM "SvmRbf"
#define PASSWORD "vapnik"

namespace mll {
namespace ivank {

// Auxiliary functions and classes {{{

#define ARRAY_SIZE(a) int(sizeof(a) / sizeof(a[0]))

template<class T> double GetSum(const T &c) {
    double res = 0;
    for (typename T::const_iterator it = c.begin(); it != c.end(); ++it)
        res += *it;
    return res;
}

template<class T> double GetMean(const T &c) {
    return GetSum(c) / (double)(c.size());
}

template<class T> double GetMode(const T &c) {
    map<double, int> freq;
    for (typename T::const_iterator it = c.begin(); it != c.end(); ++it)
        ++freq[*it];

    double res = 0;
    int res_freq = -1;
    for (map<double, int>::const_iterator it = freq.end(); it != freq.end(); ++it) {
        if (it->second > res_freq) {
            res = it->first;
            res_freq = it->second;
        }
    }

    return res;
}

// C++ wrapper for libsvm's struct svm_problem
struct svm_problem_xx : public svm_problem {
    vector<svm_node> nodes_;
    vector<svm_node *> pnodes_;
    vector<double> targets_;

    svm_problem_xx(const vector<vector<double> > &features, const vector<int> &targets) {
        nodes_.resize(features.size() * (features[0].size() + 1));
        for (int i = 0, k = 0; i < features.size(); i++) {
            pnodes_.push_back(&nodes_[k]);
            targets_.push_back(targets[i]);
            for (int j = 0; j < features[i].size(); j++) {
                nodes_[k].index = j;
                nodes_[k].value = features[i][j];
                k++;
            }
            nodes_[k++].index = -1;
        }

        l = features.size();
        x = &pnodes_[0];
        y = &targets_[0];
    }

  private:
    svm_problem_xx(const svm_problem_xx &) {}
    void operator =(const svm_problem_xx &) {}
};

// C++ wrapper for libsvm's struct svm_model
struct svm_model_xx {
    svm_model *model;
    svm_model_xx(svm_model *model) : model(model) {}
    ~svm_model_xx() { svm_destroy_model(model); }

  private:
    svm_model_xx(const svm_model_xx &) {}
    void operator =(const svm_model_xx &) {}
};

// }}}

class SvmRbf: public Classifier<SvmRbf> {
    DECLARE_REGISTRATION();

    sh_ptr<svm_problem_xx> train_prob;
    sh_ptr<svm_model_xx> model;

  public:
    // Transforms each nominal feature into one of more real features,
    // imputes missing values, scales data.
    static vector< vector<double> > PreprocessFeatures(const IDataSet *data) {
        vector<int> offsets(1, 0);
        for (int j = 0; j < data->GetFeatureCount(); j++) {
            FeatureInfo info = data->GetMetaData().GetFeatureInfo(j);
            assert(info.Type == Numeric || info.Type == Nominal);
            int expansion = (info.Type == Nominal && info.NominalValues.size() >= 3) ? info.NominalValues.size() : 1;
            offsets.push_back(offsets.back() + expansion);
        }

        vector< vector<double> > nonmissing_values(data->GetFeatureCount());
        for (int i = 0; i < data->GetObjectCount(); i++) {
            for (int j = 0; j < data->GetFeatureCount(); j++) {
                if (data->HasFeature(i, j)) {
                    double feature = data->GetFeature(i, j);
                    nonmissing_values[j].push_back(feature);
                }
            }
        }

        vector<double> imputed_value(data->GetFeatureCount(), 0.0);
        for (int j = 0; j < data->GetFeatureCount(); j++) {
            FeatureInfo info = data->GetMetaData().GetFeatureInfo(j);

            if (nonmissing_values[j].size() == 0) {
                imputed_value[j] = 0;
                continue;
            }

            sort(nonmissing_values[j].begin(), nonmissing_values[j].end());

            if (info.Type == Nominal && info.NominalValues.size() >= 3) {
                imputed_value[j] = GetMode(nonmissing_values[j]);
            } else if (info.Type == Nominal) {
                imputed_value[j] = 0;
            } else {
                imputed_value[j] = GetMean(nonmissing_values[j]);
            }
        }

        vector< vector<double> > result(data->GetObjectCount(), vector<double>(offsets.back(), 0.0));
        for (int i = 0; i < data->GetObjectCount(); ++i) {
            for (int j = 0; j < data->GetFeatureCount(); ++j) {
                FeatureInfo info = data->GetMetaData().GetFeatureInfo(j);
                double feature = data->HasFeature(i, j) ? data->GetFeature(i, j) : imputed_value[j];

                if (info.Type == Nominal && info.NominalValues.size() >= 3) {
                    int val = (int)feature;
                    assert(0 <= val && val < info.NominalValues.size());
                    result[i][offsets[j] + val] = 1;
                } else if (info.Type == Nominal) {
                    // binary
                    if (!data->HasFeature(i, j))
                        result[i][offsets[j]] = 0;
                    else if (fabs(feature) < 0.5)
                        result[i][offsets[j]] = -1.0;
                    else
                        result[i][offsets[j]] = 1.0;
                } else {
                    // real
                    if (nonmissing_values[j].size() != 0) {
                        // scale to [-1, 1]
                        double fmin = nonmissing_values[j][0];
                        double fmax = nonmissing_values[j].back();
                        if (fmax > fmin)
                            feature = (feature - fmin) / (fmax - fmin);
                        else
                            feature = 0;
                    }
                    result[i][offsets[j]] = feature;
                }
            }
        }

        return result;
    }

    virtual void Learn(IDataSet* data) {
        vector< vector<double> > features = PreprocessFeatures(data);

        vector<int> targets;
        for (int i = 0; i < data->GetObjectCount(); i++)
            targets.push_back(data->GetTarget(i));

        train_prob = sh_ptr<svm_problem_xx>(new svm_problem_xx(features, targets));

        double c_values[] = { 2048, 32768 };
        double gamma_values[] = { 3.0517578125e-05, 0.0001220703125 };

        svm_parameter best_param;
        double best_accuracy = -1;

        // select hyperparameters (regularization coefficient C and kernel parameter gamma) by 10-fold crossvalidation
        for (int c_index = 0; c_index < ARRAY_SIZE(c_values); c_index++) {
            for (int gamma_index = 0; gamma_index < ARRAY_SIZE(gamma_values); gamma_index++) {
                svm_parameter param;
                memset(&param, 0, sizeof(param));
                param.svm_type = C_SVC;
                param.kernel_type = RBF;
                param.C = c_values[c_index];
                param.gamma = gamma_values[gamma_index];
                param.cache_size = 100;
                param.eps = 1e-3;
                param.shrinking = 1;

                vector<double> res(targets.size());
                svm_cross_validation(train_prob.get(), &param, 10, &res[0]);

                double accuracy = 0;
                for (int i = 0; i < res.size(); i++)
                    if (res[i] == targets[i])
                        accuracy += 1;
                accuracy /= res.size();

                if (accuracy > best_accuracy) {
                    best_accuracy = accuracy;
                    best_param = param;
                }
            }
        }

        model = sh_ptr<svm_model_xx>(new svm_model_xx(svm_train(train_prob.get(), &best_param)));
    }

    virtual void Classify(IDataSet* data) const {
        vector< vector<double> > features = PreprocessFeatures(data);
        svm_problem_xx test_prob(features, vector<int>(data->GetObjectCount(), 0));

        for (int i = 0; i < data->GetObjectCount(); i++)
            data->SetTarget(i, svm_predict(model->model, test_prob.x[i]));
    }
};

} // namespace ivank
} // namespace mll

REGISTER_CLASSIFIER(mll::ivank::SvmRbf, "SvmRbf", "ivank",
        "SVM with RBF kernel. Uses libsvm as solver.\n"
        "Selects hyperparameters from the set C ∈ {2^11, 2^15}, gamma ∈ {2^-13, 2^-15} by 10-fold CV.\n"
        "Feature preprocessing:\n"
        "  * Imputes missing real features by mean, missing nominals by mode.\n"
        "  * Encodes binary features by values -1, 1 and 0 (for missing features). Linearly scales real features into range [-1, 1].\n");

// vim: fdm=marker ts=8 sts=4 sw=4 et
