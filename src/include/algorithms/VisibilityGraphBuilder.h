#ifndef ALGORITHMS_VISIBILITY_GRAPH_BUILDER_H
#define ALGORITHMS_VISIBILITY_GRAPH_BUILDER_H

#include "GraphBuilder.h"
#include "../geometry/Path.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace algorithms::graph {

    class VisibilityGraphBuilder : public GraphBuilder {
    public:
        explicit VisibilityGraphBuilder(int points_per_obstacle = 8);

        [[nodiscard]] Graph build(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;
        [[nodiscard]] geometry::Point get_node_point(std::size_t node_id) const;
        [[nodiscard]] std::optional<std::size_t> get_node_id(const geometry::Point& point) const;
        [[nodiscard]] std::size_t node_count() const { return node_to_point_.size(); }
        void set_points_per_obstacle(int points);
        [[nodiscard]] int get_points_per_obstacle() const { return points_per_obstacle_; }

        [[nodiscard]] geometry::Path build_path_from_node_path(
            const std::vector<std::size_t>& node_path) const;

    private:
        struct DirectedEdgeKey {
            std::size_t from = 0;
            std::size_t to = 0;

            bool operator==(const DirectedEdgeKey& other) const = default;
        };

        struct DirectedEdgeKeyHash {
            [[nodiscard]] std::size_t operator()(const DirectedEdgeKey& key) const noexcept;
        };

        struct NodeBucketKey {
            std::int64_t x = 0;
            std::int64_t y = 0;

            bool operator==(const NodeBucketKey& other) const = default;
        };

        struct NodeBucketKeyHash {
            [[nodiscard]] std::size_t operator()(const NodeBucketKey& key) const noexcept;
        };

        struct ObstacleCellKey {
            int x = 0;
            int y = 0;

            bool operator==(const ObstacleCellKey& other) const = default;
        };

        struct ObstacleCellKeyHash {
            [[nodiscard]] std::size_t operator()(const ObstacleCellKey& key) const noexcept;
        };

        struct ArcEdgeInfo {
            geometry::Point center{};
            double radius = 0.0;
            double start_angle = 0.0;
            double signed_delta = 0.0;
        };

        int points_per_obstacle_;

        std::vector<geometry::Point> node_to_point_;
        std::vector<std::optional<std::size_t>> node_owner_obstacle_;
        std::unordered_map<DirectedEdgeKey, ArcEdgeInfo, DirectedEdgeKeyHash> directed_arc_edges_;
        std::unordered_map<NodeBucketKey, std::vector<std::size_t>, NodeBucketKeyHash> node_lookup_;
        std::unordered_map<ObstacleCellKey, std::vector<std::size_t>, ObstacleCellKeyHash> obstacle_grid_;
        double obstacle_grid_cell_size_ = 1.0;
        double obstacle_grid_inv_cell_size_ = 1.0;
        double max_obstacle_radius_ = 0.0;
        mutable std::vector<std::uint32_t> obstacle_query_marks_;
        mutable std::uint32_t obstacle_query_stamp_ = 1;
        mutable std::vector<std::size_t> obstacle_query_buffer_;

        [[nodiscard]] bool segment_is_collision_free(
            const geometry::Point& a,
            const geometry::Point& b,
            const geometry::Scene& scene,
            std::optional<std::size_t> ignored_first = std::nullopt,
            std::optional<std::size_t> ignored_second = std::nullopt) const;

        [[nodiscard]] bool arc_is_collision_free(
            const geometry::Scene& scene,
            std::size_t obstacle_index,
            double start_angle,
            double end_angle) const;

        [[nodiscard]] std::size_t add_or_get_node(
            const geometry::Point& point,
            std::optional<std::size_t> owner_obstacle);

        void add_undirected_edge(Graph& graph, std::size_t from, std::size_t to, double weight) const;
        void build_obstacle_spatial_index(const geometry::Scene& scene);
        void collect_obstacle_candidates_in_aabb(double min_x,
                                                double min_y,
                                                double max_x,
                                                double max_y) const;
        [[nodiscard]] bool point_is_inside_other_obstacle(const geometry::Point& point,
                                                          const geometry::Scene& scene,
                                                          std::size_t self_obstacle_index) const;

        static std::vector<std::pair<geometry::Point, geometry::Point>> circle_circle_tangents(
            const geometry::Disk& first,
            const geometry::Disk& second);

        static std::vector<geometry::Point> point_circle_tangency_points(
            const geometry::Point& point,
            const geometry::Disk& disk);

        [[nodiscard]] static DirectedEdgeKey make_directed_edge_key(std::size_t from, std::size_t to);
        [[nodiscard]] static double normalize_angle(double angle);
        [[nodiscard]] static double positive_angle_delta(double from, double to);
        [[nodiscard]] static NodeBucketKey make_node_bucket_key(const geometry::Point& point);
        [[nodiscard]] double calculate_distance(const geometry::Point& a, const geometry::Point& b) const;
    };

}

#endif
