#include "../../include/algorithms/VisibilityGraphBuilder.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace algorithms::graph {

    namespace {

        constexpr double kPi = 3.14159265358979323846;
        constexpr double kPointEpsilon = 1e-6;
        constexpr double kCollisionEpsilon = 1e-9;

        bool point_in_scene_bounds(const geometry::Point& point, const geometry::Scene& scene) {
            return point.x >= 0.0 && point.x <= scene.width &&
                   point.y >= 0.0 && point.y <= scene.height;
        }

        bool points_are_close(const geometry::Point& first,
                              const geometry::Point& second,
                              const double epsilon = kPointEpsilon) {
            return first.distance(second) <= epsilon;
        }

        void append_unique_node(std::vector<std::size_t>& nodes, const std::size_t node_id) {
            if (std::find(nodes.begin(), nodes.end(), node_id) == nodes.end()) {
                nodes.push_back(node_id);
            }
        }

        void append_path_point(geometry::Path& path, const geometry::Point& point) {
            if (!path.points.empty() && points_are_close(path.points.back(), point)) {
                return;
            }
            path.points.push_back(point);
        }

        double distance_sq_point_to_segment(const geometry::Point& point,
                                            const geometry::Point& segment_start,
                                            const geometry::Point& segment_end) {
            const double vx = segment_end.x - segment_start.x;
            const double vy = segment_end.y - segment_start.y;
            const double len_sq = vx * vx + vy * vy;
            if (len_sq <= 1e-12) {
                const double dx = point.x - segment_start.x;
                const double dy = point.y - segment_start.y;
                return dx * dx + dy * dy;
            }

            const double wx = point.x - segment_start.x;
            const double wy = point.y - segment_start.y;
            const double t_unclamped = (wx * vx + wy * vy) / len_sq;
            const double t = std::max(0.0, std::min(1.0, t_unclamped));

            const double closest_x = segment_start.x + t * vx;
            const double closest_y = segment_start.y + t * vy;
            const double dx = point.x - closest_x;
            const double dy = point.y - closest_y;
            return dx * dx + dy * dy;
        }

        double estimate_obstacle_grid_cell_size(const geometry::Scene& scene) {
            if (scene.obstacles.empty()) {
                return std::max(1.0, std::min(scene.width, scene.height));
            }

            double radius_sum = 0.0;
            for (const auto& obstacle : scene.obstacles) {
                radius_sum += std::max(0.0, obstacle.radius);
            }

            const double obstacle_count = static_cast<double>(scene.obstacles.size());
            const double avg_radius = radius_sum / obstacle_count;
            const double scene_area = std::max(1.0, scene.width) * std::max(1.0, scene.height);
            const double area_based = std::sqrt(scene_area / obstacle_count);
            return std::max({1.0, avg_radius * 2.0, area_based * 0.5});
        }

    } // namespace

    VisibilityGraphBuilder::VisibilityGraphBuilder(const int points_per_obstacle)
        : points_per_obstacle_(points_per_obstacle) {
        if (points_per_obstacle_ < 3) {
            throw std::invalid_argument("Points per obstacle must be at least 3");
        }
    }

    void VisibilityGraphBuilder::set_points_per_obstacle(const int points) {
        if (points < 3) {
            throw std::invalid_argument("Points per obstacle must be at least 3");
        }
        points_per_obstacle_ = points;
    }

    Graph VisibilityGraphBuilder::build(const geometry::Scene& scene) {
        node_to_point_.clear();
        node_owner_obstacle_.clear();
        directed_arc_edges_.clear();
        node_lookup_.clear();
        build_obstacle_spatial_index(scene);

        struct PendingEdge {
            std::size_t from = 0;
            std::size_t to = 0;
            double weight = 0.0;
        };

        std::vector<PendingEdge> pending_edges;
        std::vector<std::vector<std::size_t>> obstacle_nodes(scene.obstacles.size());
        pending_edges.reserve(scene.obstacles.size() * 8);
        node_lookup_.reserve(scene.obstacles.size() * 8 + 2);
        directed_arc_edges_.reserve(scene.obstacles.size() * 8);

        auto register_segment = [&](const geometry::Point& first,
                                    const geometry::Point& second,
                                    const std::optional<std::size_t> ignored_first,
                                    const std::optional<std::size_t> ignored_second,
                                    const std::optional<std::size_t> first_obstacle,
                                    const std::optional<std::size_t> second_obstacle) {
            if (!segment_is_collision_free(first, second, scene, ignored_first, ignored_second)) {
                return;
            }

            const std::size_t from = add_or_get_node(first, first_obstacle);
            const std::size_t to = add_or_get_node(second, second_obstacle);
            if (from == to) {
                return;
            }

            pending_edges.push_back({from, to, calculate_distance(first, second)});

            if (first_obstacle.has_value()) {
                append_unique_node(obstacle_nodes[first_obstacle.value()], from);
            }
            if (second_obstacle.has_value()) {
                append_unique_node(obstacle_nodes[second_obstacle.value()], to);
            }
        };

        add_or_get_node(scene.start, std::nullopt);
        add_or_get_node(scene.goal, std::nullopt);

        register_segment(scene.start, scene.goal, std::nullopt, std::nullopt, std::nullopt, std::nullopt);

        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];

            for (const auto& tangent_point : point_circle_tangency_points(scene.start, obstacle)) {
                register_segment(scene.start, tangent_point, obstacle_index, std::nullopt, std::nullopt, obstacle_index);
            }

            for (const auto& tangent_point : point_circle_tangency_points(scene.goal, obstacle)) {
                register_segment(scene.goal, tangent_point, obstacle_index, std::nullopt, std::nullopt, obstacle_index);
            }
        }

        for (std::size_t first = 0; first < scene.obstacles.size(); ++first) {
            for (std::size_t second = first + 1; second < scene.obstacles.size(); ++second) {
                const auto tangent_candidates =
                    circle_circle_tangents(scene.obstacles[first], scene.obstacles[second]);
                for (const auto& tangent : tangent_candidates) {
                    register_segment(tangent.first, tangent.second, first, second, first, second);
                }
            }
        }

        Graph graph;
        graph.adj.resize(node_to_point_.size());
        for (const auto& edge : pending_edges) {
            add_undirected_edge(graph, edge.from, edge.to, edge.weight);
        }

        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];
            const auto& obstacle_node_ids = obstacle_nodes[obstacle_index];
            if (obstacle_node_ids.size() < 2) {
                continue;
            }

            std::vector<std::pair<double, std::size_t>> nodes_by_angle;
            nodes_by_angle.reserve(obstacle_node_ids.size());
            for (const std::size_t node_id : obstacle_node_ids) {
                const auto& point = node_to_point_[node_id];
                const double angle = normalize_angle(std::atan2(
                    point.y - obstacle.center.y,
                    point.x - obstacle.center.x));
                nodes_by_angle.emplace_back(angle, node_id);
            }

            std::sort(nodes_by_angle.begin(), nodes_by_angle.end(),
                      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

            std::vector<std::pair<double, std::size_t>> unique_nodes_by_angle;
            unique_nodes_by_angle.reserve(nodes_by_angle.size());
            for (const auto& entry : nodes_by_angle) {
                if (unique_nodes_by_angle.empty() ||
                    positive_angle_delta(unique_nodes_by_angle.back().first, entry.first) > 1e-4) {
                    unique_nodes_by_angle.push_back(entry);
                }
            }

            if (unique_nodes_by_angle.size() >= 2 &&
                positive_angle_delta(unique_nodes_by_angle.back().first,
                                     unique_nodes_by_angle.front().first) < 1e-4) {
                unique_nodes_by_angle.pop_back();
            }

            if (unique_nodes_by_angle.size() < 2) {
                continue;
            }

            for (std::size_t index = 0; index < unique_nodes_by_angle.size(); ++index) {
                const auto& current = unique_nodes_by_angle[index];
                const auto& next = unique_nodes_by_angle[(index + 1) % unique_nodes_by_angle.size()];
                if (current.second == next.second) {
                    continue;
                }

                if (!arc_is_collision_free(scene, obstacle_index, current.first, next.first)) {
                    continue;
                }

                const double delta = positive_angle_delta(current.first, next.first);
                if (delta <= 1e-9) {
                    continue;
                }

                const double weight = obstacle.radius * delta;
                add_undirected_edge(graph, current.second, next.second, weight);

                directed_arc_edges_[make_directed_edge_key(current.second, next.second)] = {
                    obstacle.center,
                    obstacle.radius,
                    current.first,
                    delta
                };
                directed_arc_edges_[make_directed_edge_key(next.second, current.second)] = {
                    obstacle.center,
                    obstacle.radius,
                    next.first,
                    -delta
                };
            }
        }

        return graph;
    }

    std::string VisibilityGraphBuilder::name() const {
        return "VisibilityGraphBuilder";
    }

    geometry::Point VisibilityGraphBuilder::get_node_point(const std::size_t node_id) const {
        if (node_id >= node_to_point_.size()) {
            throw std::out_of_range("Node ID out of range");
        }
        return node_to_point_[node_id];
    }

    std::optional<std::size_t> VisibilityGraphBuilder::get_node_id(const geometry::Point& point) const {
        const NodeBucketKey bucket = make_node_bucket_key(point);
        for (std::int64_t dy = -1; dy <= 1; ++dy) {
            for (std::int64_t dx = -1; dx <= 1; ++dx) {
                const NodeBucketKey candidate_key{bucket.x + dx, bucket.y + dy};
                const auto it = node_lookup_.find(candidate_key);
                if (it == node_lookup_.end()) {
                    continue;
                }
                for (const std::size_t node_id : it->second) {
                    if (points_are_close(node_to_point_[node_id], point)) {
                        return node_id;
                    }
                }
            }
        }
        return std::nullopt;
    }

    geometry::Path VisibilityGraphBuilder::build_path_from_node_path(
        const std::vector<std::size_t>& node_path) const {
        geometry::Path path;
        if (node_path.empty()) {
            return path;
        }

        append_path_point(path, get_node_point(node_path.front()));
        for (std::size_t index = 0; index + 1 < node_path.size(); ++index) {
            const std::size_t from = node_path[index];
            const std::size_t to = node_path[index + 1];
            const auto arc_it = directed_arc_edges_.find(make_directed_edge_key(from, to));
            if (arc_it == directed_arc_edges_.end()) {
                append_path_point(path, get_node_point(to));
                continue;
            }

            const auto& arc = arc_it->second;
            const int full_circle_segments = std::max(points_per_obstacle_ * 4, 48);
            const double angle_step = (2.0 * kPi) / static_cast<double>(full_circle_segments);
            const int segment_count = std::max(
                2,
                static_cast<int>(std::ceil(std::abs(arc.signed_delta) / angle_step)));

            for (int step = 1; step <= segment_count; ++step) {
                const double t = static_cast<double>(step) / static_cast<double>(segment_count);
                const double angle = arc.start_angle + arc.signed_delta * t;
                append_path_point(path, {
                    arc.center.x + arc.radius * std::cos(angle),
                    arc.center.y + arc.radius * std::sin(angle)
                });
            }
        }

        return path;
    }

    bool VisibilityGraphBuilder::segment_is_collision_free(
        const geometry::Point& a,
        const geometry::Point& b,
        const geometry::Scene& scene,
        const std::optional<std::size_t> ignored_first,
        const std::optional<std::size_t> ignored_second) const {
        if (!point_in_scene_bounds(a, scene) || !point_in_scene_bounds(b, scene)) {
            return false;
        }

        const double min_x = std::min(a.x, b.x) - max_obstacle_radius_;
        const double min_y = std::min(a.y, b.y) - max_obstacle_radius_;
        const double max_x = std::max(a.x, b.x) + max_obstacle_radius_;
        const double max_y = std::max(a.y, b.y) + max_obstacle_radius_;
        collect_obstacle_candidates_in_aabb(min_x, min_y, max_x, max_y);

        for (const std::size_t obstacle_index : obstacle_query_buffer_) {
            if ((ignored_first.has_value() && ignored_first.value() == obstacle_index) ||
                (ignored_second.has_value() && ignored_second.value() == obstacle_index)) {
                continue;
            }

            const auto& obstacle = scene.obstacles[obstacle_index];
            if (obstacle.radius <= 0.0) {
                continue;
            }

            const double distance_sq = distance_sq_point_to_segment(obstacle.center, a, b);
            if (distance_sq < obstacle.radius * obstacle.radius - kCollisionEpsilon) {
                return false;
            }
        }

        return true;
    }

    bool VisibilityGraphBuilder::arc_is_collision_free(
        const geometry::Scene& scene,
        const std::size_t obstacle_index,
        const double start_angle,
        const double end_angle) const {
        if (obstacle_index >= scene.obstacles.size()) {
            return false;
        }

        const auto& obstacle = scene.obstacles[obstacle_index];
        if (obstacle.radius <= 0.0) {
            return false;
        }

        const double delta = positive_angle_delta(start_angle, end_angle);
        if (delta <= 1e-9) {
            return false;
        }

        const int samples = std::max(8, static_cast<int>(std::ceil(delta / (kPi / 24.0))));
        for (int sample = 1; sample < samples; ++sample) {
            const double t = static_cast<double>(sample) / static_cast<double>(samples);
            const double angle = start_angle + delta * t;
            const geometry::Point point{
                obstacle.center.x + obstacle.radius * std::cos(angle),
                obstacle.center.y + obstacle.radius * std::sin(angle)
            };

            if (!point_in_scene_bounds(point, scene)) {
                return false;
            }
            if (point_is_inside_other_obstacle(point, scene, obstacle_index)) {
                return false;
            }
        }

        return true;
    }

    std::size_t VisibilityGraphBuilder::add_or_get_node(
        const geometry::Point& point,
        const std::optional<std::size_t> owner_obstacle) {
        const NodeBucketKey bucket = make_node_bucket_key(point);
        for (std::int64_t dy = -1; dy <= 1; ++dy) {
            for (std::int64_t dx = -1; dx <= 1; ++dx) {
                const NodeBucketKey candidate_key{bucket.x + dx, bucket.y + dy};
                const auto it = node_lookup_.find(candidate_key);
                if (it == node_lookup_.end()) {
                    continue;
                }

                for (const std::size_t node_id : it->second) {
                    if (!points_are_close(node_to_point_[node_id], point)) {
                        continue;
                    }

                    if (!node_owner_obstacle_[node_id].has_value() && owner_obstacle.has_value()) {
                        node_owner_obstacle_[node_id] = owner_obstacle;
                    }
                    return node_id;
                }
            }
        }

        node_to_point_.push_back(point);
        node_owner_obstacle_.push_back(owner_obstacle);
        node_lookup_[bucket].push_back(node_to_point_.size() - 1);
        return node_to_point_.size() - 1;
    }

    void VisibilityGraphBuilder::add_undirected_edge(
        Graph& graph,
        const std::size_t from,
        const std::size_t to,
        const double weight) const {
        if (from >= graph.adj.size() || to >= graph.adj.size()) {
            throw std::out_of_range("Graph node index out of range");
        }

        const auto add_or_update = [&](const std::size_t source, const std::size_t target) {
            for (auto& edge : graph.adj[source]) {
                if (edge.to == target) {
                    edge.weight = std::min(edge.weight, weight);
                    return;
                }
            }
            graph.adj[source].push_back({target, weight});
        };

        add_or_update(from, to);
        add_or_update(to, from);
    }

    void VisibilityGraphBuilder::build_obstacle_spatial_index(const geometry::Scene& scene) {
        obstacle_grid_.clear();
        obstacle_query_buffer_.clear();
        obstacle_query_marks_.assign(scene.obstacles.size(), 0);
        obstacle_query_stamp_ = 1;
        max_obstacle_radius_ = 0.0;

        if (scene.obstacles.empty()) {
            obstacle_grid_cell_size_ = 1.0;
            obstacle_grid_inv_cell_size_ = 1.0;
            return;
        }

        obstacle_grid_cell_size_ = estimate_obstacle_grid_cell_size(scene);
        obstacle_grid_inv_cell_size_ = 1.0 / obstacle_grid_cell_size_;
        obstacle_grid_.reserve(scene.obstacles.size() * 4);

        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];
            max_obstacle_radius_ = std::max(max_obstacle_radius_, std::max(0.0, obstacle.radius));

            const int min_cell_x = static_cast<int>(std::floor((obstacle.center.x - obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int max_cell_x = static_cast<int>(std::floor((obstacle.center.x + obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int min_cell_y = static_cast<int>(std::floor((obstacle.center.y - obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int max_cell_y = static_cast<int>(std::floor((obstacle.center.y + obstacle.radius) * obstacle_grid_inv_cell_size_));

            for (int cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
                for (int cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
                    obstacle_grid_[{cell_x, cell_y}].push_back(obstacle_index);
                }
            }
        }
    }

    void VisibilityGraphBuilder::collect_obstacle_candidates_in_aabb(
        const double min_x,
        const double min_y,
        const double max_x,
        const double max_y) const {
        obstacle_query_buffer_.clear();
        if (obstacle_query_marks_.empty()) {
            return;
        }

        ++obstacle_query_stamp_;
        if (obstacle_query_stamp_ == 0) {
            std::fill(obstacle_query_marks_.begin(), obstacle_query_marks_.end(), 0);
            obstacle_query_stamp_ = 1;
        }

        const int min_cell_x = static_cast<int>(std::floor(min_x * obstacle_grid_inv_cell_size_));
        const int max_cell_x = static_cast<int>(std::floor(max_x * obstacle_grid_inv_cell_size_));
        const int min_cell_y = static_cast<int>(std::floor(min_y * obstacle_grid_inv_cell_size_));
        const int max_cell_y = static_cast<int>(std::floor(max_y * obstacle_grid_inv_cell_size_));

        for (int cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
            for (int cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
                const auto it = obstacle_grid_.find({cell_x, cell_y});
                if (it == obstacle_grid_.end()) {
                    continue;
                }

                for (const std::size_t obstacle_index : it->second) {
                    if (obstacle_query_marks_[obstacle_index] == obstacle_query_stamp_) {
                        continue;
                    }
                    obstacle_query_marks_[obstacle_index] = obstacle_query_stamp_;
                    obstacle_query_buffer_.push_back(obstacle_index);
                }
            }
        }
    }

    bool VisibilityGraphBuilder::point_is_inside_other_obstacle(const geometry::Point& point,
                                                                const geometry::Scene& scene,
                                                                const std::size_t self_obstacle_index) const {
        if (scene.obstacles.empty()) {
            return false;
        }

        collect_obstacle_candidates_in_aabb(point.x - max_obstacle_radius_,
                                            point.y - max_obstacle_radius_,
                                            point.x + max_obstacle_radius_,
                                            point.y + max_obstacle_radius_);

        for (const std::size_t obstacle_index : obstacle_query_buffer_) {
            if (obstacle_index == self_obstacle_index) {
                continue;
            }

            const auto& obstacle = scene.obstacles[obstacle_index];
            if (point.distance(obstacle.center) < obstacle.radius - kCollisionEpsilon) {
                return true;
            }
        }

        return false;
    }

    std::vector<std::pair<geometry::Point, geometry::Point>>
    VisibilityGraphBuilder::circle_circle_tangents(const geometry::Disk& first,
                                                   const geometry::Disk& second) {
        std::vector<std::pair<geometry::Point, geometry::Point>> tangents;
        const double dx = second.center.x - first.center.x;
        const double dy = second.center.y - first.center.y;
        const double center_distance_sq = dx * dx + dy * dy;
        if (center_distance_sq <= 1e-12) {
            return tangents;
        }

        for (int sign = -1; sign <= 1; sign += 2) {
            const double radius_delta = first.radius - static_cast<double>(sign) * second.radius;
            const double tangent_length_sq = center_distance_sq - radius_delta * radius_delta;
            if (tangent_length_sq < -1e-9) {
                continue;
            }

            const double tangent_length = tangent_length_sq <= 0.0 ? 0.0 : std::sqrt(tangent_length_sq);
            for (int orientation = -1; orientation <= 1; orientation += 2) {
                const double nx = (
                    dx * radius_delta - dy * tangent_length * static_cast<double>(orientation)
                ) / center_distance_sq;
                const double ny = (
                    dy * radius_delta + dx * tangent_length * static_cast<double>(orientation)
                ) / center_distance_sq;

                tangents.emplace_back(
                    geometry::Point{
                        first.center.x + first.radius * nx,
                        first.center.y + first.radius * ny
                    },
                    geometry::Point{
                        second.center.x + static_cast<double>(sign) * second.radius * nx,
                        second.center.y + static_cast<double>(sign) * second.radius * ny
                    });
            }
        }

        return tangents;
    }

    std::vector<geometry::Point> VisibilityGraphBuilder::point_circle_tangency_points(
        const geometry::Point& point,
        const geometry::Disk& disk) {
        std::vector<geometry::Point> tangency_points;
        const double dx = point.x - disk.center.x;
        const double dy = point.y - disk.center.y;
        const double dist_sq = dx * dx + dy * dy;
        const double radius_sq = disk.radius * disk.radius;
        if (dist_sq <= radius_sq + 1e-12) {
            return tangency_points;
        }

        const double projection_scale = radius_sq / dist_sq;
        const double offset_scale = disk.radius * std::sqrt(dist_sq - radius_sq) / dist_sq;
        tangency_points.push_back({
            disk.center.x + projection_scale * dx - offset_scale * dy,
            disk.center.y + projection_scale * dy + offset_scale * dx
        });
        tangency_points.push_back({
            disk.center.x + projection_scale * dx + offset_scale * dy,
            disk.center.y + projection_scale * dy - offset_scale * dx
        });
        return tangency_points;
    }

    VisibilityGraphBuilder::DirectedEdgeKey VisibilityGraphBuilder::make_directed_edge_key(
        const std::size_t from,
        const std::size_t to) {
        return {from, to};
    }

    std::size_t VisibilityGraphBuilder::DirectedEdgeKeyHash::operator()(
        const DirectedEdgeKey& key) const noexcept {
        const std::size_t from_hash = std::hash<std::size_t>{}(key.from);
        const std::size_t to_hash = std::hash<std::size_t>{}(key.to);
        return from_hash ^ (to_hash + 0x9e3779b97f4a7c15ULL + (from_hash << 6) + (from_hash >> 2));
    }

    std::size_t VisibilityGraphBuilder::NodeBucketKeyHash::operator()(
        const NodeBucketKey& key) const noexcept {
        const std::size_t x_hash = std::hash<std::int64_t>{}(key.x);
        const std::size_t y_hash = std::hash<std::int64_t>{}(key.y);
        return x_hash ^ (y_hash + 0x9e3779b97f4a7c15ULL + (x_hash << 6) + (x_hash >> 2));
    }

    std::size_t VisibilityGraphBuilder::ObstacleCellKeyHash::operator()(
        const ObstacleCellKey& key) const noexcept {
        const std::size_t x_hash = std::hash<int>{}(key.x);
        const std::size_t y_hash = std::hash<int>{}(key.y);
        return x_hash ^ (y_hash + 0x9e3779b97f4a7c15ULL + (x_hash << 6) + (x_hash >> 2));
    }

    double VisibilityGraphBuilder::normalize_angle(double angle) {
        while (angle < 0.0) {
            angle += 2.0 * kPi;
        }
        while (angle >= 2.0 * kPi) {
            angle -= 2.0 * kPi;
        }
        return angle;
    }

    double VisibilityGraphBuilder::positive_angle_delta(const double from, const double to) {
        double delta = normalize_angle(to) - normalize_angle(from);
        if (delta < 0.0) {
            delta += 2.0 * kPi;
        }
        return delta;
    }

    VisibilityGraphBuilder::NodeBucketKey VisibilityGraphBuilder::make_node_bucket_key(
        const geometry::Point& point) {
        return {
            static_cast<std::int64_t>(std::llround(point.x / kPointEpsilon)),
            static_cast<std::int64_t>(std::llround(point.y / kPointEpsilon))
        };
    }

    double VisibilityGraphBuilder::calculate_distance(const geometry::Point& a,
                                                      const geometry::Point& b) const {
        return a.distance(b);
    }

} // namespace algorithms::graph
