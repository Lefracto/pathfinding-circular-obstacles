#ifndef ALGORITHMS_PLANNER_H
#define ALGORITHMS_PLANNER_H

#include <cstddef>
#include <optional>
#include <string>
#include "../geometry/Path.h"
#include "../geometry/Scene.h"

namespace algorithms {

    using PathResult = std::optional<geometry::Path>;

    struct BenchmarkResult {
        geometry::Path path;
        double runtime_ms = 0.0;
        std::string algorithm_name;
        std::optional<double> exact_path_length = std::nullopt;
        std::optional<double> graph_build_time_ms = std::nullopt;
        std::optional<double> graph_search_time_ms = std::nullopt;
        std::optional<double> graph_path_extraction_time_ms = std::nullopt;
        std::optional<std::size_t> graph_node_count = std::nullopt;
        std::optional<std::size_t> graph_edge_count = std::nullopt;
        std::optional<std::size_t> graph_expanded_nodes = std::nullopt;
    };

    class Planner {
    public:
        [[nodiscard]] virtual PathResult find_path(const geometry::Scene& scene) = 0;
        [[nodiscard]] virtual BenchmarkResult plan(const geometry::Scene& scene) = 0;
        [[nodiscard]] virtual std::string name() const = 0;

        virtual ~Planner() = default;
    };

}

#endif
