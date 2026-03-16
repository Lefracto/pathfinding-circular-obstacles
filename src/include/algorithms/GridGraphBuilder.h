#ifndef ALGORITHMS_GRID_GRAPH_BUILDER_H
#define ALGORITHMS_GRID_GRAPH_BUILDER_H

#include "GraphBuilder.h"
#include "../geometry/Point.h"
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace algorithms::graph {

    class GridGraphBuilder : public GraphBuilder {
    public:
        /**
         * @param grid_step Size of each grid cell (default: 5.0)
         * @param allow_diagonal Whether to allow diagonal connections (8-connectivity vs 4-connectivity)
         */
        explicit GridGraphBuilder(double grid_step = 5.0, bool allow_diagonal = true);

        /**
         * Build a graph from the given scene.
         * @param scene Scene to convert to graph
         * @return Graph representation of the scene
         */
        [[nodiscard]] Graph build(const geometry::Scene& scene) override;

        /**
         * Get the name of this graph builder
         */
        [[nodiscard]] std::string name() const override;

        /**
         * Get the point (coordinates) corresponding to a graph node.
         * @param node_id Node index in the graph
         * @return Point coordinates of the node, or throws if node_id is invalid
         */
        [[nodiscard]] geometry::Point get_node_point(std::size_t node_id) const;

        /**
         * Get the node ID corresponding to a point (if it exists in the graph).
         * @param point Point to find
         * @return Node ID if found, or std::nullopt if point is not a valid node
         */
        [[nodiscard]] std::optional<std::size_t> get_node_id(const geometry::Point& point) const;

        [[nodiscard]] std::optional<std::size_t> get_start_node_id() const { return start_node_id_; }
        [[nodiscard]] std::optional<std::size_t> get_goal_node_id() const { return goal_node_id_; }

        /**
         * Get the number of nodes in the last built graph
         */
        [[nodiscard]] std::size_t node_count() const { return node_to_point_.size(); }

        /**
         * Get grid step size
         */
        [[nodiscard]] double get_grid_step() const { return grid_step_; }

        /**
         * Set grid step size
         */
        void set_grid_step(double step) { grid_step_ = step; }

        /**
         * Check if diagonal connections are allowed
         */
        [[nodiscard]] bool is_diagonal_allowed() const { return allow_diagonal_; }

        /**
         * Set whether diagonal connections are allowed
         */
        void set_allow_diagonal(bool allow) { allow_diagonal_ = allow; }

    private:
        double grid_step_;
        bool allow_diagonal_;
        
        // Mapping from node ID to point coordinates
        std::vector<geometry::Point> node_to_point_;
        
        // Mapping from point to node ID (for fast lookup)
        std::unordered_map<std::size_t, std::size_t> point_to_node_;
        std::optional<std::size_t> start_node_id_ = std::nullopt;
        std::optional<std::size_t> goal_node_id_ = std::nullopt;

        /**
         * Check if a point is inside any obstacle
         */
        [[nodiscard]] bool is_point_in_obstacle(const geometry::Point& point, const geometry::Scene& scene) const;

        /**
         * Check if a point is within scene bounds
         */
        [[nodiscard]] bool is_point_in_bounds(const geometry::Point& point, const geometry::Scene& scene) const;

        [[nodiscard]] bool is_segment_collision_free(const geometry::Point& a,
                                                     const geometry::Point& b,
                                                     const geometry::Scene& scene) const;

        /**
         * Calculate distance between two points (for edge weights)
         */
        [[nodiscard]] double calculate_distance(const geometry::Point& a, const geometry::Point& b) const;

        /**
         * Get grid cell indices for a point
         */
        [[nodiscard]] std::pair<int, int> point_to_grid(const geometry::Point& point) const;

        /**
         * Get point coordinates for grid cell center
         */
        [[nodiscard]] geometry::Point grid_to_point(int grid_x, int grid_y) const;

        [[nodiscard]] static std::size_t make_grid_key(int grid_x, int grid_y, int grid_width);
    };

} // namespace algorithms::graph

#endif //ALGORITHMS_GRID_GRAPH_BUILDER_H
