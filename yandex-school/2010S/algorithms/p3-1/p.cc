#include <cctype>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

//#define CHECK assert
void CHECK(bool c) { if (!c) abort(); }

class Automaton {
private:
    std::vector<int> edges;

public:
    int size, alphabet_size, start;
    std::vector<bool> is_final;

    int operator()(int vertex, int symbol) const {
        return edges[vertex * alphabet_size + symbol];
    }

    int &operator()(int vertex, int symbol) {
        return edges[vertex * alphabet_size + symbol];
    }

    void Read() {
        int num_terminals;
        CHECK(scanf("%d %d %d", &size, &num_terminals, &alphabet_size) == 3);

        start = 0;

        is_final.resize(size, false);
        for (int i = 0; i < num_terminals; i++) {
            int vertex;
            CHECK(scanf("%d", &vertex) == 1);
            is_final[vertex] = true;
        }

        int num_edges = size * alphabet_size;
        edges.resize(num_edges, -1);
        for (int i = 0; i < num_edges; i++) {
            int src, dst;
            char sym;
            CHECK(scanf("%d %c %d", &src, &sym, &dst) == 3);
            (*this)(src, sym - 'a') = dst;
        }
    }
};

class UnionFind {
private:
    std::vector<int> parent, rank;

public:
    int size;

    UnionFind(int size) : parent(size), rank(size, 0), size(size) {
        for (int i = 0; i < size; i++)
            parent[i] = i;
    }

    int Find(int x) {
        if (parent[parent[x]] == parent[x])
            return parent[x];
        else
            return parent[x] = Find(parent[x]);
    }

    void Union(int x, int y) {
        x = Find(x);
        y = Find(y);
        if (x != y) {
            if (rank[x] > rank[y])
                std::swap(x, y);
            parent[y] = x;
            rank[x]++;
        }
    }
};

bool CheckEquivalence(const Automaton &automaton1, const Automaton &automaton2)
{
    int size1 = automaton1.size, size2 = automaton2.size, alphabet_size = automaton1.alphabet_size;
    if (automaton1.alphabet_size != automaton2.alphabet_size)
        return false;

    UnionFind uf(size1 + size2);
    uf.Union(automaton1.start, size1 + automaton2.start);

    for (int iter = 0;; iter++) {
        std::vector<int> temp(uf.size * alphabet_size, -1);
        bool changed = false;

        for (int x = 0; x < size1 + size2; x++) {
            for (int c = 0; c < alphabet_size; c++) {
                int y = (x < size1 ? automaton1(x, c) : (size1 + automaton2(x - size1, c)));
                int xx = uf.Find(x);
                int yy = uf.Find(y);
                int &tmp = temp[xx * alphabet_size + c];
                if (tmp == -1) {
                    tmp = yy;
                } else if (uf.Find(tmp) != yy) {
                    uf.Union(tmp, yy);
                    changed = true;
                }
            }
        }

        if (!changed) {
            //fprintf(stderr, "Converged after %d iters\n", iter);
            break;
        }
    }

    std::vector<int> final(uf.size, 0);
    for (int x = 0; x < size1 + size2; x++) {
        bool f = (x < size1 ? automaton1.is_final[x] : automaton2.is_final[x - size1]);
        if (f)
            final[uf.Find(x)] = 1;
    }

    for (int x = 0; x < size1 + size2; x++) {
        bool f = (x < size1 ? automaton1.is_final[x] : automaton2.is_final[x - size1]);
        if (f != final[uf.Find(x)])
            return false;
    }
    return true;
}

int main()
{
    Automaton a, b;
    a.Read();
    b.Read();
    printf(CheckEquivalence(a, b) ? "EQUIVALENT\n" : "NOT EQUIVALENT\n");
    return 0;
}
