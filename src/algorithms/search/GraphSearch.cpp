#include "../../include/algorithms/GraphSearch.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <queue>
#include <vector>

namespace algorithms::search {

    namespace {

        struct OpenNode {
            std::size_t node_id = 0;
            double f_score = 0.0;
        };

        struct OpenNodeGreater {
            bool operator()(const OpenNode& lhs, const OpenNode& rhs) const {
                return lhs.f_score > rhs.f_score;
            }
        };

    }

    GraphSearchResult shortest_path(const graph::Graph& graph,
                                    const std::size_t start_node,
                                    const std::size_t goal_node,
                                    const GraphSearchStrategy strategy,
                                    const std::vector<double>& heuristic) {
        GraphSearchResult result;

        const std::size_t node_count = graph.adj.size();
        if (node_count == 0 || start_node >= node_count || goal_node >= node_count) {
            return result;
        }

        if (start_node == goal_node) {
            result.found = true;
            result.total_cost = 0.0;
            result.node_path = {start_node};
            return result;
        }

        const bool use_heuristic = (strategy == GraphSearchStrategy::AStar) &&
                                   (heuristic.size() == node_count);

        std::vector<double> g_score(node_count, std::numeric_limits<double>::infinity());
        std::vector<std::size_t> parent(node_count, node_count);
        std::vector<unsigned char> closed(node_count, 0);

        g_score[start_node] = 0.0;
        parent[start_node] = start_node;

        std::priority_queue<OpenNode, std::vector<OpenNode>, OpenNodeGreater> open_set;
        const double start_h = use_heuristic ? std::max(0.0, heuristic[start_node]) : 0.0;
        open_set.push({start_node, start_h});

        while (!open_set.empty()) {
            const OpenNode current = open_set.top();
            open_set.pop();

            const std::size_t current_id = current.node_id;
            if (closed[current_id] != 0) {
                continue;
            }
            closed[current_id] = 1;
            ++result.expanded_nodes;

            if (current_id == goal_node) {
                break;
            }

            const double current_g = g_score[current_id];
            if (!std::isfinite(current_g)) {
                continue;
            }

            for (const auto& edge : graph.adj[current_id]) {
                if (edge.to >= node_count) {
                    continue;
                }
                if (closed[edge.to] != 0) {
                    continue;
                }
                if (!(edge.weight >= 0.0) || !std::isfinite(edge.weight)) {
                    continue;
                }

                const double tentative_g = current_g + edge.weight;
                if (tentative_g + 1e-12 >= g_score[edge.to]) {
                    continue;
                }

                g_score[edge.to] = tentative_g;
                parent[edge.to] = current_id;

                const double heuristic_value = use_heuristic ? std::max(0.0, heuristic[edge.to]) : 0.0;
                open_set.push({edge.to, tentative_g + heuristic_value});
            }
        }

        if (closed[goal_node] == 0 || !std::isfinite(g_score[goal_node])) {
            return result;
        }

        std::vector<std::size_t> reversed_path;
        reversed_path.reserve(node_count);

        std::size_t cursor = goal_node;
        while (cursor != start_node) {
            reversed_path.push_back(cursor);
            const std::size_t next = parent[cursor];
            if (next >= node_count || next == cursor) {
                return GraphSearchResult{};
            }
            cursor = next;
        }
        reversed_path.push_back(start_node);

        std::reverse(reversed_path.begin(), reversed_path.end());
        result.found = true;
        result.total_cost = g_score[goal_node];
        result.node_path = std::move(reversed_path);
        return result;
    }

}
