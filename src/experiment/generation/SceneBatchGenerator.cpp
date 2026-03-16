#include "experiment/generation/SceneBatchGenerator.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace experiment {

    namespace {

        bool point_in_scene_bounds(const geometry::Point& point, const geometry::Scene& scene) {
            return point.x >= 0.0 && point.x <= scene.width && point.y >= 0.0 && point.y <= scene.height;
        }

        bool point_inside_obstacles(const geometry::Point& point, const geometry::Scene& scene) {
            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.contains(point)) {
                    return true;
                }
            }
            return false;
        }

        bool disk_intersects_scene(const geometry::Disk& disk, const geometry::Scene& scene) {
            const double closest_x = std::clamp(disk.center.x, 0.0, scene.width);
            const double closest_y = std::clamp(disk.center.y, 0.0, scene.height);
            const double dx = disk.center.x - closest_x;
            const double dy = disk.center.y - closest_y;
            const double distance_sq = dx * dx + dy * dy;
            return distance_sq <= disk.radius * disk.radius;
        }

        bool obstacles_overlap(const geometry::Disk& lhs, const geometry::Disk& rhs) {
            const double center_distance = lhs.center.distance(rhs.center);
            return center_distance < (lhs.radius + rhs.radius);
        }

        geometry::Point sample_endpoint_candidate(const EndpointConfig& endpoint,
                                                  const SceneBoundsConfig& scene,
                                                  std::mt19937& rng) {
            if (endpoint.mode == EndpointMode::Fixed) {
                return endpoint.fixed_point;
            }
            if (endpoint.distribution.has_value()) {
                return endpoint.distribution.value().sample(rng);
            }

            std::uniform_real_distribution<double> x_distribution(0.0, scene.width);
            std::uniform_real_distribution<double> y_distribution(0.0, scene.height);
            return {x_distribution(rng), y_distribution(rng)};
        }

        bool try_generate_start_goal(geometry::Scene& scene,
                                     const StartGoalConfig& config,
                                     std::mt19937& rng,
                                     std::string& error_message) {
            auto validate_fixed_endpoint = [&scene](const geometry::Point& point,
                                                    const char* endpoint_name,
                                                    std::string& error) -> bool {
                if (!point_in_scene_bounds(point, scene)) {
                    error = std::string(endpoint_name) + " is out of scene bounds";
                    return false;
                }
                if (point_inside_obstacles(point, scene)) {
                    error = std::string(endpoint_name) + " is inside an obstacle";
                    return false;
                }
                return true;
            };

            geometry::Point start_point;
            if (config.start.mode == EndpointMode::Fixed) {
                start_point = config.start.fixed_point;
                if (!validate_fixed_endpoint(start_point, "start", error_message)) {
                    return false;
                }
            } else {
                bool found = false;
                for (std::size_t attempt = 0; attempt < config.max_attempts; ++attempt) {
                    start_point = sample_endpoint_candidate(config.start, {scene.width, scene.height}, rng);
                    if (!point_in_scene_bounds(start_point, scene)) {
                        continue;
                    }
                    if (point_inside_obstacles(start_point, scene)) {
                        continue;
                    }
                    found = true;
                    break;
                }
                if (!found) {
                    error_message = "Failed to sample start point in free space";
                    return false;
                }
            }

            geometry::Point goal_point;
            if (config.goal.mode == EndpointMode::Fixed) {
                goal_point = config.goal.fixed_point;
                if (!validate_fixed_endpoint(goal_point, "goal", error_message)) {
                    return false;
                }
                if (goal_point.distance(start_point) < config.min_distance) {
                    error_message = "Fixed start/goal violate min_distance";
                    return false;
                }
            } else {
                bool found = false;
                for (std::size_t attempt = 0; attempt < config.max_attempts; ++attempt) {
                    goal_point = sample_endpoint_candidate(config.goal, {scene.width, scene.height}, rng);
                    if (!point_in_scene_bounds(goal_point, scene)) {
                        continue;
                    }
                    if (point_inside_obstacles(goal_point, scene)) {
                        continue;
                    }
                    if (goal_point.distance(start_point) < config.min_distance) {
                        continue;
                    }
                    found = true;
                    break;
                }
                if (!found) {
                    error_message = "Failed to sample goal point in free space with min_distance";
                    return false;
                }
            }

            scene.start = start_point;
            scene.goal = goal_point;
            return true;
        }

        SceneInstance generate_instance(const GeneratorConfig& config, const std::size_t index) {
            SceneInstance instance;
            instance.instance_id = "scene_" + std::to_string(index + 1);
            instance.seed = static_cast<std::uint32_t>(config.global_seed + static_cast<std::uint32_t>(index));
            instance.generation_status = SceneGenerationStatus::NotGenerated;
            instance.run_status = SceneRunStatus::NotRun;

            geometry::Scene scene({}, {}, config.scene.width, config.scene.height);
            std::mt19937 rng(instance.seed);

            const std::size_t target_obstacles = config.obstacles.count_distribution.sample(rng);
            const std::size_t max_attempts =
                std::max<std::size_t>(1, target_obstacles) * config.obstacles.max_attempts_per_obstacle;

            std::size_t attempts = 0;
            std::size_t next_id = 0;
            while (scene.obstacles.size() < target_obstacles && attempts < max_attempts) {
                ++attempts;
                const double radius = config.obstacles.radius_distribution.sample(rng);
                if (radius <= 0.0) {
                    continue;
                }

                const geometry::Point center = config.obstacles.center_distribution.sample(rng);
                const geometry::Disk candidate(center, radius, next_id);

                if (config.obstacles.require_intersection_with_scene && !disk_intersects_scene(candidate, scene)) {
                    continue;
                }

                if (!config.obstacles.allow_overlap) {
                    bool has_overlap = false;
                    for (const auto& existing : scene.obstacles) {
                        if (obstacles_overlap(candidate, existing)) {
                            has_overlap = true;
                            break;
                        }
                    }
                    if (has_overlap) {
                        continue;
                    }
                }

                scene.add_obstacle(candidate);
                ++next_id;
            }

            if (scene.obstacles.size() < target_obstacles) {
                std::ostringstream message;
                message << "Generated " << scene.obstacles.size() << " of " << target_obstacles
                        << " obstacles after " << attempts << " attempts";
                instance.scene_data = scene;
                instance.error_message = message.str();
                instance.generation_status = SceneGenerationStatus::Failed;
                instance.run_status = SceneRunStatus::Failed;
                return instance;
            }

            std::string endpoint_error;
            if (!try_generate_start_goal(scene, config.start_goal, rng, endpoint_error)) {
                instance.scene_data = scene;
                instance.error_message = endpoint_error;
                instance.generation_status = SceneGenerationStatus::Failed;
                instance.run_status = SceneRunStatus::Failed;
                return instance;
            }

            instance.scene_data = scene;
            instance.generation_status = SceneGenerationStatus::Generated;
            instance.run_status = SceneRunStatus::Ready;
            return instance;
        }

    } // namespace

    std::vector<SceneInstance> SceneBatchGenerator::generate_instances(const GeneratorConfig& config,
                                                                       std::atomic<std::size_t>* completed_counter) {
        std::vector<SceneInstance> instances;
        instances.reserve(config.instance_count);

        if (completed_counter != nullptr) {
            completed_counter->store(0);
        }

        for (std::size_t i = 0; i < config.instance_count; ++i) {
            instances.push_back(generate_instance(config, i));
            if (completed_counter != nullptr) {
                completed_counter->fetch_add(1);
            }
        }

        return instances;
    }

} // namespace experiment


