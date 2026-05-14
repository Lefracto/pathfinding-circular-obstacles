#ifndef EXPERIMENT_GENERATOR_CONFIG_H
#define EXPERIMENT_GENERATOR_CONFIG_H

#include "../../geometry/Point.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace experiment {

    enum class RealDistributionType {
        Fixed,
        Uniform,
        Normal
    };

    struct RealDistributionSpec {
        RealDistributionType type = RealDistributionType::Uniform;
        double value = 0.0;
        double min = 0.0;
        double max = 1.0;
        double mean = 0.0;
        double stddev = 1.0;
        std::optional<double> clamp_min = std::nullopt;
        std::optional<double> clamp_max = std::nullopt;

        [[nodiscard]] double sample(std::mt19937& rng) const;
        [[nodiscard]] void validate(const std::string& path) const;
    };

    enum class IntDistributionType {
        Fixed,
        Uniform,
        Discrete
    };

    struct IntDistributionSpec {
        IntDistributionType type = IntDistributionType::Uniform;
        std::size_t value = 0;
        std::size_t min = 0;
        std::size_t max = 1;
        std::vector<std::size_t> values;
        std::vector<double> weights;

        [[nodiscard]] std::size_t sample(std::mt19937& rng) const;
        [[nodiscard]] void validate(const std::string& path) const;
    };

    struct PointDistributionSpec {
        RealDistributionSpec x;
        RealDistributionSpec y;

        [[nodiscard]] geometry::Point sample(std::mt19937& rng) const;
        [[nodiscard]] void validate(const std::string& path) const;
    };

    enum class EndpointMode {
        Fixed,
        RandomFree
    };

    struct EndpointConfig {
        EndpointMode mode = EndpointMode::RandomFree;
        geometry::Point fixed_point{0.0, 0.0};
        std::optional<PointDistributionSpec> distribution = std::nullopt;

        [[nodiscard]] void validate(const std::string& path) const;
    };

    struct SceneBoundsConfig {
        double width = 100.0;
        double height = 100.0;
    };

    struct ObstacleGenerationConfig {
        IntDistributionSpec count_distribution;
        RealDistributionSpec radius_distribution;
        PointDistributionSpec center_distribution;
        bool allow_overlap = true;
        bool require_intersection_with_scene = true;
        std::size_t max_attempts_per_obstacle = 100;
    };

    struct StartGoalConfig {
        EndpointConfig start;
        EndpointConfig goal;
        double min_distance = 0.0;
        std::size_t max_attempts = 1000;
    };

    struct AlgorithmConfig {
        std::string id;
        bool enabled = true;
        nlohmann::json params = nlohmann::json::object();
    };

    enum class MetricsExecutionMode {
        Serial,
        Parallel
    };

    enum class ObstacleAreaRatioMethod {
        MonteCarlo,
        GridSampling
    };

    enum class ClearanceRegionMode {
        LargestFreeComponent,
        StartGoalComponent,
        StartGoalOrLargest
    };

    struct MetricsComputationConfig {
        bool auto_compute_on_generation = false;
        MetricsExecutionMode execution_mode = MetricsExecutionMode::Parallel;
        std::size_t worker_threads = 0; // 0 means auto

        bool compute_obstacle_area_ratio = true;
        bool compute_boundary_density = true;
        bool compute_cell_density_cv = true;
        bool compute_radii_cv = true;
        bool compute_clearance = true;
        bool compute_largest_component_ratio = true;

        ObstacleAreaRatioMethod obstacle_area_ratio_method = ObstacleAreaRatioMethod::MonteCarlo;
        std::size_t obstacle_area_samples = 20000;
        std::size_t obstacle_area_grid_resolution = 250;

        std::size_t boundary_angle_samples = 360;

        std::size_t cell_grid_x = 10;
        std::size_t cell_grid_y = 10;
        std::size_t cell_density_samples = 12000;

        std::size_t clearance_grid_x = 140;
        std::size_t clearance_grid_y = 140;
        double clearance_quantile = 0.10;
        ClearanceRegionMode clearance_region_mode = ClearanceRegionMode::StartGoalOrLargest;

        std::size_t connectivity_grid_x = 140;
        std::size_t connectivity_grid_y = 140;
    };

    struct GeneratorConfig {
        int version = 1;
        std::string name = "experiment";
        std::size_t instance_count = 1;
        std::uint32_t global_seed = 42;
        SceneBoundsConfig scene;
        ObstacleGenerationConfig obstacles;
        StartGoalConfig start_goal;
        std::vector<AlgorithmConfig> algorithms;
        MetricsComputationConfig metrics;
        bool save_debug_structures = false;
    };

}

#endif
