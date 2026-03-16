#include "algorithms/GraphBasedPlanners.h"

#include "algorithms/search/GraphSearch.h"

#include <chrono>
#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace algorithms {

    namespace {

        struct GraphPlannerComputation {
            geometry::Path path;
            double total_cost = 0.0;
        };

        template <typename BuilderT>
        concept PathExpandableBuilder = requires(const BuilderT& builder,
                                                 const std::vector<std::size_t>& node_path) {
            { builder.build_path_from_node_path(node_path) } -> std::same_as<geometry::Path>;
        };

        template <typename BuilderT>
        std::optional<GraphPlannerComputation> compute_path_with_builder(
            BuilderT& builder,
            const geometry::Scene& scene,
            const search::GraphSearchStrategy strategy) {
            const graph::Graph graph = builder.build(scene);
            if (graph.adj.empty()) {
                return std::nullopt;
            }

            const std::optional<std::size_t> start_node_id = builder.get_node_id(scene.start);
            const std::optional<std::size_t> goal_node_id = builder.get_node_id(scene.goal);
            if (!start_node_id.has_value() || !goal_node_id.has_value()) {
                return std::nullopt;
            }

            std::vector<double> heuristic;
            if (strategy == search::GraphSearchStrategy::AStar) {
                const geometry::Point goal_point = builder.get_node_point(goal_node_id.value());
                heuristic.resize(graph.adj.size(), 0.0);
                for (std::size_t i = 0; i < graph.adj.size(); ++i) {
                    heuristic[i] = builder.get_node_point(i).distance(goal_point);
                }
            }

            const search::GraphSearchResult search_result = search::shortest_path(
                graph,
                start_node_id.value(),
                goal_node_id.value(),
                strategy,
                heuristic);
            if (!search_result.found || search_result.node_path.empty()) {
                return std::nullopt;
            }

            GraphPlannerComputation computation;
            computation.total_cost = search_result.total_cost;

            if constexpr (PathExpandableBuilder<BuilderT>) {
                computation.path = builder.build_path_from_node_path(search_result.node_path);
            } else {
                computation.path.points.reserve(search_result.node_path.size());
                for (const std::size_t node_id : search_result.node_path) {
                    computation.path.points.push_back(builder.get_node_point(node_id));
                }
            }

            return computation;
        }

        template <typename BuilderT>
        PathResult find_path_with_builder(BuilderT& builder,
                                          const geometry::Scene& scene,
                                          const search::GraphSearchStrategy strategy) {
            const auto computation = compute_path_with_builder(builder, scene, strategy);
            if (!computation.has_value()) {
                return std::nullopt;
            }
            return computation->path;
        }

        template <typename PlannerFn>
        BenchmarkResult benchmark_graph_planner(const geometry::Scene& scene,
                                                const std::string& algorithm_name,
                                                PlannerFn&& planner_fn) {
            const auto started_at = std::chrono::steady_clock::now();
            const auto computation = planner_fn(scene);
            const auto finished_at = std::chrono::steady_clock::now();

            BenchmarkResult benchmark;
            benchmark.algorithm_name = algorithm_name;
            benchmark.runtime_ms = std::chrono::duration<double, std::milli>(
                finished_at - started_at).count();
            if (computation.has_value()) {
                benchmark.path = std::move(computation->path);
                benchmark.exact_path_length = computation->total_cost;
            }
            return benchmark;
        }

    } // namespace

    GridDijkstraPlanner::GridDijkstraPlanner(const double grid_step, const bool allow_diagonal)
        : builder_(grid_step, allow_diagonal) {}

    PathResult GridDijkstraPlanner::find_path(const geometry::Scene& scene) {
        return find_path_with_builder(builder_, scene, search::GraphSearchStrategy::Dijkstra);
    }

    BenchmarkResult GridDijkstraPlanner::plan(const geometry::Scene& scene) {
        return benchmark_graph_planner(
            scene,
            name(),
            [this](const geometry::Scene& local_scene) {
                return compute_path_with_builder(builder_, local_scene, search::GraphSearchStrategy::Dijkstra);
            });
    }

    std::string GridDijkstraPlanner::name() const {
        return "grid_dijkstra";
    }

    GridAStarPlanner::GridAStarPlanner(const double grid_step, const bool allow_diagonal)
        : builder_(grid_step, allow_diagonal) {}

    PathResult GridAStarPlanner::find_path(const geometry::Scene& scene) {
        return find_path_with_builder(builder_, scene, search::GraphSearchStrategy::AStar);
    }

    BenchmarkResult GridAStarPlanner::plan(const geometry::Scene& scene) {
        return benchmark_graph_planner(
            scene,
            name(),
            [this](const geometry::Scene& local_scene) {
                return compute_path_with_builder(builder_, local_scene, search::GraphSearchStrategy::AStar);
            });
    }

    std::string GridAStarPlanner::name() const {
        return "grid_astar";
    }

    VisibilityDijkstraPlanner::VisibilityDijkstraPlanner(const int points_per_obstacle)
        : builder_(points_per_obstacle) {}

    PathResult VisibilityDijkstraPlanner::find_path(const geometry::Scene& scene) {
        return find_path_with_builder(builder_, scene, search::GraphSearchStrategy::Dijkstra);
    }

    BenchmarkResult VisibilityDijkstraPlanner::plan(const geometry::Scene& scene) {
        return benchmark_graph_planner(
            scene,
            name(),
            [this](const geometry::Scene& local_scene) {
                return compute_path_with_builder(builder_, local_scene, search::GraphSearchStrategy::Dijkstra);
            });
    }

    std::string VisibilityDijkstraPlanner::name() const {
        return "visibility_dijkstra";
    }

    VisibilityAStarPlanner::VisibilityAStarPlanner(const int points_per_obstacle)
        : builder_(points_per_obstacle) {}

    PathResult VisibilityAStarPlanner::find_path(const geometry::Scene& scene) {
        return find_path_with_builder(builder_, scene, search::GraphSearchStrategy::AStar);
    }

    BenchmarkResult VisibilityAStarPlanner::plan(const geometry::Scene& scene) {
        return benchmark_graph_planner(
            scene,
            name(),
            [this](const geometry::Scene& local_scene) {
                return compute_path_with_builder(builder_, local_scene, search::GraphSearchStrategy::AStar);
            });
    }

    std::string VisibilityAStarPlanner::name() const {
        return "visibility_astar";
    }

} // namespace algorithms


