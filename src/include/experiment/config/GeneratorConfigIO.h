#ifndef EXPERIMENT_GENERATOR_CONFIG_IO_H
#define EXPERIMENT_GENERATOR_CONFIG_IO_H

#include "GeneratorConfig.h"
#include <string>

#include <nlohmann/json.hpp>

namespace experiment {

    class GeneratorConfigIO {
    public:
        static nlohmann::json to_json(const GeneratorConfig& config);
        static GeneratorConfig from_json(const nlohmann::json& j);

        static bool save_to_file(const GeneratorConfig& config, const std::string& filepath);
        static GeneratorConfig load_from_file(const std::string& filepath);
    };

}

#endif
