#ifndef EXPERIMENT_EXPERIMENT_IO_H
#define EXPERIMENT_EXPERIMENT_IO_H

#include "ExperimentModel.h"
#include <string>

#include <nlohmann/json.hpp>

namespace experiment {

    class ExperimentIO {
    public:
        static nlohmann::json to_json(const Experiment& experiment);
        static Experiment from_json(const nlohmann::json& j);

        static bool save_to_file(const Experiment& experiment, const std::string& filepath);
        static Experiment load_from_file(const std::string& filepath);
    };

} // namespace experiment

#endif // EXPERIMENT_EXPERIMENT_IO_H
