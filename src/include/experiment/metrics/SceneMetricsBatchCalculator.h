#ifndef EXPERIMENT_SCENE_METRICS_BATCH_CALCULATOR_H
#define EXPERIMENT_SCENE_METRICS_BATCH_CALCULATOR_H

#include "../core/ExperimentModel.h"
#include "../config/GeneratorConfig.h"
#include <atomic>
#include <vector>

namespace experiment {

    struct SceneMetricsBatchResult {
        std::vector<SceneMetrics> metrics;
    };

    class SceneMetricsBatchCalculator {
    public:
        static SceneMetricsBatchResult compute(const std::vector<SceneInstance>& instances,
                                               const MetricsComputationConfig& config,
                                               std::atomic<std::size_t>* completed_counter = nullptr);
    };

} // namespace experiment

#endif // EXPERIMENT_SCENE_METRICS_BATCH_CALCULATOR_H
