#include "algorithms/GraphBasedPlanners.h"

#include "../../include/algorithms/GraphSearch.h"

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
            bool found = false;
            double graph_build_time_ms = 0.0;
            double graph_search_time_ms = 0.0;
            double graph_path_extraction_time_ms = 0.0;
            std::size_t graph_node_count = 0;
            std::size_t graph_edge_count = 0;
            std::size_t graph_expanded_nodes = 0;
        };

        double elapsed_ms(const std::chrono::steady_clock::time_point started_at,
                          const std::chrono::steady_clock::time_point finished_at) {
            return std::chrono::duration<double, std::milli>(finished_at - started_at).count();
        }

        std::size_t count_graph_edges(const graph::Graph& graph) {
            std::size_t edge_count = 0;
            for (const auto& adjacency : graph.adj) {
                edge_count += adjacency.size();
            }
            return edge_count;
        }

        template <typename BuilderT>
        concept PathExpandableBuilder = requires(const BuilderT& builder,
                                                 const std::vector<std::size_t>& node_path) {
            { builder.build_path_from_node_path(node_path) } -> std::same_as<geometry::Path>;
        };

        template <typename BuilderT>
        GraphPlannerComputation compute_path_with_builder(
            BuilderT& builder,
            const geometry::Scene& scene,
            const search::GraphSearchStrategy strategy) {
            GraphPlannerComputation computation;

            const auto build_started_at = std::chrono::steady_clock::now();
            const graph::Graph graph = builder.build(scene);
            const auto build_finished_at = std::chrono::steady_clock::now();
            computation.graph_build_time_ms = elapsed_ms(build_started_at, build_finished_at);
            computation.graph_node_count = graph.adj.size();
            computation.graph_edge_count = count_graph_edges(graph);

            if (graph.adj.empty()) {
                return computation;
            }

            const auto search_started_at = std::chrono::steady_clock::now();
            const std::optional<std::size_t> start_node_id = builder.get_node_id(scene.start);
            const std::optional<std::size_t> goal_node_id = builder.get_node_id(scene.goal);
            if (!start_node_id.has_value() || !goal_node_id.has_value()) {
                const auto search_finished_at = std::chrono::steady_clock::now();
                computation.graph_search_time_ms = elapsed_ms(search_started_at, search_finished_at);
                return computation;
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
            const auto search_finished_at = std::chrono::steady_clock::now();
            computation.graph_search_time_ms = elapsed_ms(search_started_at, search_finished_at);
            computation.graph_expanded_nodes = search_result.expanded_nodes;

            if (!search_result.found || search_result.node_path.empty()) {
                return computation;
            }

            computation.total_cost = search_result.total_cost;

            const auto path_started_at = std::chrono::steady_clock::now();
            if constexpr (PathExpandableBuilder<BuilderT>) {
                computation.path = builder.build_path_from_node_path(search_result.node_path);
            } else {
                computation.path.points.reserve(search_result.node_path.size());
                for (const std::size_t node_id : search_result.node_path) {
                    computation.path.points.push_back(builder.get_node_point(node_id));
                }
            }
            const auto path_finished_at = std::chrono::steady_clock::now();
            computation.graph_path_extraction_time_ms = elapsed_ms(path_started_at, path_finished_at);
            computation.found = true;

            return computation;
        }

        template <typename BuilderT>
        PathResult find_path_with_builder(BuilderT& builder,
                                          const geometry::Scene& scene,
            const search::GraphSearchStrategy strategy) {
            const auto computation = compute_path_with_builder(builder, scene, strategy);
            if (!computation.found) {
                return std::nullopt;
            }
            return computation.path;
        }

        template <typename PlannerFn>
        BenchmarkResult benchmark_graph_planner(const geometry::Scene& scene,
            const std::string& algorithm_name,
            PlannerFn&& planner_fn) {
            const auto started_at = std::chrono::steady_clock::now();
            auto computation = planner_fn(scene);
            const auto finished_at = std::chrono::steady_clock::now();

            BenchmarkResult benchmark;
            benchmark.algorithm_name = algorithm_name;
            benchmark.runtime_ms = elapsed_ms(started_at, finished_at);
            benchmark.graph_build_time_ms = computation.graph_build_time_ms;
            benchmark.graph_search_time_ms = computation.graph_search_time_ms;
            benchmark.graph_path_extraction_time_ms = computation.graph_path_extraction_time_ms;
            benchmark.graph_node_count = computation.graph_node_count;
            benchmark.graph_edge_count = computation.graph_edge_count;
            benchmark.graph_expanded_nodes = computation.graph_expanded_nodes;
            if (computation.found) {
                benchmark.path = std::move(computation.path);
                benchmark.exact_path_length = computation.total_cost;
            }
            return benchmark;
        }

    } // namespace

    GridDijkstraPlanner::GridDijkstraPlanner(const double grid_step,
                                             const bool allow_diagonal,
                                             const bool use_obstacle_spatial_index)
        : builder_(grid_step, allow_diagonal, use_obstacle_spatial_index) {}

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

    GridAStarPlanner::GridAStarPlanner(const double grid_step,
                                       const bool allow_diagonal,
                                       const bool use_obstacle_spatial_index)
        : builder_(grid_step, allow_diagonal, use_obstacle_spatial_index) {}

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

}
