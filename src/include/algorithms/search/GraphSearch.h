#ifndef ALGORITHMS_SEARCH_GRAPH_SEARCH_H
#define ALGORITHMS_SEARCH_GRAPH_SEARCH_H

#include "../Graph.h"
#include <cstddef>
#include <vector>

namespace algorithms::search {

    enum class GraphSearchStrategy {
        Dijkstra,
        AStar
    };

    struct GraphSearchResult {
        bool found = false;
        double total_cost = 0.0;
        std::size_t expanded_nodes = 0;
        std::vector<std::size_t> node_path;
    };

    [[nodiscard]] GraphSearchResult shortest_path(const graph::Graph& graph,
                                                  std::size_t start_node,
                                                  std::size_t goal_node,
                                                  GraphSearchStrategy strategy = GraphSearchStrategy::Dijkstra,
                                                  const std::vector<double>& heuristic = {});

} // namespace algorithms::search

#endif // ALGORITHMS_SEARCH_GRAPH_SEARCH_H
