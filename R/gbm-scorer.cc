/*
C++ scorer for gradient boosted tree models built by R's gbm package.

First, you must export gbm model into a plain text file using the following script:

  export.gbm.model <- function(model, filename) {
    f = file(filename, open="w");
    cat("GBM MODEL\n", file=f);
    cat(sprintf("initF = %.15g\n", model$initF), file=f);
    cat(sprintf("vars = %s\n", paste(m$var.names, collapse=" ")), file=f);
    cat(sprintf("ntrees = %d\n", length(model$trees)), file=f);
    for (i in 1:length(model$trees)) {
      t = model$trees[[i]];
      cat(sprintf("tree %d\n", length(t[[1]])), file=f);
      for (j in 1:length(t[[1]])) {
        cat(sprintf("%d %.15g %d %d %d\n", t[[1]][j], t[[2]][j], t[[3]][j], t[[4]][j], t[[5]][j]), file=f);
      }
    }
  }

Usage: ./scorer model [ntrees] <infile >outfile
Input is a CSV file with header.
Output is a numerical score, one per each record in the input.

Current limitations:
  * only real-valued features are supported
  * feature names must not include spaces
  * missing values are not (yet) supported
*/
#include <cassert>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
using namespace std;

#define CHECK assert

bool ReadLine(FILE *fp, string *res)
{
    res->clear();

    for (;;) {
        int c = getc(fp);
        if (c == EOF)
            return res->size() != 0;
        if (c == '\n')
            return true;
        res->push_back((char)c);
    }
}

bool ReadCSVLine(FILE *fp, vector<string> *res)
{
    res->clear();
    res->push_back("");

    for (;;) {
        int c = getc(fp);
        if (c == EOF)
            return res->size() > 1 || res->back().size() != 0;
        if (c == '\n')
            return true;
        if (c == ',')
            res->push_back("");
        else
            res->back().push_back((char)c);
    }
}

void Split(const char *s, vector<string> *res)
{
    res->clear();

    for (;;) {
        while (*s && isspace(*s))
            s++;
        if (*s == 0)
            return;

        res->push_back("");
        while (*s && !isspace(*s))
            res->back().push_back(*s++);
    }
}

struct GBMModel {
    struct Node {
        int splitVar, leftNode, rightNode, missingNode;
        double splitCode;
    };

    int ntrees;
    double initF;
    vector<Node> nodes;
    vector<int> roots;
    vector<string> vars;

    double ScoreTree(const Node *base, double *x) const {
        const Node *node = base;
        while (true) {
            if (node->splitVar == -1)
                return node->splitCode;
            else if (x[node->splitVar] < node->splitCode)
                node = base + node->leftNode;
            else
                node = base + node->rightNode;
            // TODO: missing values support
        }
    }

    double Score(double *x) const {
        double res = initF;
        CHECK(ntrees <= (int)roots.size());
        for (int i = 0; i < ntrees; i++)
            res += ScoreTree(&nodes[roots[i]], x);
        return res;
    }

    GBMModel(const char *filename) {
        string line;
        Node n;

        FILE *fp = fopen(filename, "r");

        if (fp == NULL) {
            fprintf(stderr, "Can't open \"%s\"\n", filename);
            perror("fopen");
            abort();
        }

        CHECK(ReadLine(fp, &line));
        CHECK(line == "GBM MODEL");
        CHECK(ReadLine(fp, &line));
        CHECK(sscanf(line.c_str(), "initF = %lf", &initF) == 1);
        CHECK(ReadLine(fp, &line));
        CHECK(memcmp(line.c_str(), "vars = ", 7) == 0);
        Split(line.c_str() + 7, &vars);
        CHECK(ReadLine(fp, &line));
        CHECK(sscanf(line.c_str(), "ntrees = %d", &ntrees) == 1);
        CHECK(ntrees >= 0);

        roots.reserve(ntrees);

        for (int i = 0; i < ntrees; i++) {
            int tree_size;

            CHECK(ReadLine(fp, &line));
            CHECK(1 == sscanf(line.c_str(), "tree %d", &tree_size));
            CHECK(tree_size >= 1);

            roots.push_back(nodes.size());

            for (int j = 0; j < tree_size; j++) {
                CHECK(ReadLine(fp, &line));
                CHECK(5 == sscanf(line.c_str(), "%d %lf %d %d %d", &n.splitVar, &n.splitCode, &n.leftNode, &n.rightNode, &n.missingNode));
                CHECK(-1 <= n.splitVar && n.splitVar <= (int)vars.size());
                if (n.splitVar != -1) {
                    CHECK(n.leftNode > j && n.leftNode < tree_size);
                    CHECK(n.rightNode > j && n.rightNode < tree_size);
                }
                nodes.push_back(n);
            }
        }

        fclose(fp);
    }
};

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s modelfile [ntrees] <cvsfile\n", argv[0]);
        return 1;
    }

    GBMModel model(argv[1]);

    if (argc == 3)
        model.ntrees = atoi(argv[2]);

    vector<int> idx;
    {
        int nvars = model.vars.size();
        vector<string> header;

        ReadCSVLine(stdin, &header);

        idx.assign(nvars, -1);
        for (int i = 0; i < nvars; i++) {
            size_t j;
            for (j = 0; j < header.size(); j++)
                if (header[j] == model.vars[i])
                    break;
            if (j == header.size()) {
                fprintf(stderr, "Error: variable \"%s\" is not present in the input\n", model.vars[i].c_str());
                exit(1);
            }
            idx[i] = j;
        }
    }

    vector<double> vals(idx.size(), 0.0);
    vector<string> line;

    for (int no = 2; ReadCSVLine(stdin, &line); no++) {
        for (size_t i = 0; i < idx.size(); i++) {
            if (idx[i] >= (int)line.size()) {
                fprintf(stderr, "Error on line %d: too few fields\n", no);
                exit(1);
            }
            if (sscanf(line[idx[i]].c_str(), "%lf", &vals[i]) != 1) {
                fprintf(stderr, "Error on line %d: couldn't convert \"%s\" to a float\n", no, line[i].c_str());
                exit(1);
            }
        }

        printf("%.15g\n", model.Score(&vals[0]));
    }

    return 0;
}
