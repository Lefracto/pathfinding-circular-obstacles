//
// Implementation of GridGraphBuilder
//

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
    } // namespace

    GridGraphBuilder::GridGraphBuilder(double grid_step, bool allow_diagonal)
        : grid_step_(grid_step), allow_diagonal_(allow_diagonal) {
        if (grid_step <= 0) {
            throw std::invalid_argument("Grid step must be positive");
        }
    }

    Graph GridGraphBuilder::build(const geometry::Scene& scene) {
        node_to_point_.clear();
        point_to_node_.clear();
        start_node_id_ = std::nullopt;
        goal_node_id_ = std::nullopt;

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
        // Center of the grid cell
        double x = (grid_x + 0.5) * grid_step_;
        double y = (grid_y + 0.5) * grid_step_;
        return {x, y};
    }

    std::size_t GridGraphBuilder::make_grid_key(int grid_x, int grid_y, int grid_width) {
        return static_cast<std::size_t>(grid_y) * static_cast<std::size_t>(grid_width) +
               static_cast<std::size_t>(grid_x);
    }

} // namespace algorithms::graph
