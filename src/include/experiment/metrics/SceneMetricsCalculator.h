#ifndef EXPERIMENT_SCENE_METRICS_CALCULATOR_H
#define EXPERIMENT_SCENE_METRICS_CALCULATOR_H

#include "../core/ExperimentModel.h"
#include "../../geometry/Scene.h"
#include <cstddef>
#include <cstdint>

namespace experiment {

    struct SceneMetricsComputationOptions {
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

    class SceneMetricsCalculator {
    public:
        static SceneMetricsComputationOptions from_config(const MetricsComputationConfig& config);

        static SceneMetrics compute(const geometry::Scene& scene,
                                    std::uint32_t seed,
                                    const SceneMetricsComputationOptions& options = {});
    };

}

#endif
