#include <climits>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <deque>
#include <vector>

#define CHECK(cond) while (!(cond)) { fprintf(stderr, "Assertion %s failed at %s:%d\n", #cond, __FILE__, __LINE__); abort(); }

struct Edge {
    int source;
    int destination;
    int weight;
};

class EdgeIterator {
    std::vector<Edge>::const_iterator head, tail;

  public:
    EdgeIterator(const std::vector<Edge> &edges) : head(edges.begin()), tail(edges.end()) {}
    bool Done() const { return head == tail; }
    void Next() { ++head; }
    const Edge *operator ->() { return &*head; }
};

class Graph {
    int num_vertices, num_edges;
    std::vector< std::vector<Edge> > adj_lists;

  public:
    EdgeIterator GetAdjacencyList(int source) const {
        CHECK(0 <= source && source < num_vertices);
        return EdgeIterator(adj_lists[source]);
    }

    int Size() const {
        return num_vertices;
    }

    static void Read(Graph *graph, int *source, int *destination) {
        CHECK(scanf("%d %d %d %d", &graph->num_vertices, &graph->num_edges, source, destination) == 4);
        graph->adj_lists.assign(graph->num_vertices, std::vector<Edge>());

        (*source)--;
        (*destination)--;

        for (int index = 0; index < graph->num_edges; index++) {
            Edge edge;
            CHECK(scanf("%d %d %d", &edge.source, &edge.destination, &edge.weight) == 3);
            CHECK(1 <= edge.source && edge.source <= graph->num_vertices);
            CHECK(1 <= edge.destination && edge.destination <= graph->num_vertices);
            CHECK(edge.weight == 0 || edge.weight == 1);

            edge.source--;
            edge.destination--;

            graph->adj_lists[edge.source].push_back(edge);

            std::swap(edge.source, edge.destination);
            graph->adj_lists[edge.source].push_back(edge);
        }
    }
};

int GetShortestPathLength(const Graph &graph, int source, int destination)
{
    std::vector<int> dist(graph.Size(), INT_MAX);
    std::deque<int> deck;

    deck.push_back(source);
    dist[source] = 0;

    while (!deck.empty()) {
        int vertex = deck.front();
        deck.pop_front();

        for (EdgeIterator edge = graph.GetAdjacencyList(vertex); !edge.Done(); edge.Next()) {
            if (dist[vertex] + edge->weight < dist[edge->destination]) {
                dist[edge->destination] = dist[vertex] + edge->weight;
                if (edge->weight == 0)
                    deck.push_front(edge->destination);
                else
                    deck.push_back(edge->destination);
            }
        }
    }

    return dist[destination] == INT_MAX ? -1 : dist[destination];
}

int main()
{
    Graph graph;
    int source, destination;
    Graph::Read(&graph, &source, &destination);
    printf("%d\n", GetShortestPathLength(graph, source, destination));
    return 0;
}
