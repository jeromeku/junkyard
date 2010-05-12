#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <vector>

#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "Assertion %s failed at %s:%d\n", #cond, __FILE__, __LINE__); exit(1); } } while(0)

class Automaton {  // {{{
private:
    std::vector<int> edges;
    int size, alphabet_size, start;
    std::vector<bool> is_final;

public:
    int Size() const { return size; }
    int AlphabetSize() const { return alphabet_size; }
    
    // Returns start state
    int GetStartVertex() const { return start; }

    // Returns true if the vertex represents a final state
    bool IsFinal(int vertex) const { return is_final[vertex]; }

    // Transition function
    int operator()(int vertex, int symbol) const {
        return edges[vertex * alphabet_size + symbol];
    }

    // Reads graph from stdin
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
            edges[src * alphabet_size + (sym - 'a')] = dst;
        }
    }
};  // }}}

class JointAutomaton {  // represents two automata as the same graph with two start states {{{
    const Automaton &first;
    const Automaton &second;

public:
    JointAutomaton(const Automaton &first, const Automaton &second) : first(first), second(second) {
        CHECK(first.AlphabetSize() == second.AlphabetSize());
    }

    int Size() const {
        return first.Size() + second.Size();
    }

    int AlphabetSize() const {
        return first.AlphabetSize();
    }

    // Returns start state if first (automaton_number=0) or second automaton (automaton_number=1)
    int GetStartVertex(int automaton_number) const {
        CHECK(automaton_number == 0 || automaton_number == 1);
        return automaton_number == 0 ? first.GetStartVertex() : (first.Size() + second.GetStartVertex());
    }

    // Returns true if the vertex represents a final state
    bool IsFinal(int vertex) const {
        if (vertex < first.Size())
            return first.IsFinal(vertex);
        else
            return second.IsFinal(vertex - first.Size());
    }

    // Transition function
    int operator()(int vertex, int symbol) const {
        if (vertex < first.Size())
            return first(vertex, symbol);
        else
            return first.Size() + second(vertex - first.Size(), symbol);
    }
};  // }}}

// Union-find data structure with path compression and union by rank heuristics.
class UnionFind {  // {{{
private:
    std::vector<int> parent, rank;

public:
    int size;

    UnionFind(int size) : parent(size), rank(size, 0), size(size) {
        for (int i = 0; i < size; i++)
            parent[i] = i;
    }

    // Find representative of the class that x belongs to
    int Find(int x) {
        if (parent[parent[x]] == parent[x])
            return parent[x];
        else
            return parent[x] = Find(parent[x]);
    }

    // Unite two classes
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
};  // }}}

bool CheckEquivalence(const Automaton &automaton1, const Automaton &automaton2)
{
    if (automaton1.AlphabetSize() != automaton2.AlphabetSize())
        return false;

    JointAutomaton joint(automaton1, automaton2);

    UnionFind unionFind(joint.Size());
    unionFind.Union(joint.GetStartVertex(0), joint.GetStartVertex(1));

    bool changed = false;
    do {
        // find states to be merged
        std::vector< std::vector<int> > mergeSequences(joint.Size() * joint.AlphabetSize());
        for (int vertex = 0; vertex < joint.Size(); vertex++) {
            int src_class = unionFind.Find(vertex);
            for (int symbol = 0; symbol < joint.AlphabetSize(); symbol++) {
                int dst_class = unionFind.Find(joint(vertex, symbol));
                mergeSequences[src_class * joint.AlphabetSize() + symbol].push_back(dst_class);
            }
        }

        // merge states
        changed = false;
        for (size_t list_index = 0; list_index < mergeSequences.size(); list_index++) {
            const std::vector<int> &list = mergeSequences[list_index];
            for (size_t index = 1; index < list.size(); index++) {
                int src = unionFind.Find(list[0]);
                int dst = unionFind.Find(list[index]);
                if (src != dst) {
                    changed = true;
                    unionFind.Union(src, dst);
                }
            }
        }
    } while (changed);

    // check for contradictions
    for (int vertex = 0; vertex < joint.Size(); vertex++)
        if (joint.IsFinal(vertex) != joint.IsFinal(unionFind.Find(vertex)))
            return false;

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
