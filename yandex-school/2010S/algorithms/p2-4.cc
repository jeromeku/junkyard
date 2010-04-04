#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#define CHECK(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "Assertion %s failed at %s:%d\n", #condition, __FILE__, __LINE__); \
            abort(); \
        } \
    } while (0)

const int MAX_STRING_SIZE = 1000;
const int INF = std::numeric_limits<int>::max() / 2;

template<class Container>
class ContainerIterator {
    typename Container::const_iterator begin, end;

  public:
    ContainerIterator(const Container &container) : begin(container.begin()), end(container.end()) {}
    bool Done() const { return begin == end; }
    void Next() { ++begin; }
    const typename Container::value_type *operator ->() const { return &(*begin); }
};

struct Edge {
    int destination;
    char symbol;
};

class Automaton {
  private:
    std::vector<std::vector<Edge> > adjacency_lists;

  public:
    typedef ContainerIterator<std::vector<Edge> > EdgeIterator;

    int size;
    int start;
    std::vector<bool> final;

    void Read() {
        int num_edges, num_terminals, source;
        CHECK(scanf("%d %d %d", &size, &num_edges, &num_terminals) == 3);

        start = 0;

        final.resize(size, false);
        for (int i = 0; i < num_terminals; i++) {
            int vertex;
            CHECK(scanf("%d", &vertex) == 1);
            final[vertex] = true;
        }

        adjacency_lists.resize(size);
        for (int i = 0; i < num_edges; i++) {
            Edge edge;
            CHECK(scanf("%d %c %d", &source, &edge.symbol, &edge.destination) == 3);
            adjacency_lists[source].push_back(edge);
        }
    }

    EdgeIterator GetAdjacencyList(int vertex) const {
        return EdgeIterator(adjacency_lists[vertex]);
    }
};

class Frontier {
    const Automaton &automaton;
    const std::string &input_string;
    std::vector<int> values;
    std::vector<bool> is_visited;

public:
    Frontier(const Automaton &automaton, const std::string &input_string)
        : automaton(automaton),
          input_string(input_string),
          values(automaton.size, INF) {
    }

    int &operator[](int index) { return values[index]; }
    int operator[](int index) const { return values[index]; }

    void Consume(int symbol) {
        std::vector<int> new_values(values.size(), INF);
        for (int source = 0; source < automaton.size; source++) {
            for (Automaton::EdgeIterator edge = automaton.GetAdjacencyList(source); !edge.Done(); edge.Next()) {
                if (edge->symbol != symbol)
                    continue;
                new_values[edge->destination] = std::min(new_values[edge->destination], values[source]);
            }
        }
        values = new_values;
    }

    void PropagateAlongEpsilonTransitions() {
        // sort of sorting
        int input_size = (int)input_string.size();
        std::vector<int> head(input_size + 1, -1), next(automaton.size, -1);
        for (int i = 0; i < automaton.size; i++) {
            if (values[i] != INF) {
                CHECK(values[i] <= input_size);
                next[i] = head[values[i]];
                head[values[i]] = i;
            }
        }

        is_visited.assign(automaton.size, false);
        for (size_t i = 0; i < head.size(); i++)
            for (int j = head[i]; j != -1; j = next[j])
                DFS(j);
    }

  private:
    void DFS(int state) {
        if (is_visited[state])
            return;
        is_visited[state] = true;

        for (Automaton::EdgeIterator edge = automaton.GetAdjacencyList(state); !edge.Done(); edge.Next()) {
            if (edge->symbol == '$') {
                if (values[state] < values[edge->destination]) {
                    values[edge->destination] = values[state];
                    DFS(edge->destination);
                }
            }
        }
    }
};

std::string FindLargestSubstring(const Automaton &automaton, const std::string &input) {
    int input_size = (int)input.size();
    int best_len = 0, best_start = 0;

    Frontier frontier(automaton, input);  // holds a slice of f(x, j) values for a fixed j
    frontier[automaton.start] = 0;
    frontier.PropagateAlongEpsilonTransitions();

    for (int j = 0; j < input_size; j++) {  // refer to my solution's description about what j is.
#if 0
        printf("f(x, %d): ", j);
        for (int x = 0; x < automaton.size; x++)
            printf(frontier[x] == INF ? " *" : " %d", frontier[x]);
        printf("  %c\n", input[j]);
#endif

        // compute f(x, j+1)
        frontier.Consume(input[j]);
        frontier[automaton.start] = std::min(j + 1, frontier[automaton.start]);
        frontier.PropagateAlongEpsilonTransitions();

        for (int state = 0; state < automaton.size; state++) {
            if (automaton.final[state] && frontier[state] != INF) {
                int len = j + 1 - frontier[state];
                CHECK(0 <= len && len <= input_size);
                if (len > best_len) {
                    best_len = len;
                    best_start = frontier[state];
                }
            }
        }
    }

    return input.substr(best_start, best_len);
}

int main()
{
    Automaton automaton;
    automaton.Read();

    char input[MAX_STRING_SIZE + 1];
    CHECK(scanf("%s", input) == 1);

    std::string output = FindLargestSubstring(automaton, input);
    printf("%s\n", output.size() == 0 ? "No solution" : output.c_str());

    return 0;
}
