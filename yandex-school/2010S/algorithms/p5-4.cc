#include <cstdio>
#include <vector>

struct Edge {
    int src, dst, weight;
    Edge(int src, int dst, int weight) : src(src), dst(dst), weight(weight) {}
    Edge() : src(0), dst(0), weight(0) {}
};

class EdgeIterator {
    std::vector<Edge>::const_iterator head, tail;

  public:
    EdgeIterator(const std::vector<Edge> &vec) : head(vec.begin()), tail(vec.end()) {}
    const Edge *operator ->() const { return &*head; }
    void Next() { ++head; }
    bool Done() { return head == tail; }
};

class Graph {
    int size;
    std::vector< std::vector<Edge> > adj_lists;

  public:
    void ReadFrom(FILE *fp) {
        int num_edges;
        fscanf(fp, "%d %d", &size, &num_edges);

        adj_lists.assign(size, std::vector<Edge>());

        while (num_edges-- > 0) {
            Edge edge;
            fscanf(fp, "%d %d %d", &edge.src, &edge.dst, &edge.weight);
            edge.src--;
            edge.dst--;
            adj_lists[edge.src].push_back(edge);
        }
    }

    int Size() const {
        return size;
    }

    EdgeIterator GetAdjList(int vertex) const {
        return EdgeIterator(adj_lists[vertex]);
    }

    // Returns graph with reversed edges.
    Graph GetTranspose() const {
        Graph res;
        res.size = size;
        res.adj_lists.assign(size, std::vector<Edge>());
        for (int src = 0; src < size; src++)
            for (EdgeIterator edge = GetAdjList(src); !edge.Done(); edge.Next())
                res.adj_lists[edge->dst].push_back(Edge(edge->dst, edge->src, edge->weight));
        return res;
    }
};

// Finds strongly connected components, labels them with integers 0, 1, 2...,
// returns an array with SCC label for each vertex.
std::vector<int> GetStrongComponents(const Graph &graph)
{
    // Implementation of Kosaraju's SCC algorithm
    struct KosarajuImpl {
        const Graph &graph;
        Graph transposed_graph;

        std::vector<bool> visited;
        std::vector<int> dfs1_sequence;
        std::vector<int> labels;

        KosarajuImpl(const Graph &graph) :
            graph(graph), transposed_graph(graph.GetTranspose()) {}

        // First DFS. Writes a sequence of vertices as they are finished into dfs1_sequence.
        void DFS1(int src) {
            std::vector<int> stack(1, src);

            while (!stack.empty()) {
                src = stack.back();
                stack.pop_back();

                if (src < 0) {
                    dfs1_sequence.push_back(-src - 1);
                    continue;
                }
                
                if (visited[src])
                    continue;

                visited[src] = true;
                stack.push_back(-src - 1);

                for (EdgeIterator edge = graph.GetAdjList(src); !edge.Done(); edge.Next())
                    if (!visited[edge->dst])
                        stack.push_back(edge->dst);
            }
        }

        // Second DFS on transposed graph. Labels all vertices in src's component with given label.
        void DFS2(int src, int label) {
            std::vector<int> stack(1, src);
            while (!stack.empty()) {
                src = stack.back();
                stack.pop_back();

                if (visited[src])
                    continue;

                visited[src] = true;
                labels[src] = label;

                for (EdgeIterator edge = transposed_graph.GetAdjList(src); !edge.Done(); edge.Next())
                    if (!visited[edge->dst])
                        stack.push_back(edge->dst);
            }
        }

        void Run() {
            visited.assign(graph.Size(), false);
            for (int vertex = 0; vertex < graph.Size(); vertex++)
                if (!visited[vertex])
                    DFS1(vertex);

            int num_scc = 0;
            labels.assign(graph.Size(), -1);
            visited.assign(graph.Size(), false);

            for (int index = dfs1_sequence.size() - 1; index >= 0; index--) {
                int vertex = dfs1_sequence[index];
                if (!visited[vertex])
                    DFS2(vertex, num_scc++);
            }
        }
    };

    KosarajuImpl impl(graph);
    impl.Run();
    return impl.labels;
}

// Runs a single Bellman-Ford iteration ignoring edges that cross strongly connected components.
void BellmanFordStep(const Graph &graph, const std::vector<int> &scc_labels, std::vector<int> &dist)
{
    std::vector<int> new_dist = dist;
    for (int src = 0; src < graph.Size(); src++) {
        for (EdgeIterator edge = graph.GetAdjList(src); !edge.Done(); edge.Next()) {
            if (scc_labels[edge->src] != scc_labels[edge->dst])
                continue;
            if (dist[edge->src] + edge->weight < new_dist[edge->dst])
                new_dist[edge->dst] = dist[edge->src] + edge->weight;
        }
    }
    dist.swap(new_dist);
}

int GetFirstVertexOnNegCycle(const Graph &graph)
{
    std::vector<int> scc_labels = GetStrongComponents(graph);

    // Run Bellman-Ford in strongly connected components to detect negative cycles
    std::vector<int> dist_n(graph.Size(), 0);
    for (int i = 0; i < graph.Size(); i++)
        BellmanFordStep(graph, scc_labels, dist_n);

    std::vector<int> dist_n_plus_1 = dist_n;
    BellmanFordStep(graph, scc_labels, dist_n_plus_1);

    // Detect strongly connected components that have a negative cycle
    std::vector<bool> good_scc(graph.Size(), false);
    for (int vertex = 0; vertex < graph.Size(); vertex++)
        if (dist_n[vertex] != dist_n_plus_1[vertex])
            good_scc[scc_labels[vertex]] = true;

    // Return smallest vertex that belongs to an SCC with a negative cycle
    for (int vertex = 0; vertex < graph.Size(); vertex++)
        if (good_scc[scc_labels[vertex]])
            return vertex;
    return -1;
}

int main()
{
    Graph G;
    G.ReadFrom(stdin);
    int ans = GetFirstVertexOnNegCycle(G);
    printf(ans == -1 ? "NO\n" : "YES\n%d\n", ans + 1);
    return 0;
}
