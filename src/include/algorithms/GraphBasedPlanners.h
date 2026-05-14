#ifndef ALGORITHMS_GRAPH_BASED_PLANNERS_H
#define ALGORITHMS_GRAPH_BASED_PLANNERS_H

#include "GridGraphBuilder.h"
#include "Planner.h"
#include "VisibilityGraphBuilder.h"

namespace algorithms {

    class GridDijkstraPlanner final : public Planner {
    public:
        explicit GridDijkstraPlanner(double grid_step = 5.0,
                                     bool allow_diagonal = true,
                                     bool use_obstacle_spatial_index = false);

        [[nodiscard]] PathResult find_path(const geometry::Scene& scene) override;
        [[nodiscard]] BenchmarkResult plan(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;

    private:
        graph::GridGraphBuilder builder_;
    };

    class GridAStarPlanner final : public Planner {
    public:
        explicit GridAStarPlanner(double grid_step = 5.0,
                                  bool allow_diagonal = true,
                                  bool use_obstacle_spatial_index = false);

        [[nodiscard]] PathResult find_path(const geometry::Scene& scene) override;
        [[nodiscard]] BenchmarkResult plan(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;

    private:
        graph::GridGraphBuilder builder_;
    };

    class VisibilityDijkstraPlanner final : public Planner {
    public:
        explicit VisibilityDijkstraPlanner(int points_per_obstacle = 12);

        [[nodiscard]] PathResult find_path(const geometry::Scene& scene) override;
        [[nodiscard]] BenchmarkResult plan(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;

    private:
        graph::VisibilityGraphBuilder builder_;
    };

    class VisibilityAStarPlanner final : public Planner {
    public:
        explicit VisibilityAStarPlanner(int points_per_obstacle = 12);

        [[nodiscard]] PathResult find_path(const geometry::Scene& scene) override;
        [[nodiscard]] BenchmarkResult plan(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;

    private:
        graph::VisibilityGraphBuilder builder_;
    };

}

#endif
