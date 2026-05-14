#ifndef EXPERIMENT_SCENE_BATCH_GENERATOR_H
#define EXPERIMENT_SCENE_BATCH_GENERATOR_H

#include "../core/ExperimentModel.h"
#include "../config/GeneratorConfig.h"
#include <atomic>
#include <vector>

namespace experiment {

    class SceneBatchGenerator {
    public:
        static std::vector<SceneInstance> generate_instances(const GeneratorConfig& config,
                                                             std::atomic<std::size_t>* completed_counter = nullptr);
        static std::vector<SceneInstance> generate_instances_for_indices(
            const GeneratorConfig& config,
            const std::vector<std::size_t>& scene_indices,
            std::atomic<std::size_t>* completed_counter = nullptr);
    };

}

#endif
