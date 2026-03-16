#ifndef EXPERIMENT_EXPERIMENT_SERVICE_H
#define EXPERIMENT_EXPERIMENT_SERVICE_H

#include "ExperimentModel.h"
#include "../config/GeneratorConfig.h"
#include <atomic>
#include <string>

#include <nlohmann/json.hpp>

namespace experiment {

    class ExperimentService {
    public:
        static Experiment create_empty_experiment(const std::string& experiment_name = "untitled_experiment");

        static Experiment create_experiment(const GeneratorConfig& config,
                                            const std::string& experiment_name,
                                            const std::string& source_config_path = "");

        static void generate_instances(Experiment& experiment,
                                       std::atomic<std::size_t>* generated_counter = nullptr);

        static std::size_t estimate_total_algorithm_runs(const Experiment& experiment);
        static std::size_t estimate_total_algorithm_runs_for_scene(const Experiment& experiment,
                                                                   std::size_t scene_index);
        static void run_algorithms(Experiment& experiment,
                                   std::atomic<std::size_t>* completed_runs = nullptr);
        static void run_algorithms_for_scene(Experiment& experiment,
                                             std::size_t scene_index,
                                             std::atomic<std::size_t>* completed_runs = nullptr);

        static nlohmann::json build_algorithm_report_json(const Experiment& experiment);
        static bool save_algorithm_report_json(const Experiment& experiment, const std::string& filepath);
    };

} // namespace experiment

#endif // EXPERIMENT_EXPERIMENT_SERVICE_H


