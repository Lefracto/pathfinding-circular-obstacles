#include "experiment/metrics/SceneMetricsCalculator.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <vector>

namespace experiment {

    namespace {

        constexpr double kPi = 3.14159265358979323846;

        bool point_inside_obstacles(const geometry::Point& point, const geometry::Scene& scene) {
            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.contains(point)) {
                    return true;
                }
            }
            return false;
        }

        geometry::Point sample_point_in_scene(std::mt19937& rng, const geometry::Scene& scene) {
            std::uniform_real_distribution<double> x_distribution(0.0, scene.width);
            std::uniform_real_distribution<double> y_distribution(0.0, scene.height);
            return {x_distribution(rng), y_distribution(rng)};
        }

        double mean(const std::vector<double>& values) {
            if (values.empty()) {
                return 0.0;
            }
            const double sum = std::accumulate(values.begin(), values.end(), 0.0);
            return sum / static_cast<double>(values.size());
        }

        double stddev(const std::vector<double>& values, const double values_mean) {
            if (values.empty()) {
                return 0.0;
            }

            double variance = 0.0;
            for (const double value : values) {
                const double delta = value - values_mean;
                variance += delta * delta;
            }
            variance /= static_cast<double>(values.size());
            return std::sqrt(variance);
        }

        double compute_obstacle_area_ratio(const geometry::Scene& scene,
                                           const std::size_t sample_count,
                                           std::mt19937& rng) {
            if (sample_count == 0 || scene.width <= 0.0 || scene.height <= 0.0) {
                return 0.0;
            }

            std::size_t occupied_samples = 0;
            for (std::size_t i = 0; i < sample_count; ++i) {
                if (point_inside_obstacles(sample_point_in_scene(rng, scene), scene)) {
                    ++occupied_samples;
                }
            }

            return static_cast<double>(occupied_samples) / static_cast<double>(sample_count);
        }

        double compute_obstacle_area_ratio_grid(const geometry::Scene& scene, const std::size_t grid_resolution) {
            if (grid_resolution == 0 || scene.width <= 0.0 || scene.height <= 0.0) {
                return 0.0;
            }

            std::size_t occupied_samples = 0;
            const std::size_t total_samples = grid_resolution * grid_resolution;

            for (std::size_t gy = 0; gy < grid_resolution; ++gy) {
                for (std::size_t gx = 0; gx < grid_resolution; ++gx) {
                    const geometry::Point point{
                        (static_cast<double>(gx) + 0.5) * scene.width / static_cast<double>(grid_resolution),
                        (static_cast<double>(gy) + 0.5) * scene.height / static_cast<double>(grid_resolution)};
                    if (point_inside_obstacles(point, scene)) {
                        ++occupied_samples;
                    }
                }
            }

            return static_cast<double>(occupied_samples) / static_cast<double>(total_samples);
        }

        bool point_inside_scene(const geometry::Point& point, const geometry::Scene& scene) {
            return point.x >= 0.0 && point.x <= scene.width && point.y >= 0.0 && point.y <= scene.height;
        }

        double compute_boundary_density(const geometry::Scene& scene, const std::size_t angle_samples) {
            if (scene.width <= 0.0 || scene.height <= 0.0 || scene.obstacles.empty()) {
                return 0.0;
            }

            const std::size_t safe_samples = std::max<std::size_t>(angle_samples, 8);
            double total_boundary_length = 0.0;

            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.radius <= 0.0) {
                    continue;
                }

                std::size_t inside_count = 0;
                for (std::size_t i = 0; i < safe_samples; ++i) {
                    const double angle = 2.0 * kPi * static_cast<double>(i) / static_cast<double>(safe_samples);
                    const geometry::Point boundary_point{
                        obstacle.center.x + obstacle.radius * std::cos(angle),
                        obstacle.center.y + obstacle.radius * std::sin(angle)};
                    if (point_inside_scene(boundary_point, scene)) {
                        ++inside_count;
                    }
                }

                const double inside_fraction =
                    static_cast<double>(inside_count) / static_cast<double>(safe_samples);
                total_boundary_length += 2.0 * kPi * obstacle.radius * inside_fraction;
            }

            const double scene_area = scene.width * scene.height;
            return scene_area > 0.0 ? total_boundary_length / scene_area : 0.0;
        }

        double compute_cell_density_cv(const geometry::Scene& scene,
                                       const std::size_t grid_x,
                                       const std::size_t grid_y,
                                       const std::size_t sample_count,
                                       std::mt19937& rng) {
            if (scene.width <= 0.0 || scene.height <= 0.0 || grid_x == 0 || grid_y == 0 || sample_count == 0) {
                return 0.0;
            }

            const std::size_t cell_count = grid_x * grid_y;
            std::vector<std::size_t> total_by_cell(cell_count, 0);
            std::vector<std::size_t> occupied_by_cell(cell_count, 0);

            const double cell_width = scene.width / static_cast<double>(grid_x);
            const double cell_height = scene.height / static_cast<double>(grid_y);

            for (std::size_t i = 0; i < sample_count; ++i) {
                const geometry::Point point = sample_point_in_scene(rng, scene);

                const std::size_t ix =
                    std::min(grid_x - 1, static_cast<std::size_t>(point.x / std::max(cell_width, 1e-12)));
                const std::size_t iy =
                    std::min(grid_y - 1, static_cast<std::size_t>(point.y / std::max(cell_height, 1e-12)));

                const std::size_t linear_index = iy * grid_x + ix;
                ++total_by_cell[linear_index];
                if (point_inside_obstacles(point, scene)) {
                    ++occupied_by_cell[linear_index];
                }
            }

            std::vector<double> local_densities;
            local_densities.reserve(cell_count);
            for (std::size_t i = 0; i < cell_count; ++i) {
                if (total_by_cell[i] == 0) {
                    local_densities.push_back(0.0);
                } else {
                    local_densities.push_back(
                        static_cast<double>(occupied_by_cell[i]) / static_cast<double>(total_by_cell[i]));
                }
            }

            const double density_mean = mean(local_densities);
            if (density_mean <= 1e-12) {
                return 0.0;
            }

            return stddev(local_densities, density_mean) / density_mean;
        }

        double compute_radii_cv(const geometry::Scene& scene) {
            if (scene.obstacles.empty()) {
                return 0.0;
            }

            std::vector<double> radii;
            radii.reserve(scene.obstacles.size());
            for (const auto& obstacle : scene.obstacles) {
                if (obstacle.radius > 0.0) {
                    radii.push_back(obstacle.radius);
                }
            }

            if (radii.empty()) {
                return 0.0;
            }

            const double radii_mean = mean(radii);
            if (radii_mean <= 1e-12) {
                return 0.0;
            }

            return stddev(radii, radii_mean) / radii_mean;
        }

        geometry::Point cell_center(const geometry::Scene& scene,
                                    const std::size_t grid_x,
                                    const std::size_t grid_y,
                                    const std::size_t ix,
                                    const std::size_t iy) {
            return {
                (static_cast<double>(ix) + 0.5) * scene.width / static_cast<double>(grid_x),
                (static_cast<double>(iy) + 0.5) * scene.height / static_cast<double>(grid_y)
            };
        }

        std::optional<std::size_t> point_to_cell_index(const geometry::Point& point,
                                                       const geometry::Scene& scene,
                                                       const std::size_t grid_x,
                                                       const std::size_t grid_y) {
            if (grid_x == 0 || grid_y == 0 || !point_inside_scene(point, scene)) {
                return std::nullopt;
            }

            const double cell_width = scene.width / static_cast<double>(grid_x);
            const double cell_height = scene.height / static_cast<double>(grid_y);
            const std::size_t ix = std::min(
                grid_x - 1,
                static_cast<std::size_t>(point.x / std::max(cell_width, 1e-12)));
            const std::size_t iy = std::min(
                grid_y - 1,
                static_cast<std::size_t>(point.y / std::max(cell_height, 1e-12)));
            return iy * grid_x + ix;
        }

        double point_to_workspace_boundary_distance(const geometry::Point& point, const geometry::Scene& scene) {
            return std::min({point.x, scene.width - point.x, point.y, scene.height - point.y});
        }

        double point_clearance(const geometry::Point& point, const geometry::Scene& scene) {
            double nearest_obstacle_gap = std::numeric_limits<double>::infinity();
            for (const auto& obstacle : scene.obstacles) {
                nearest_obstacle_gap = std::min(
                    nearest_obstacle_gap,
                    point.distance(obstacle.center) - obstacle.radius);
            }

            const double boundary_gap = point_to_workspace_boundary_distance(point, scene);
            const double local_clearance = std::min(boundary_gap, nearest_obstacle_gap);
            return std::max(0.0, local_clearance);
        }

        double quantile_of_sorted_values(const std::vector<double>& sorted_values, const double quantile) {
            if (sorted_values.empty()) {
                return 0.0;
            }
            if (sorted_values.size() == 1 || quantile <= 0.0) {
                return sorted_values.front();
            }
            if (quantile >= 1.0) {
                return sorted_values.back();
            }

            const double position = quantile * static_cast<double>(sorted_values.size() - 1);
            const std::size_t lower_index = static_cast<std::size_t>(std::floor(position));
            const std::size_t upper_index = static_cast<std::size_t>(std::ceil(position));
            if (lower_index == upper_index) {
                return sorted_values[lower_index];
            }

            const double alpha = position - static_cast<double>(lower_index);
            return sorted_values[lower_index] * (1.0 - alpha) + sorted_values[upper_index] * alpha;
        }

        double compute_clearance(const geometry::Scene& scene,
                                 const std::size_t grid_x,
                                 const std::size_t grid_y,
                                 const double quantile,
                                 const ClearanceRegionMode region_mode) {
            if (scene.width <= 0.0 || scene.height <= 0.0 || grid_x == 0 || grid_y == 0) {
                return 0.0;
            }

            const std::size_t total_cells = grid_x * grid_y;
            std::vector<unsigned char> is_free(total_cells, 0);
            std::vector<double> cell_clearance(total_cells, 0.0);

            for (std::size_t iy = 0; iy < grid_y; ++iy) {
                for (std::size_t ix = 0; ix < grid_x; ++ix) {
                    const std::size_t index = iy * grid_x + ix;
                    const geometry::Point center = cell_center(scene, grid_x, grid_y, ix, iy);
                    if (!point_inside_obstacles(center, scene)) {
                        is_free[index] = 1;
                        cell_clearance[index] = point_clearance(center, scene);
                    }
                }
            }

            std::vector<int> component_id(total_cells, -1);
            std::vector<std::vector<std::size_t>> components;
            std::queue<std::size_t> frontier;

            for (std::size_t start_index = 0; start_index < total_cells; ++start_index) {
                if (!is_free[start_index] || component_id[start_index] >= 0) {
                    continue;
                }

                const int current_component = static_cast<int>(components.size());
                components.emplace_back();
                component_id[start_index] = current_component;
                frontier.push(start_index);

                while (!frontier.empty()) {
                    const std::size_t current = frontier.front();
                    frontier.pop();
                    components.back().push_back(current);

                    const std::size_t cx = current % grid_x;
                    const std::size_t cy = current / grid_x;

                    const auto try_visit = [&](const std::size_t next) {
                        if (is_free[next] && component_id[next] < 0) {
                            component_id[next] = current_component;
                            frontier.push(next);
                        }
                    };

                    if (cx > 0) {
                        try_visit(current - 1);
                    }
                    if (cx + 1 < grid_x) {
                        try_visit(current + 1);
                    }
                    if (cy > 0) {
                        try_visit(current - grid_x);
                    }
                    if (cy + 1 < grid_y) {
                        try_visit(current + grid_x);
                    }
                }
            }

            if (components.empty()) {
                return 0.0;
            }

            const auto largest_component_it = std::max_element(
                components.begin(),
                components.end(),
                [](const auto& lhs, const auto& rhs) { return lhs.size() < rhs.size(); });
            const std::size_t largest_component_index =
                static_cast<std::size_t>(std::distance(components.begin(), largest_component_it));

            const auto start_cell_index = point_to_cell_index(scene.start, scene, grid_x, grid_y);
            const auto goal_cell_index = point_to_cell_index(scene.goal, scene, grid_x, grid_y);
            std::optional<std::size_t> start_goal_component_index = std::nullopt;
            if (start_cell_index.has_value() && goal_cell_index.has_value() &&
                is_free[start_cell_index.value()] && is_free[goal_cell_index.value()]) {
                const int start_component = component_id[start_cell_index.value()];
                const int goal_component = component_id[goal_cell_index.value()];
                if (start_component >= 0 && start_component == goal_component) {
                    start_goal_component_index = static_cast<std::size_t>(start_component);
                }
            }

            std::size_t selected_component_index = largest_component_index;
            switch (region_mode) {
                case ClearanceRegionMode::LargestFreeComponent:
                    selected_component_index = largest_component_index;
                    break;
                case ClearanceRegionMode::StartGoalComponent:
                    if (!start_goal_component_index.has_value()) {
                        return 0.0;
                    }
                    selected_component_index = start_goal_component_index.value();
                    break;
                case ClearanceRegionMode::StartGoalOrLargest:
                    selected_component_index = start_goal_component_index.value_or(largest_component_index);
                    break;
            }

            const auto& selected_component = components[selected_component_index];
            if (selected_component.empty()) {
                return 0.0;
            }

            std::vector<double> selected_clearances;
            selected_clearances.reserve(selected_component.size());
            for (const std::size_t index : selected_component) {
                selected_clearances.push_back(cell_clearance[index]);
            }
            std::sort(selected_clearances.begin(), selected_clearances.end());
            return quantile_of_sorted_values(selected_clearances, quantile);
        }

        double compute_largest_component_ratio(const geometry::Scene& scene,
                                               const std::size_t grid_x,
                                               const std::size_t grid_y) {
            if (scene.width <= 0.0 || scene.height <= 0.0 || grid_x == 0 || grid_y == 0) {
                return 0.0;
            }

            const std::size_t total_cells = grid_x * grid_y;
            const double cell_width = scene.width / static_cast<double>(grid_x);
            const double cell_height = scene.height / static_cast<double>(grid_y);

            std::vector<unsigned char> is_free(total_cells, 0);
            std::size_t free_cells = 0;

            for (std::size_t y = 0; y < grid_y; ++y) {
                for (std::size_t x = 0; x < grid_x; ++x) {
                    const geometry::Point center{
                        (static_cast<double>(x) + 0.5) * cell_width,
                        (static_cast<double>(y) + 0.5) * cell_height};
                    const std::size_t index = y * grid_x + x;
                    if (!point_inside_obstacles(center, scene)) {
                        is_free[index] = 1;
                        ++free_cells;
                    }
                }
            }

            if (free_cells == 0) {
                return 0.0;
            }

            std::vector<unsigned char> visited(total_cells, 0);
            std::size_t largest_component_size = 0;
            std::queue<std::size_t> queue;

            for (std::size_t start_index = 0; start_index < total_cells; ++start_index) {
                if (visited[start_index] || !is_free[start_index]) {
                    continue;
                }

                visited[start_index] = 1;
                queue.push(start_index);
                std::size_t component_size = 0;

                while (!queue.empty()) {
                    const std::size_t current = queue.front();
                    queue.pop();
                    ++component_size;

                    const std::size_t cx = current % grid_x;
                    const std::size_t cy = current / grid_x;

                    if (cx > 0) {
                        const std::size_t left = current - 1;
                        if (!visited[left] && is_free[left]) {
                            visited[left] = 1;
                            queue.push(left);
                        }
                    }
                    if (cx + 1 < grid_x) {
                        const std::size_t right = current + 1;
                        if (!visited[right] && is_free[right]) {
                            visited[right] = 1;
                            queue.push(right);
                        }
                    }
                    if (cy > 0) {
                        const std::size_t down = current - grid_x;
                        if (!visited[down] && is_free[down]) {
                            visited[down] = 1;
                            queue.push(down);
                        }
                    }
                    if (cy + 1 < grid_y) {
                        const std::size_t up = current + grid_x;
                        if (!visited[up] && is_free[up]) {
                            visited[up] = 1;
                            queue.push(up);
                        }
                    }
                }

                largest_component_size = std::max(largest_component_size, component_size);
            }

            return static_cast<double>(largest_component_size) / static_cast<double>(free_cells);
        }

    } // namespace

    SceneMetricsComputationOptions SceneMetricsCalculator::from_config(const MetricsComputationConfig& config) {
        SceneMetricsComputationOptions options;
        options.compute_obstacle_area_ratio = config.compute_obstacle_area_ratio;
        options.compute_boundary_density = config.compute_boundary_density;
        options.compute_cell_density_cv = config.compute_cell_density_cv;
        options.compute_radii_cv = config.compute_radii_cv;
        options.compute_clearance = config.compute_clearance;
        options.compute_largest_component_ratio = config.compute_largest_component_ratio;

        options.obstacle_area_ratio_method = config.obstacle_area_ratio_method;
        options.obstacle_area_samples = config.obstacle_area_samples;
        options.obstacle_area_grid_resolution = config.obstacle_area_grid_resolution;
        options.boundary_angle_samples = config.boundary_angle_samples;
        options.cell_grid_x = config.cell_grid_x;
        options.cell_grid_y = config.cell_grid_y;
        options.cell_density_samples = config.cell_density_samples;
        options.clearance_grid_x = config.clearance_grid_x;
        options.clearance_grid_y = config.clearance_grid_y;
        options.clearance_quantile = config.clearance_quantile;
        options.clearance_region_mode = config.clearance_region_mode;
        options.connectivity_grid_x = config.connectivity_grid_x;
        options.connectivity_grid_y = config.connectivity_grid_y;
        return options;
    }

    SceneMetrics SceneMetricsCalculator::compute(const geometry::Scene& scene,
                                                 const std::uint32_t seed,
                                                 const SceneMetricsComputationOptions& options) {
        SceneMetrics metrics;
        if (scene.width <= 0.0 || scene.height <= 0.0) {
            return metrics;
        }

        SceneMetricsComputationOptions safe_options = options;
        safe_options.obstacle_area_samples = std::max<std::size_t>(1, safe_options.obstacle_area_samples);
        safe_options.obstacle_area_grid_resolution =
            std::max<std::size_t>(2, safe_options.obstacle_area_grid_resolution);
        safe_options.boundary_angle_samples = std::max<std::size_t>(8, safe_options.boundary_angle_samples);
        safe_options.cell_grid_x = std::max<std::size_t>(1, safe_options.cell_grid_x);
        safe_options.cell_grid_y = std::max<std::size_t>(1, safe_options.cell_grid_y);
        safe_options.cell_density_samples = std::max<std::size_t>(1, safe_options.cell_density_samples);
        safe_options.clearance_grid_x = std::max<std::size_t>(1, safe_options.clearance_grid_x);
        safe_options.clearance_grid_y = std::max<std::size_t>(1, safe_options.clearance_grid_y);
        safe_options.clearance_quantile = std::clamp(safe_options.clearance_quantile, 0.0, 1.0);
        safe_options.connectivity_grid_x = std::max<std::size_t>(1, safe_options.connectivity_grid_x);
        safe_options.connectivity_grid_y = std::max<std::size_t>(1, safe_options.connectivity_grid_y);

        std::mt19937 rng(seed ^ 0x9E3779B9u);

        if (safe_options.compute_obstacle_area_ratio) {
            if (safe_options.obstacle_area_ratio_method == ObstacleAreaRatioMethod::GridSampling) {
                metrics.obstacle_area_ratio =
                    compute_obstacle_area_ratio_grid(scene, safe_options.obstacle_area_grid_resolution);
            } else {
                metrics.obstacle_area_ratio =
                    compute_obstacle_area_ratio(scene, safe_options.obstacle_area_samples, rng);
            }
        }

        if (safe_options.compute_boundary_density) {
            metrics.boundary_density =
                compute_boundary_density(scene, safe_options.boundary_angle_samples);
        }

        if (safe_options.compute_cell_density_cv) {
            metrics.cell_density_cv = compute_cell_density_cv(
                scene,
                safe_options.cell_grid_x,
                safe_options.cell_grid_y,
                safe_options.cell_density_samples,
                rng);
        }

        if (safe_options.compute_radii_cv) {
            metrics.radii_cv = compute_radii_cv(scene);
        }

        if (safe_options.compute_clearance) {
            metrics.clearance = compute_clearance(
                scene,
                safe_options.clearance_grid_x,
                safe_options.clearance_grid_y,
                safe_options.clearance_quantile,
                safe_options.clearance_region_mode);
        }

        if (safe_options.compute_largest_component_ratio) {
            metrics.largest_component_ratio = compute_largest_component_ratio(
                scene,
                safe_options.connectivity_grid_x,
                safe_options.connectivity_grid_y);
        }

        return metrics;
    }

} // namespace experiment
