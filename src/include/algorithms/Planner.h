//
// Created by USER on 26/12/2025.
//

#ifndef ALGORITHMS_PLANNER_H
#define ALGORITHMS_PLANNER_H

#include <string>
#include <chrono>
#include <optional>
#include "../geometry/scene.h"
#include "../geometry/path.h"

namespace algorithms {

    using PathResult = std::optional<geometry::Path>;

    struct BenchmarkResult {
        geometry::Path path;
        double runtime_ms = 0.0;
        std::string algorithm_name;
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
