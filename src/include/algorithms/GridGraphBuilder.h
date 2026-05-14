#ifndef ALGORITHMS_GRID_GRAPH_BUILDER_H
#define ALGORITHMS_GRID_GRAPH_BUILDER_H

#include "GraphBuilder.h"
#include "../geometry/Point.h"
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <optional>
#include <string>

namespace algorithms::graph {

    class GridGraphBuilder : public GraphBuilder {
    public:
        explicit GridGraphBuilder(double grid_step = 5.0,
                                  bool allow_diagonal = true,
                                  bool use_obstacle_spatial_index = false);

        [[nodiscard]] Graph build(const geometry::Scene& scene) override;

        [[nodiscard]] std::string name() const override;

        [[nodiscard]] geometry::Point get_node_point(std::size_t node_id) const;

        [[nodiscard]] std::optional<std::size_t> get_node_id(const geometry::Point& point) const;

        [[nodiscard]] std::optional<std::size_t> get_start_node_id() const { return start_node_id_; }

        [[nodiscard]] std::optional<std::size_t> get_goal_node_id() const { return goal_node_id_; }

        [[nodiscard]] std::size_t node_count() const { return node_to_point_.size(); }

        [[nodiscard]] double get_grid_step() const { return grid_step_; }

        void set_grid_step(double step) { grid_step_ = step; }

        [[nodiscard]] bool is_diagonal_allowed() const { return allow_diagonal_; }

        void set_allow_diagonal(bool allow) { allow_diagonal_ = allow; }

        [[nodiscard]] bool uses_obstacle_spatial_index() const { return use_obstacle_spatial_index_; }

        void set_use_obstacle_spatial_index(bool use_index) { use_obstacle_spatial_index_ = use_index; }

    private:
        struct ObstacleCellKey {
            int x = 0;
            int y = 0;

            bool operator==(const ObstacleCellKey& other) const = default;
        };

        struct ObstacleCellKeyHash {
            [[nodiscard]] std::size_t operator()(const ObstacleCellKey& key) const noexcept;
        };

        double grid_step_;
        bool allow_diagonal_;
        bool use_obstacle_spatial_index_;
        
        std::vector<geometry::Point> node_to_point_;
        std::unordered_map<std::size_t, std::size_t> point_to_node_;
        std::optional<std::size_t> start_node_id_ = std::nullopt;
        std::optional<std::size_t> goal_node_id_ = std::nullopt;
        std::unordered_map<ObstacleCellKey, std::vector<std::size_t>, ObstacleCellKeyHash> obstacle_grid_;
        double obstacle_grid_cell_size_ = 1.0;
        double obstacle_grid_inv_cell_size_ = 1.0;
        double max_obstacle_radius_ = 0.0;
        mutable std::vector<std::uint32_t> obstacle_query_marks_;
        mutable std::uint32_t obstacle_query_stamp_ = 1;
        mutable std::vector<std::size_t> obstacle_query_buffer_;

        [[nodiscard]] bool is_point_in_obstacle(const geometry::Point& point, const geometry::Scene& scene) const;

        [[nodiscard]] bool is_point_in_bounds(const geometry::Point& point, const geometry::Scene& scene) const;

        [[nodiscard]] bool is_segment_collision_free(const geometry::Point& a,
                                                     const geometry::Point& b,
                                                     const geometry::Scene& scene) const;

        [[nodiscard]] double calculate_distance(const geometry::Point& a, const geometry::Point& b) const;

        [[nodiscard]] std::pair<int, int> point_to_grid(const geometry::Point& point) const;

        [[nodiscard]] geometry::Point grid_to_point(int grid_x, int grid_y) const;

        void build_obstacle_spatial_index(const geometry::Scene& scene);

        void clear_obstacle_spatial_index();

        [[nodiscard]] Graph build_with_spatial_index(const geometry::Scene& scene);

        void collect_obstacle_candidates_in_aabb(double min_x,
                                                 double min_y,
                                                 double max_x,
                                                 double max_y) const;

        [[nodiscard]] static std::size_t make_grid_key(int grid_x, int grid_y, int grid_width);
    };

}

#endif
