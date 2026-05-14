#include "../../include/algorithms/GridGraphBuilder.h"
#include "../../include/geometry/Disk.h"
#include "../../include/geometry/Point.h"
#include <cmath>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <limits>

namespace algorithms::graph {

    namespace {
        constexpr double kEpsilon = 1e-9;
        constexpr std::size_t kNoNode = std::numeric_limits<std::size_t>::max();

        double distance_sq_point_to_segment(const geometry::Point& point,
                                            const geometry::Point& segment_start,
                                            const geometry::Point& segment_end) {
            const double vx = segment_end.x - segment_start.x;
            const double vy = segment_end.y - segment_start.y;
            const double segment_len_sq = vx * vx + vy * vy;
            if (segment_len_sq <= kEpsilon) {
                const double dx = point.x - segment_start.x;
                const double dy = point.y - segment_start.y;
                return dx * dx + dy * dy;
            }

            const double wx = point.x - segment_start.x;
            const double wy = point.y - segment_start.y;
            const double t_unclamped = (wx * vx + wy * vy) / segment_len_sq;
            const double t = std::max(0.0, std::min(1.0, t_unclamped));

            const double closest_x = segment_start.x + t * vx;
            const double closest_y = segment_start.y + t * vy;
            const double dx = point.x - closest_x;
            const double dy = point.y - closest_y;
            return dx * dx + dy * dy;
        }

        double estimate_obstacle_grid_cell_size(const geometry::Scene& scene, const double grid_step) {
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
            return std::max({1.0, grid_step * 2.0, avg_radius * 2.0, area_based * 0.5});
        }
    } // namespace

    GridGraphBuilder::GridGraphBuilder(double grid_step,
                                       bool allow_diagonal,
                                       bool use_obstacle_spatial_index)
        : grid_step_(grid_step),
          allow_diagonal_(allow_diagonal),
          use_obstacle_spatial_index_(use_obstacle_spatial_index) {
        if (grid_step <= 0) {
            throw std::invalid_argument("Grid step must be positive");
        }
    }

    Graph GridGraphBuilder::build(const geometry::Scene& scene) {
        if (use_obstacle_spatial_index_) {
            return build_with_spatial_index(scene);
        }

        node_to_point_.clear();
        point_to_node_.clear();
        start_node_id_ = std::nullopt;
        goal_node_id_ = std::nullopt;
        clear_obstacle_spatial_index();

        const int grid_width = std::max(1, static_cast<int>(std::ceil(scene.width / grid_step_)));
        const int grid_height = std::max(1, static_cast<int>(std::ceil(scene.height / grid_step_)));

        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                const geometry::Point cell_center = grid_to_point(gx, gy);
                if (is_point_in_bounds(cell_center, scene) &&
                    !is_point_in_obstacle(cell_center, scene)) {
                    const std::size_t node_id = node_to_point_.size();
                    node_to_point_.push_back(cell_center);
                    const std::size_t grid_key = make_grid_key(gx, gy, grid_width);
                    point_to_node_[grid_key] = node_id;
                }
            }
        }

        const std::size_t regular_node_count = node_to_point_.size();

        auto maybe_add_special_node = [this, &scene](const geometry::Point& point,
                                                      std::optional<std::size_t>& target_node) {
            if (!is_point_in_bounds(point, scene) || is_point_in_obstacle(point, scene)) {
                target_node = std::nullopt;
                return;
            }

            for (std::size_t i = 0; i < node_to_point_.size(); ++i) {
                if (node_to_point_[i].distance(point) <= 1e-6) {
                    target_node = i;
                    return;
                }
            }

            target_node = node_to_point_.size();
            node_to_point_.push_back(point);
        };

        maybe_add_special_node(scene.start, start_node_id_);
        maybe_add_special_node(scene.goal, goal_node_id_);

        Graph graph;
        graph.adj.resize(node_to_point_.size());

        constexpr std::array<std::pair<int, int>, 8> kNeighborOffsets = {
            std::pair{0, -1}, std::pair{1, 0}, std::pair{0, 1}, std::pair{-1, 0},
            std::pair{-1, -1}, std::pair{1, -1}, std::pair{1, 1}, std::pair{-1, 1}
        };
        const std::size_t neighbor_count = allow_diagonal_ ? 8 : 4;

        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                const std::size_t grid_key = make_grid_key(gx, gy, grid_width);
                const auto it = point_to_node_.find(grid_key);
                if (it == point_to_node_.end()) {
                    continue;
                }

                const std::size_t from_node = it->second;
                const geometry::Point& from_point = node_to_point_[from_node];

                for (std::size_t dir = 0; dir < neighbor_count; ++dir) {
                    const int nx = gx + kNeighborOffsets[dir].first;
                    const int ny = gy + kNeighborOffsets[dir].second;
                    if (nx < 0 || nx >= grid_width || ny < 0 || ny >= grid_height) {
                        continue;
                    }

                    const std::size_t neighbor_key = make_grid_key(nx, ny, grid_width);
                    const auto neighbor_it = point_to_node_.find(neighbor_key);
                    if (neighbor_it == point_to_node_.end()) {
                        continue;
                    }

                    const std::size_t to_node = neighbor_it->second;
                    const geometry::Point& to_point = node_to_point_[to_node];
                    if (!is_segment_collision_free(from_point, to_point, scene)) {
                        continue;
                    }

                    graph.adj[from_node].push_back({to_node, calculate_distance(from_point, to_point)});
                }
            }
        }

        const double attach_radius = allow_diagonal_
                                         ? (grid_step_ * std::sqrt(5.0))
                                         : (grid_step_ * 1.75);
        const double attach_radius_sq = attach_radius * attach_radius;
        auto connect_special_to_regular = [&](const std::optional<std::size_t> special_node_id) {
            if (!special_node_id.has_value()) {
                return;
            }

            const std::size_t special_id = special_node_id.value();
            const geometry::Point& special_point = node_to_point_[special_id];

            for (std::size_t node_id = 0; node_id < regular_node_count; ++node_id) {
                if (node_id == special_id) {
                    continue;
                }
                const geometry::Point& node_point = node_to_point_[node_id];
                const double dx = node_point.x - special_point.x;
                const double dy = node_point.y - special_point.y;
                const double dist_sq = dx * dx + dy * dy;
                if (dist_sq > attach_radius_sq) {
                    continue;
                }
                if (!is_segment_collision_free(special_point, node_point, scene)) {
                    continue;
                }

                const double weight = std::sqrt(dist_sq);
                graph.adj[special_id].push_back({node_id, weight});
                graph.adj[node_id].push_back({special_id, weight});
            }
        };

        connect_special_to_regular(start_node_id_);
        connect_special_to_regular(goal_node_id_);

        if (start_node_id_.has_value() && goal_node_id_.has_value()) {
            const std::size_t start_node = start_node_id_.value();
            const std::size_t goal_node = goal_node_id_.value();
            if (start_node == goal_node) {
                return graph;
            }
            const geometry::Point& start_point = node_to_point_[start_node];
            const geometry::Point& goal_point = node_to_point_[goal_node];

            if (is_segment_collision_free(start_point, goal_point, scene)) {
                const double weight = calculate_distance(start_point, goal_point);
                graph.adj[start_node].push_back({goal_node, weight});
                graph.adj[goal_node].push_back({start_node, weight});
            }
        }

        return graph;
    }

    std::string GridGraphBuilder::name() const {
        return "GridGraphBuilder";
    }

    geometry::Point GridGraphBuilder::get_node_point(std::size_t node_id) const {
        if (node_id >= node_to_point_.size()) {
            throw std::out_of_range("Node ID out of range");
        }
        return node_to_point_[node_id];
    }

    std::optional<std::size_t> GridGraphBuilder::get_node_id(const geometry::Point& point) const {
        constexpr double exact_match_epsilon = 1e-6;
        if (start_node_id_.has_value() &&
            node_to_point_[start_node_id_.value()].distance(point) <= exact_match_epsilon) {
            return start_node_id_;
        }
        if (goal_node_id_.has_value() &&
            node_to_point_[goal_node_id_.value()].distance(point) <= exact_match_epsilon) {
            return goal_node_id_;
        }

        for (std::size_t i = 0; i < node_to_point_.size(); ++i) {
            if (node_to_point_[i].distance(point) <= exact_match_epsilon) {
                return i;
            }
        }

        double min_distance = std::numeric_limits<double>::max();
        std::size_t closest_node = 0;
        bool found = false;

        for (std::size_t i = 0; i < node_to_point_.size(); ++i) {
            const double dist = node_to_point_[i].distance(point);
            if (dist < min_distance) {
                min_distance = dist;
                closest_node = i;
                found = true;
            }
        }

        const double max_match_distance = allow_diagonal_ ? (grid_step_ * 0.8) : (grid_step_ * 0.6);
        if (found && min_distance <= max_match_distance) {
            return closest_node;
        }
        return std::nullopt;
    }

    bool GridGraphBuilder::is_point_in_obstacle(const geometry::Point& point, const geometry::Scene& scene) const {
        if (use_obstacle_spatial_index_) {
            collect_obstacle_candidates_in_aabb(point.x, point.y, point.x, point.y);
            for (const std::size_t obstacle_index : obstacle_query_buffer_) {
                if (scene.obstacles[obstacle_index].contains(point)) {
                    return true;
                }
            }
            return false;
        }

        for (const auto& obstacle : scene.obstacles) {
            if (obstacle.contains(point)) {
                return true;
            }
        }
        return false;
    }

    bool GridGraphBuilder::is_point_in_bounds(const geometry::Point& point, const geometry::Scene& scene) const {
        return point.x >= 0.0 && point.x <= scene.width &&
               point.y >= 0.0 && point.y <= scene.height;
    }

    bool GridGraphBuilder::is_segment_collision_free(const geometry::Point& a,
                                                     const geometry::Point& b,
                                                     const geometry::Scene& scene) const {
        if (use_obstacle_spatial_index_) {
            const double min_x = std::min(a.x, b.x);
            const double min_y = std::min(a.y, b.y);
            const double max_x = std::max(a.x, b.x);
            const double max_y = std::max(a.y, b.y);
            collect_obstacle_candidates_in_aabb(min_x, min_y, max_x, max_y);

            for (const std::size_t obstacle_index : obstacle_query_buffer_) {
                const auto& obstacle = scene.obstacles[obstacle_index];
                if (obstacle.radius <= 0.0) {
                    continue;
                }

                const double radius_sq = obstacle.radius * obstacle.radius;
                const double dist_sq = distance_sq_point_to_segment(obstacle.center, a, b);
                if (dist_sq < radius_sq - 1e-9) {
                    return false;
                }
            }
            return true;
        }

        for (const auto& obstacle : scene.obstacles) {
            if (obstacle.radius <= 0.0) {
                continue;
            }

            const double radius_sq = obstacle.radius * obstacle.radius;
            const double dist_sq = distance_sq_point_to_segment(obstacle.center, a, b);
            if (dist_sq < radius_sq - 1e-9) {
                return false;
            }
        }
        return true;
    }

    double GridGraphBuilder::calculate_distance(const geometry::Point& a, const geometry::Point& b) const {
        return a.distance(b);
    }

    std::pair<int, int> GridGraphBuilder::point_to_grid(const geometry::Point& point) const {
        int grid_x = static_cast<int>(std::floor(point.x / grid_step_));
        int grid_y = static_cast<int>(std::floor(point.y / grid_step_));
        return {grid_x, grid_y};
    }

    geometry::Point GridGraphBuilder::grid_to_point(int grid_x, int grid_y) const {
        double x = (grid_x + 0.5) * grid_step_;
        double y = (grid_y + 0.5) * grid_step_;
        return {x, y};
    }

    std::size_t GridGraphBuilder::make_grid_key(int grid_x, int grid_y, int grid_width) {
        return static_cast<std::size_t>(grid_y) * static_cast<std::size_t>(grid_width) +
               static_cast<std::size_t>(grid_x);
    }

    void GridGraphBuilder::build_obstacle_spatial_index(const geometry::Scene& scene) {
        clear_obstacle_spatial_index();
        obstacle_query_marks_.assign(scene.obstacles.size(), 0);
        obstacle_query_stamp_ = 1;

        if (scene.obstacles.empty()) {
            return;
        }

        obstacle_grid_cell_size_ = estimate_obstacle_grid_cell_size(scene, grid_step_);
        obstacle_grid_inv_cell_size_ = 1.0 / obstacle_grid_cell_size_;
        obstacle_grid_.reserve(scene.obstacles.size() * 4);

        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];
            max_obstacle_radius_ = std::max(max_obstacle_radius_, std::max(0.0, obstacle.radius));

            const int min_cell_x =
                static_cast<int>(std::floor((obstacle.center.x - obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int max_cell_x =
                static_cast<int>(std::floor((obstacle.center.x + obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int min_cell_y =
                static_cast<int>(std::floor((obstacle.center.y - obstacle.radius) * obstacle_grid_inv_cell_size_));
            const int max_cell_y =
                static_cast<int>(std::floor((obstacle.center.y + obstacle.radius) * obstacle_grid_inv_cell_size_));

            for (int cell_y = min_cell_y; cell_y <= max_cell_y; ++cell_y) {
                for (int cell_x = min_cell_x; cell_x <= max_cell_x; ++cell_x) {
                    obstacle_grid_[{cell_x, cell_y}].push_back(obstacle_index);
                }
            }
        }
    }

    void GridGraphBuilder::clear_obstacle_spatial_index() {
        obstacle_grid_.clear();
        obstacle_query_buffer_.clear();
        obstacle_query_marks_.clear();
        obstacle_query_stamp_ = 1;
        obstacle_grid_cell_size_ = 1.0;
        obstacle_grid_inv_cell_size_ = 1.0;
        max_obstacle_radius_ = 0.0;
    }

    Graph GridGraphBuilder::build_with_spatial_index(const geometry::Scene& scene) {
        node_to_point_.clear();
        point_to_node_.clear();
        start_node_id_ = std::nullopt;
        goal_node_id_ = std::nullopt;
        build_obstacle_spatial_index(scene);

        const int grid_width = std::max(1, static_cast<int>(std::ceil(scene.width / grid_step_)));
        const int grid_height = std::max(1, static_cast<int>(std::ceil(scene.height / grid_step_)));
        const std::size_t total_grid_cells =
            static_cast<std::size_t>(grid_width) * static_cast<std::size_t>(grid_height);
        std::vector<std::size_t> cell_to_node(total_grid_cells, kNoNode);

        node_to_point_.reserve(total_grid_cells + 2);
        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                const geometry::Point cell_center = grid_to_point(gx, gy);
                if (!is_point_in_bounds(cell_center, scene) || is_point_in_obstacle(cell_center, scene)) {
                    continue;
                }

                const std::size_t node_id = node_to_point_.size();
                node_to_point_.push_back(cell_center);
                cell_to_node[make_grid_key(gx, gy, grid_width)] = node_id;
            }
        }

        const std::size_t regular_node_count = node_to_point_.size();

        auto maybe_add_special_node = [&](const geometry::Point& point,
                                          std::optional<std::size_t>& target_node) {
            if (!is_point_in_bounds(point, scene) || is_point_in_obstacle(point, scene)) {
                target_node = std::nullopt;
                return;
            }

            const auto [grid_x, grid_y] = point_to_grid(point);
            if (grid_x >= 0 && grid_x < grid_width && grid_y >= 0 && grid_y < grid_height) {
                const std::size_t cell_key = make_grid_key(grid_x, grid_y, grid_width);
                const std::size_t node_id = cell_to_node[cell_key];
                if (node_id != kNoNode && node_to_point_[node_id].distance(point) <= 1e-6) {
                    target_node = node_id;
                    return;
                }
            }

            for (std::size_t i = 0; i < node_to_point_.size(); ++i) {
                if (node_to_point_[i].distance(point) <= 1e-6) {
                    target_node = i;
                    return;
                }
            }

            target_node = node_to_point_.size();
            node_to_point_.push_back(point);
        };

        maybe_add_special_node(scene.start, start_node_id_);
        maybe_add_special_node(scene.goal, goal_node_id_);

        Graph graph;
        graph.adj.resize(node_to_point_.size());
        const std::size_t base_degree = allow_diagonal_ ? 8 : 4;
        for (auto& adjacency : graph.adj) {
            adjacency.reserve(base_degree + 2);
        }

        const std::array<std::pair<int, int>, 4> undirected_neighbor_offsets = {
            std::pair{1, 0},
            std::pair{0, 1},
            std::pair{1, 1},
            std::pair{-1, 1},
        };
        const std::size_t neighbor_count = allow_diagonal_ ? 4 : 2;

        const double straight_weight = grid_step_;
        const double diagonal_weight = grid_step_ * std::sqrt(2.0);

        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                const std::size_t from_node = cell_to_node[make_grid_key(gx, gy, grid_width)];
                if (from_node == kNoNode) {
                    continue;
                }

                const geometry::Point& from_point = node_to_point_[from_node];
                for (std::size_t dir = 0; dir < neighbor_count; ++dir) {
                    const int nx = gx + undirected_neighbor_offsets[dir].first;
                    const int ny = gy + undirected_neighbor_offsets[dir].second;
                    if (nx < 0 || nx >= grid_width || ny < 0 || ny >= grid_height) {
                        continue;
                    }

                    const std::size_t to_node = cell_to_node[make_grid_key(nx, ny, grid_width)];
                    if (to_node == kNoNode) {
                        continue;
                    }

                    const geometry::Point& to_point = node_to_point_[to_node];
                    if (!is_segment_collision_free(from_point, to_point, scene)) {
                        continue;
                    }

                    const bool diagonal_edge =
                        undirected_neighbor_offsets[dir].first != 0 &&
                        undirected_neighbor_offsets[dir].second != 0;
                    const double weight = diagonal_edge ? diagonal_weight : straight_weight;
                    graph.adj[from_node].push_back({to_node, weight});
                    graph.adj[to_node].push_back({from_node, weight});
                }
            }
        }

        const double attach_radius = allow_diagonal_
                                         ? (grid_step_ * std::sqrt(5.0))
                                         : (grid_step_ * 1.75);
        const double attach_radius_sq = attach_radius * attach_radius;
        auto connect_special_to_regular = [&](const std::optional<std::size_t> special_node_id) {
            if (!special_node_id.has_value()) {
                return;
            }

            const std::size_t special_id = special_node_id.value();
            const geometry::Point& special_point = node_to_point_[special_id];
            const int min_grid_x = std::max(0, static_cast<int>(std::floor((special_point.x - attach_radius) / grid_step_)));
            const int max_grid_x =
                std::min(grid_width - 1, static_cast<int>(std::floor((special_point.x + attach_radius) / grid_step_)));
            const int min_grid_y = std::max(0, static_cast<int>(std::floor((special_point.y - attach_radius) / grid_step_)));
            const int max_grid_y =
                std::min(grid_height - 1, static_cast<int>(std::floor((special_point.y + attach_radius) / grid_step_)));

            for (int gy = min_grid_y; gy <= max_grid_y; ++gy) {
                for (int gx = min_grid_x; gx <= max_grid_x; ++gx) {
                    const std::size_t node_id = cell_to_node[make_grid_key(gx, gy, grid_width)];
                    if (node_id == kNoNode || node_id >= regular_node_count || node_id == special_id) {
                        continue;
                    }

                    const geometry::Point& node_point = node_to_point_[node_id];
                    const double dx = node_point.x - special_point.x;
                    const double dy = node_point.y - special_point.y;
                    const double dist_sq = dx * dx + dy * dy;
                    if (dist_sq > attach_radius_sq) {
                        continue;
                    }
                    if (!is_segment_collision_free(special_point, node_point, scene)) {
                        continue;
                    }

                    const double weight = std::sqrt(dist_sq);
                    graph.adj[special_id].push_back({node_id, weight});
                    graph.adj[node_id].push_back({special_id, weight});
                }
            }
        };

        connect_special_to_regular(start_node_id_);
        connect_special_to_regular(goal_node_id_);

        if (start_node_id_.has_value() && goal_node_id_.has_value()) {
            const std::size_t start_node = start_node_id_.value();
            const std::size_t goal_node = goal_node_id_.value();
            if (start_node == goal_node) {
                return graph;
            }

            const geometry::Point& start_point = node_to_point_[start_node];
            const geometry::Point& goal_point = node_to_point_[goal_node];
            if (is_segment_collision_free(start_point, goal_point, scene)) {
                const double weight = calculate_distance(start_point, goal_point);
                graph.adj[start_node].push_back({goal_node, weight});
                graph.adj[goal_node].push_back({start_node, weight});
            }
        }

        return graph;
    }

    void GridGraphBuilder::collect_obstacle_candidates_in_aabb(
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

    std::size_t GridGraphBuilder::ObstacleCellKeyHash::operator()(
        const ObstacleCellKey& key) const noexcept {
        const std::size_t x_hash = std::hash<int>{}(key.x);
        const std::size_t y_hash = std::hash<int>{}(key.y);
        return x_hash ^ (y_hash + 0x9e3779b97f4a7c15ULL + (x_hash << 6) + (x_hash >> 2));
    }

}
