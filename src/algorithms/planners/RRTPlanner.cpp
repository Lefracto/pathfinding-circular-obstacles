#include "algorithms/RRTPlanner.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <random>
#include <utility>
#include <vector>

namespace algorithms {

    namespace {

        constexpr double kCollisionEpsilon = 1e-9;

        struct TreeNode {
            geometry::Point point;
            std::size_t parent = 0;
        };

        bool point_in_bounds(const geometry::Point& point, const geometry::Scene& scene) {
            return point.x >= 0.0 && point.x <= scene.width &&
                   point.y >= 0.0 && point.y <= scene.height;
        }

        bool point_in_obstacle(const geometry::Point& point, const geometry::Scene& scene) {
            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.contains(point)) {
                    return true;
                }
            }
            return false;
        }

        double distance_sq_point_to_segment(const geometry::Point& point,
                                            const geometry::Point& segment_start,
                                            const geometry::Point& segment_end) {
            const double vx = segment_end.x - segment_start.x;
            const double vy = segment_end.y - segment_start.y;
            const double segment_len_sq = vx * vx + vy * vy;
            if (segment_len_sq <= kCollisionEpsilon) {
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

        bool segment_collision_free(const geometry::Point& a,
                                    const geometry::Point& b,
                                    const geometry::Scene& scene) {
            if (!point_in_bounds(a, scene) || !point_in_bounds(b, scene)) {
                return false;
            }

            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.radius <= 0.0) {
                    continue;
                }
                const double radius_sq = obstacle.radius * obstacle.radius;
                const double dist_sq = distance_sq_point_to_segment(obstacle.center, a, b);
                if (dist_sq < radius_sq - kCollisionEpsilon) {
                    return false;
                }
            }
            return true;
        }

        geometry::Point steer_towards(const geometry::Point& from,
                                      const geometry::Point& to,
                                      const double step_size) {
            const double dx = to.x - from.x;
            const double dy = to.y - from.y;
            const double distance = std::hypot(dx, dy);
            if (distance <= step_size || distance <= 1e-12) {
                return to;
            }

            const double scale = step_size / distance;
            return {from.x + dx * scale, from.y + dy * scale};
        }

        std::size_t find_nearest_node(const std::vector<TreeNode>& tree, const geometry::Point& point) {
            std::size_t nearest_index = 0;
            double best_distance_sq = std::numeric_limits<double>::infinity();

            for (std::size_t i = 0; i < tree.size(); ++i) {
                const double dx = tree[i].point.x - point.x;
                const double dy = tree[i].point.y - point.y;
                const double distance_sq = dx * dx + dy * dy;
                if (distance_sq < best_distance_sq) {
                    best_distance_sq = distance_sq;
                    nearest_index = i;
                }
            }

            return nearest_index;
        }

        geometry::Path reconstruct_path(const std::vector<TreeNode>& tree, std::size_t goal_index) {
            geometry::Path path;
            std::vector<geometry::Point> reversed_points;
            reversed_points.reserve(tree.size());

            while (true) {
                reversed_points.push_back(tree[goal_index].point);
                if (tree[goal_index].parent == goal_index) {
                    break;
                }
                goal_index = tree[goal_index].parent;
            }

            path.points.assign(reversed_points.rbegin(), reversed_points.rend());
            return path;
        }

        geometry::Path smooth_path(const geometry::Path& path,
                                   const geometry::Scene& scene,
                                   const std::size_t smoothing_iterations,
                                   std::mt19937& rng) {
            if (path.points.size() < 3 || smoothing_iterations == 0) {
                return path;
            }

            geometry::Path smoothed = path;

            for (std::size_t iteration = 0; iteration < smoothing_iterations; ++iteration) {
                if (smoothed.points.size() < 3) {
                    break;
                }

                std::uniform_int_distribution<std::size_t> index_distribution(0, smoothed.points.size() - 1);
                std::size_t i = index_distribution(rng);
                std::size_t j = index_distribution(rng);
                if (i == j) {
                    continue;
                }
                if (i > j) {
                    std::swap(i, j);
                }
                if (j <= i + 1) {
                    continue;
                }

                if (!segment_collision_free(smoothed.points[i], smoothed.points[j], scene)) {
                    continue;
                }

                smoothed.points.erase(smoothed.points.begin() + static_cast<std::ptrdiff_t>(i + 1),
                                      smoothed.points.begin() + static_cast<std::ptrdiff_t>(j));
            }

            return smoothed;
        }
        double polyline_length(const geometry::Path& path) {
            if (path.points.size() < 2) {
                return 0.0;
            }

            double total = 0.0;
            for (std::size_t i = 0; i + 1 < path.points.size(); ++i) {
                total += path.points[i].distance(path.points[i + 1]);
            }
            return total;
        }

    } // namespace

    RRTPlanner::RRTPlanner() : RRTPlanner(Config{}) {}

    RRTPlanner::RRTPlanner(const Config& config) : config_(config) {
        if (config_.max_iterations == 0) {
            config_.max_iterations = 1;
        }
        if (config_.step_size <= 0.0) {
            config_.step_size = 1.0;
        }
        config_.goal_sample_rate = std::clamp(config_.goal_sample_rate, 0.0, 1.0);
        if (config_.goal_tolerance <= 0.0) {
            config_.goal_tolerance = config_.step_size;
        }
    }

    void RRTPlanner::set_seed(const std::uint32_t seed) {
        config_.seed = seed;
    }

    std::uint32_t RRTPlanner::seed() const {
        return config_.seed;
    }

    const RRTPlanner::Config& RRTPlanner::config() const {
        return config_;
    }

    PathResult RRTPlanner::find_path(const geometry::Scene& scene) {
        if (!(scene.width > 0.0) || !(scene.height > 0.0)) {
            return std::nullopt;
        }
        if (!point_in_bounds(scene.start, scene) || !point_in_bounds(scene.goal, scene)) {
            return std::nullopt;
        }
        if (point_in_obstacle(scene.start, scene) || point_in_obstacle(scene.goal, scene)) {
            return std::nullopt;
        }

        if (segment_collision_free(scene.start, scene.goal, scene)) {
            geometry::Path direct_path;
            direct_path.points = {scene.start, scene.goal};
            return direct_path;
        }

        std::mt19937 rng(config_.seed);
        std::uniform_real_distribution<double> x_distribution(0.0, scene.width);
        std::uniform_real_distribution<double> y_distribution(0.0, scene.height);
        std::bernoulli_distribution goal_pick_distribution(config_.goal_sample_rate);

        std::vector<TreeNode> tree;
        tree.reserve(config_.max_iterations + 2);
        tree.push_back({scene.start, 0});

        std::optional<std::size_t> goal_node_index = std::nullopt;
        for (std::size_t iteration = 0; iteration < config_.max_iterations; ++iteration) {
            geometry::Point sampled_point;
            if (goal_pick_distribution(rng)) {
                sampled_point = scene.goal;
            } else {
                bool sampled_free = false;
                for (int attempt = 0; attempt < 32; ++attempt) {
                    sampled_point = {x_distribution(rng), y_distribution(rng)};
                    if (!point_in_obstacle(sampled_point, scene)) {
                        sampled_free = true;
                        break;
                    }
                }
                if (!sampled_free) {
                    continue;
                }
            }

            const std::size_t nearest_index = find_nearest_node(tree, sampled_point);
            const geometry::Point candidate = steer_towards(
                tree[nearest_index].point,
                sampled_point,
                config_.step_size);

            if (!point_in_bounds(candidate, scene) || point_in_obstacle(candidate, scene)) {
                continue;
            }
            if (!segment_collision_free(tree[nearest_index].point, candidate, scene)) {
                continue;
            }

            const std::size_t new_index = tree.size();
            tree.push_back({candidate, nearest_index});

            if (candidate.distance(scene.goal) <= config_.goal_tolerance &&
                segment_collision_free(candidate, scene.goal, scene)) {
                goal_node_index = tree.size();
                tree.push_back({scene.goal, new_index});
                break;
            }
        }

        if (!goal_node_index.has_value()) {
            return std::nullopt;
        }

        geometry::Path result_path = reconstruct_path(tree, goal_node_index.value());
        if (config_.enable_path_smoothing) {
            result_path = smooth_path(result_path, scene, config_.smoothing_iterations, rng);
        }
        return result_path;
    }

    BenchmarkResult RRTPlanner::plan(const geometry::Scene& scene) {
        const auto started_at = std::chrono::steady_clock::now();
        PathResult path_result = find_path(scene);
        const auto finished_at = std::chrono::steady_clock::now();

        BenchmarkResult benchmark;
        benchmark.algorithm_name = name();
        benchmark.runtime_ms = std::chrono::duration<double, std::milli>(
            finished_at - started_at).count();
        if (path_result.has_value()) {
            benchmark.path = std::move(path_result.value());
            benchmark.exact_path_length = polyline_length(benchmark.path);
        }
        return benchmark;
    }

    std::string RRTPlanner::name() const {
        return "rrt";
    }

}
