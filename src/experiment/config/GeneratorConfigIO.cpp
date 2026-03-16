#include "experiment/config/GeneratorConfigIO.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace experiment {

    namespace {

        using json = nlohmann::json;

        std::string to_string(const RealDistributionType type) {
            switch (type) {
                case RealDistributionType::Fixed:
                    return "fixed";
                case RealDistributionType::Uniform:
                    return "uniform_real";
                case RealDistributionType::Normal:
                    return "normal";
            }
            throw std::runtime_error("Unknown RealDistributionType");
        }

        RealDistributionType real_distribution_type_from_string(const std::string& value) {
            if (value == "fixed") {
                return RealDistributionType::Fixed;
            }
            if (value == "uniform_real") {
                return RealDistributionType::Uniform;
            }
            if (value == "normal") {
                return RealDistributionType::Normal;
            }
            throw std::runtime_error("Unknown real distribution type: " + value);
        }

        std::string to_string(const IntDistributionType type) {
            switch (type) {
                case IntDistributionType::Fixed:
                    return "fixed";
                case IntDistributionType::Uniform:
                    return "uniform_int";
                case IntDistributionType::Discrete:
                    return "discrete_int";
            }
            throw std::runtime_error("Unknown IntDistributionType");
        }

        IntDistributionType int_distribution_type_from_string(const std::string& value) {
            if (value == "fixed") {
                return IntDistributionType::Fixed;
            }
            if (value == "uniform_int") {
                return IntDistributionType::Uniform;
            }
            if (value == "discrete_int") {
                return IntDistributionType::Discrete;
            }
            throw std::runtime_error("Unknown int distribution type: " + value);
        }

        std::string to_string(const EndpointMode mode) {
            switch (mode) {
                case EndpointMode::Fixed:
                    return "fixed";
                case EndpointMode::RandomFree:
                    return "random_free";
            }
            throw std::runtime_error("Unknown EndpointMode");
        }

        EndpointMode endpoint_mode_from_string(const std::string& value) {
            if (value == "fixed") {
                return EndpointMode::Fixed;
            }
            if (value == "random_free") {
                return EndpointMode::RandomFree;
            }
            throw std::runtime_error("Unknown endpoint mode: " + value);
        }

        std::string to_string(const MetricsExecutionMode mode) {
            switch (mode) {
                case MetricsExecutionMode::Serial:
                    return "serial";
                case MetricsExecutionMode::Parallel:
                    return "parallel";
            }
            throw std::runtime_error("Unknown MetricsExecutionMode");
        }

        MetricsExecutionMode metrics_execution_mode_from_string(const std::string& value) {
            if (value == "serial") {
                return MetricsExecutionMode::Serial;
            }
            if (value == "parallel") {
                return MetricsExecutionMode::Parallel;
            }
            throw std::runtime_error("Unknown metrics execution mode: " + value);
        }

        std::string to_string(const ObstacleAreaRatioMethod method) {
            switch (method) {
                case ObstacleAreaRatioMethod::MonteCarlo:
                    return "monte_carlo";
                case ObstacleAreaRatioMethod::GridSampling:
                    return "grid_sampling";
            }
            throw std::runtime_error("Unknown ObstacleAreaRatioMethod");
        }

        ObstacleAreaRatioMethod obstacle_area_ratio_method_from_string(const std::string& value) {
            if (value == "monte_carlo") {
                return ObstacleAreaRatioMethod::MonteCarlo;
            }
            if (value == "grid_sampling") {
                return ObstacleAreaRatioMethod::GridSampling;
            }
            throw std::runtime_error("Unknown obstacle area ratio method: " + value);
        }

        std::string to_string(const ClearanceRegionMode mode) {
            switch (mode) {
                case ClearanceRegionMode::LargestFreeComponent:
                    return "largest_free_component";
                case ClearanceRegionMode::StartGoalComponent:
                    return "start_goal_component";
                case ClearanceRegionMode::StartGoalOrLargest:
                    return "start_goal_or_largest";
            }
            throw std::runtime_error("Unknown ClearanceRegionMode");
        }

        ClearanceRegionMode clearance_region_mode_from_string(const std::string& value) {
            if (value == "largest_free_component") {
                return ClearanceRegionMode::LargestFreeComponent;
            }
            if (value == "start_goal_component") {
                return ClearanceRegionMode::StartGoalComponent;
            }
            if (value == "start_goal_or_largest") {
                return ClearanceRegionMode::StartGoalOrLargest;
            }
            throw std::runtime_error("Unknown clearance region mode: " + value);
        }

        double require_number(const json& j, const char* field, const std::string& path) {
            if (!j.contains(field) || !j[field].is_number()) {
                throw std::runtime_error(path + "." + field + " must be a number");
            }
            return j[field].get<double>();
        }

        std::size_t require_size_t(const json& j, const char* field, const std::string& path) {
            if (!j.contains(field) || !j[field].is_number_unsigned()) {
                throw std::runtime_error(path + "." + field + " must be an unsigned integer");
            }
            return j[field].get<std::size_t>();
        }

        geometry::Point point_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            if (!j.contains("x") || !j["x"].is_number()) {
                throw std::runtime_error(path + ".x must be a number");
            }
            if (!j.contains("y") || !j["y"].is_number()) {
                throw std::runtime_error(path + ".y must be a number");
            }
            return {j["x"].get<double>(), j["y"].get<double>()};
        }

        json point_to_json(const geometry::Point& p) {
            return json{{"x", p.x}, {"y", p.y}};
        }

        RealDistributionSpec real_distribution_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            if (!j.contains("type") || !j["type"].is_string()) {
                throw std::runtime_error(path + ".type must be a string");
            }

            RealDistributionSpec spec;
            spec.type = real_distribution_type_from_string(j["type"].get<std::string>());

            if (j.contains("clamp_min")) {
                if (!j["clamp_min"].is_number()) {
                    throw std::runtime_error(path + ".clamp_min must be a number");
                }
                spec.clamp_min = j["clamp_min"].get<double>();
            }
            if (j.contains("clamp_max")) {
                if (!j["clamp_max"].is_number()) {
                    throw std::runtime_error(path + ".clamp_max must be a number");
                }
                spec.clamp_max = j["clamp_max"].get<double>();
            }

            switch (spec.type) {
                case RealDistributionType::Fixed:
                    spec.value = require_number(j, "value", path);
                    break;
                case RealDistributionType::Uniform:
                    spec.min = require_number(j, "min", path);
                    spec.max = require_number(j, "max", path);
                    break;
                case RealDistributionType::Normal:
                    spec.mean = require_number(j, "mean", path);
                    spec.stddev = require_number(j, "stddev", path);
                    break;
            }

            spec.validate(path);
            return spec;
        }

        json real_distribution_to_json(const RealDistributionSpec& spec) {
            json j;
            j["type"] = to_string(spec.type);

            switch (spec.type) {
                case RealDistributionType::Fixed:
                    j["value"] = spec.value;
                    break;
                case RealDistributionType::Uniform:
                    j["min"] = spec.min;
                    j["max"] = spec.max;
                    break;
                case RealDistributionType::Normal:
                    j["mean"] = spec.mean;
                    j["stddev"] = spec.stddev;
                    break;
            }

            if (spec.clamp_min.has_value()) {
                j["clamp_min"] = spec.clamp_min.value();
            }
            if (spec.clamp_max.has_value()) {
                j["clamp_max"] = spec.clamp_max.value();
            }

            return j;
        }

        IntDistributionSpec int_distribution_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            if (!j.contains("type") || !j["type"].is_string()) {
                throw std::runtime_error(path + ".type must be a string");
            }

            IntDistributionSpec spec;
            spec.type = int_distribution_type_from_string(j["type"].get<std::string>());

            switch (spec.type) {
                case IntDistributionType::Fixed:
                    spec.value = require_size_t(j, "value", path);
                    break;
                case IntDistributionType::Uniform:
                    spec.min = require_size_t(j, "min", path);
                    spec.max = require_size_t(j, "max", path);
                    break;
                case IntDistributionType::Discrete: {
                    if (!j.contains("values") || !j["values"].is_array() || j["values"].empty()) {
                        throw std::runtime_error(path + ".values must be a non-empty array");
                    }
                    for (const auto& value : j["values"]) {
                        if (!value.is_number_unsigned()) {
                            throw std::runtime_error(path + ".values must contain unsigned integers");
                        }
                        spec.values.push_back(value.get<std::size_t>());
                    }

                    if (j.contains("weights")) {
                        if (!j["weights"].is_array()) {
                            throw std::runtime_error(path + ".weights must be an array");
                        }
                        for (const auto& weight : j["weights"]) {
                            if (!weight.is_number()) {
                                throw std::runtime_error(path + ".weights must contain numbers");
                            }
                            spec.weights.push_back(weight.get<double>());
                        }
                    }
                    break;
                }
            }

            spec.validate(path);
            return spec;
        }

        json int_distribution_to_json(const IntDistributionSpec& spec) {
            json j;
            j["type"] = to_string(spec.type);

            switch (spec.type) {
                case IntDistributionType::Fixed:
                    j["value"] = spec.value;
                    break;
                case IntDistributionType::Uniform:
                    j["min"] = spec.min;
                    j["max"] = spec.max;
                    break;
                case IntDistributionType::Discrete:
                    j["values"] = spec.values;
                    if (!spec.weights.empty()) {
                        j["weights"] = spec.weights;
                    }
                    break;
            }

            return j;
        }

        PointDistributionSpec point_distribution_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            if (!j.contains("x") || !j.contains("y")) {
                throw std::runtime_error(path + " must contain x and y distributions");
            }

            PointDistributionSpec spec;
            spec.x = real_distribution_from_json(j["x"], path + ".x");
            spec.y = real_distribution_from_json(j["y"], path + ".y");
            spec.validate(path);
            return spec;
        }

        json point_distribution_to_json(const PointDistributionSpec& spec) {
            return json{{"x", real_distribution_to_json(spec.x)},
                        {"y", real_distribution_to_json(spec.y)}};
        }

        EndpointConfig endpoint_config_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }

            EndpointConfig endpoint;
            if (j.contains("mode")) {
                if (!j["mode"].is_string()) {
                    throw std::runtime_error(path + ".mode must be a string");
                }
                endpoint.mode = endpoint_mode_from_string(j["mode"].get<std::string>());
            }

            if (j.contains("fixed_point")) {
                endpoint.fixed_point = point_from_json(j["fixed_point"], path + ".fixed_point");
            }

            if (j.contains("distribution")) {
                endpoint.distribution = point_distribution_from_json(j["distribution"], path + ".distribution");
            }

            endpoint.validate(path);
            return endpoint;
        }

        json endpoint_config_to_json(const EndpointConfig& endpoint) {
            json j;
            j["mode"] = to_string(endpoint.mode);
            j["fixed_point"] = point_to_json(endpoint.fixed_point);
            if (endpoint.distribution.has_value()) {
                j["distribution"] = point_distribution_to_json(endpoint.distribution.value());
            }
            return j;
        }

    } // namespace

    double RealDistributionSpec::sample(std::mt19937& rng) const {
        switch (type) {
            case RealDistributionType::Fixed:
                return value;
            case RealDistributionType::Uniform: {
                std::uniform_real_distribution<double> distribution(min, max);
                return distribution(rng);
            }
            case RealDistributionType::Normal: {
                std::normal_distribution<double> distribution(mean, stddev);
                double sampled = distribution(rng);
                if (!clamp_min.has_value() && !clamp_max.has_value()) {
                    return sampled;
                }

                for (int i = 0; i < 64; ++i) {
                    bool below_min = clamp_min.has_value() && sampled < clamp_min.value();
                    bool above_max = clamp_max.has_value() && sampled > clamp_max.value();
                    if (!below_min && !above_max) {
                        return sampled;
                    }
                    sampled = distribution(rng);
                }

                if (clamp_min.has_value()) {
                    sampled = std::max(sampled, clamp_min.value());
                }
                if (clamp_max.has_value()) {
                    sampled = std::min(sampled, clamp_max.value());
                }
                return sampled;
            }
        }

        throw std::runtime_error("Unknown real distribution type");
    }

    void RealDistributionSpec::validate(const std::string& path) const {
        switch (type) {
            case RealDistributionType::Fixed:
                if (!std::isfinite(value)) {
                    throw std::runtime_error(path + ".value must be finite");
                }
                break;
            case RealDistributionType::Uniform:
                if (!std::isfinite(min) || !std::isfinite(max)) {
                    throw std::runtime_error(path + ".min/max must be finite");
                }
                if (min > max) {
                    throw std::runtime_error(path + ".min must be <= max");
                }
                break;
            case RealDistributionType::Normal:
                if (!std::isfinite(mean) || !std::isfinite(stddev)) {
                    throw std::runtime_error(path + ".mean/stddev must be finite");
                }
                if (stddev <= 0.0) {
                    throw std::runtime_error(path + ".stddev must be > 0");
                }
                break;
        }

        if (clamp_min.has_value() && !std::isfinite(clamp_min.value())) {
            throw std::runtime_error(path + ".clamp_min must be finite");
        }
        if (clamp_max.has_value() && !std::isfinite(clamp_max.value())) {
            throw std::runtime_error(path + ".clamp_max must be finite");
        }
        if (clamp_min.has_value() && clamp_max.has_value() && clamp_min.value() > clamp_max.value()) {
            throw std::runtime_error(path + ".clamp_min must be <= clamp_max");
        }
    }

    std::size_t IntDistributionSpec::sample(std::mt19937& rng) const {
        switch (type) {
            case IntDistributionType::Fixed:
                return value;
            case IntDistributionType::Uniform: {
                std::uniform_int_distribution<std::size_t> distribution(min, max);
                return distribution(rng);
            }
            case IntDistributionType::Discrete: {
                std::vector<double> local_weights = weights;
                if (local_weights.empty()) {
                    local_weights.assign(values.size(), 1.0);
                }
                std::discrete_distribution<std::size_t> distribution(local_weights.begin(), local_weights.end());
                return values[distribution(rng)];
            }
        }
        throw std::runtime_error("Unknown int distribution type");
    }

    void IntDistributionSpec::validate(const std::string& path) const {
        switch (type) {
            case IntDistributionType::Fixed:
                break;
            case IntDistributionType::Uniform:
                if (min > max) {
                    throw std::runtime_error(path + ".min must be <= max");
                }
                break;
            case IntDistributionType::Discrete:
                if (values.empty()) {
                    throw std::runtime_error(path + ".values must not be empty");
                }
                if (!weights.empty() && weights.size() != values.size()) {
                    throw std::runtime_error(path + ".weights size must match values size");
                }
                for (double weight : weights) {
                    if (weight < 0.0 || !std::isfinite(weight)) {
                        throw std::runtime_error(path + ".weights must be non-negative finite values");
                    }
                }
                break;
        }
    }

    geometry::Point PointDistributionSpec::sample(std::mt19937& rng) const {
        return {x.sample(rng), y.sample(rng)};
    }

    void PointDistributionSpec::validate(const std::string& path) const {
        x.validate(path + ".x");
        y.validate(path + ".y");
    }

    void EndpointConfig::validate(const std::string& path) const {
        if (mode == EndpointMode::RandomFree && distribution.has_value()) {
            distribution.value().validate(path + ".distribution");
        }
    }

    nlohmann::json GeneratorConfigIO::to_json(const GeneratorConfig& config) {
        json j;
        j["version"] = config.version;
        j["name"] = config.name;
        j["instance_count"] = config.instance_count;
        j["global_seed"] = config.global_seed;
        j["scene"] = {{"width", config.scene.width}, {"height", config.scene.height}};

        j["obstacles"] = {
            {"count_distribution", int_distribution_to_json(config.obstacles.count_distribution)},
            {"radius_distribution", real_distribution_to_json(config.obstacles.radius_distribution)},
            {"center_distribution", point_distribution_to_json(config.obstacles.center_distribution)},
            {"allow_overlap", config.obstacles.allow_overlap},
            {"require_intersection_with_scene", config.obstacles.require_intersection_with_scene},
            {"max_attempts_per_obstacle", config.obstacles.max_attempts_per_obstacle}
        };

        j["start_goal"] = {
            {"start", endpoint_config_to_json(config.start_goal.start)},
            {"goal", endpoint_config_to_json(config.start_goal.goal)},
            {"min_distance", config.start_goal.min_distance},
            {"max_attempts", config.start_goal.max_attempts}
        };

        json algorithms = json::array();
        for (const auto& algorithm : config.algorithms) {
            json algorithm_json;
            algorithm_json["id"] = algorithm.id;
            algorithm_json["enabled"] = algorithm.enabled;
            algorithm_json["params"] = algorithm.params;
            algorithms.push_back(algorithm_json);
        }
        j["algorithms"] = algorithms;

        j["metrics"] = {
            {"auto_compute_on_generation", config.metrics.auto_compute_on_generation},
            {"execution_mode", to_string(config.metrics.execution_mode)},
            {"worker_threads", config.metrics.worker_threads},
            {"compute_obstacle_area_ratio", config.metrics.compute_obstacle_area_ratio},
            {"compute_boundary_density", config.metrics.compute_boundary_density},
            {"compute_cell_density_cv", config.metrics.compute_cell_density_cv},
            {"compute_radii_cv", config.metrics.compute_radii_cv},
            {"compute_clearance", config.metrics.compute_clearance},
            {"compute_largest_component_ratio", config.metrics.compute_largest_component_ratio},
            {"obstacle_area_ratio_method", to_string(config.metrics.obstacle_area_ratio_method)},
            {"obstacle_area_samples", config.metrics.obstacle_area_samples},
            {"obstacle_area_grid_resolution", config.metrics.obstacle_area_grid_resolution},
            {"boundary_angle_samples", config.metrics.boundary_angle_samples},
            {"cell_grid_x", config.metrics.cell_grid_x},
            {"cell_grid_y", config.metrics.cell_grid_y},
            {"cell_density_samples", config.metrics.cell_density_samples},
            {"clearance_grid_x", config.metrics.clearance_grid_x},
            {"clearance_grid_y", config.metrics.clearance_grid_y},
            {"clearance_quantile", config.metrics.clearance_quantile},
            {"clearance_region_mode", to_string(config.metrics.clearance_region_mode)},
            {"connectivity_grid_x", config.metrics.connectivity_grid_x},
            {"connectivity_grid_y", config.metrics.connectivity_grid_y}
        };

        j["output"] = {{"save_debug_structures", config.save_debug_structures}};

        return j;
    }

    GeneratorConfig GeneratorConfigIO::from_json(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw std::runtime_error("Generator config must be a JSON object");
        }

        GeneratorConfig config;

        if (j.contains("version")) {
            if (!j["version"].is_number_integer()) {
                throw std::runtime_error("version must be an integer");
            }
            config.version = j["version"].get<int>();
        }
        if (j.contains("name")) {
            if (!j["name"].is_string()) {
                throw std::runtime_error("name must be a string");
            }
            config.name = j["name"].get<std::string>();
        }
        if (j.contains("instance_count")) {
            if (!j["instance_count"].is_number_unsigned()) {
                throw std::runtime_error("instance_count must be an unsigned integer");
            }
            config.instance_count = j["instance_count"].get<std::size_t>();
        }
        if (j.contains("global_seed")) {
            if (!j["global_seed"].is_number_unsigned()) {
                throw std::runtime_error("global_seed must be an unsigned integer");
            }
            config.global_seed = j["global_seed"].get<std::uint32_t>();
        }

        if (!j.contains("scene") || !j["scene"].is_object()) {
            throw std::runtime_error("scene section is required");
        }
        config.scene.width = require_number(j["scene"], "width", "scene");
        config.scene.height = require_number(j["scene"], "height", "scene");
        if (config.scene.width <= 0.0 || config.scene.height <= 0.0) {
            throw std::runtime_error("scene width and height must be > 0");
        }

        if (!j.contains("obstacles") || !j["obstacles"].is_object()) {
            throw std::runtime_error("obstacles section is required");
        }
        const json& obstacles = j["obstacles"];
        if (!obstacles.contains("count_distribution")) {
            throw std::runtime_error("obstacles.count_distribution is required");
        }
        if (!obstacles.contains("radius_distribution")) {
            throw std::runtime_error("obstacles.radius_distribution is required");
        }

        config.obstacles.count_distribution =
            int_distribution_from_json(obstacles["count_distribution"], "obstacles.count_distribution");
        config.obstacles.radius_distribution =
            real_distribution_from_json(obstacles["radius_distribution"], "obstacles.radius_distribution");

        if (obstacles.contains("center_distribution")) {
            config.obstacles.center_distribution =
                point_distribution_from_json(obstacles["center_distribution"], "obstacles.center_distribution");
        } else {
            config.obstacles.center_distribution.x.type = RealDistributionType::Uniform;
            config.obstacles.center_distribution.x.min = 0.0;
            config.obstacles.center_distribution.x.max = config.scene.width;
            config.obstacles.center_distribution.y.type = RealDistributionType::Uniform;
            config.obstacles.center_distribution.y.min = 0.0;
            config.obstacles.center_distribution.y.max = config.scene.height;
        }

        if (obstacles.contains("allow_overlap")) {
            if (!obstacles["allow_overlap"].is_boolean()) {
                throw std::runtime_error("obstacles.allow_overlap must be a boolean");
            }
            config.obstacles.allow_overlap = obstacles["allow_overlap"].get<bool>();
        }
        if (obstacles.contains("require_intersection_with_scene")) {
            if (!obstacles["require_intersection_with_scene"].is_boolean()) {
                throw std::runtime_error("obstacles.require_intersection_with_scene must be a boolean");
            }
            config.obstacles.require_intersection_with_scene =
                obstacles["require_intersection_with_scene"].get<bool>();
        }
        if (obstacles.contains("max_attempts_per_obstacle")) {
            if (!obstacles["max_attempts_per_obstacle"].is_number_unsigned()) {
                throw std::runtime_error("obstacles.max_attempts_per_obstacle must be an unsigned integer");
            }
            config.obstacles.max_attempts_per_obstacle = obstacles["max_attempts_per_obstacle"].get<std::size_t>();
        }
        if (config.obstacles.max_attempts_per_obstacle == 0) {
            throw std::runtime_error("obstacles.max_attempts_per_obstacle must be > 0");
        }

        if (!j.contains("start_goal") || !j["start_goal"].is_object()) {
            throw std::runtime_error("start_goal section is required");
        }
        const json& start_goal = j["start_goal"];
        if (!start_goal.contains("start") || !start_goal.contains("goal")) {
            throw std::runtime_error("start_goal.start and start_goal.goal are required");
        }
        config.start_goal.start = endpoint_config_from_json(start_goal["start"], "start_goal.start");
        config.start_goal.goal = endpoint_config_from_json(start_goal["goal"], "start_goal.goal");
        if (start_goal.contains("min_distance")) {
            config.start_goal.min_distance = require_number(start_goal, "min_distance", "start_goal");
        }
        if (start_goal.contains("max_attempts")) {
            if (!start_goal["max_attempts"].is_number_unsigned()) {
                throw std::runtime_error("start_goal.max_attempts must be an unsigned integer");
            }
            config.start_goal.max_attempts = start_goal["max_attempts"].get<std::size_t>();
        }
        if (config.start_goal.min_distance < 0.0) {
            throw std::runtime_error("start_goal.min_distance must be >= 0");
        }
        if (config.start_goal.max_attempts == 0) {
            throw std::runtime_error("start_goal.max_attempts must be > 0");
        }

        config.algorithms.clear();
        if (j.contains("algorithms")) {
            if (!j["algorithms"].is_array()) {
                throw std::runtime_error("algorithms must be an array");
            }
            for (std::size_t i = 0; i < j["algorithms"].size(); ++i) {
                const auto& algorithm_json = j["algorithms"][i];
                const std::string path = "algorithms[" + std::to_string(i) + "]";
                if (!algorithm_json.is_object()) {
                    throw std::runtime_error(path + " must be an object");
                }
                if (!algorithm_json.contains("id") || !algorithm_json["id"].is_string()) {
                    throw std::runtime_error(path + ".id must be a string");
                }

                AlgorithmConfig algorithm;
                algorithm.id = algorithm_json["id"].get<std::string>();
                if (algorithm_json.contains("enabled")) {
                    if (!algorithm_json["enabled"].is_boolean()) {
                        throw std::runtime_error(path + ".enabled must be a boolean");
                    }
                    algorithm.enabled = algorithm_json["enabled"].get<bool>();
                }
                if (algorithm_json.contains("params")) {
                    if (!algorithm_json["params"].is_object()) {
                        throw std::runtime_error(path + ".params must be an object");
                    }
                    algorithm.params = algorithm_json["params"];
                }
                config.algorithms.push_back(algorithm);
            }
        }

        if (j.contains("metrics")) {
            if (!j["metrics"].is_object()) {
                throw std::runtime_error("metrics must be an object");
            }
            const auto& metrics = j["metrics"];

            if (metrics.contains("auto_compute_on_generation")) {
                if (!metrics["auto_compute_on_generation"].is_boolean()) {
                    throw std::runtime_error("metrics.auto_compute_on_generation must be a boolean");
                }
                config.metrics.auto_compute_on_generation = metrics["auto_compute_on_generation"].get<bool>();
            }
            if (metrics.contains("execution_mode")) {
                if (!metrics["execution_mode"].is_string()) {
                    throw std::runtime_error("metrics.execution_mode must be a string");
                }
                config.metrics.execution_mode =
                    metrics_execution_mode_from_string(metrics["execution_mode"].get<std::string>());
            }
            if (metrics.contains("worker_threads")) {
                if (!metrics["worker_threads"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.worker_threads must be an unsigned integer");
                }
                config.metrics.worker_threads = metrics["worker_threads"].get<std::size_t>();
            }
            if (metrics.contains("compute_obstacle_area_ratio")) {
                if (!metrics["compute_obstacle_area_ratio"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_obstacle_area_ratio must be a boolean");
                }
                config.metrics.compute_obstacle_area_ratio = metrics["compute_obstacle_area_ratio"].get<bool>();
            }
            if (metrics.contains("compute_boundary_density")) {
                if (!metrics["compute_boundary_density"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_boundary_density must be a boolean");
                }
                config.metrics.compute_boundary_density = metrics["compute_boundary_density"].get<bool>();
            }
            if (metrics.contains("compute_cell_density_cv")) {
                if (!metrics["compute_cell_density_cv"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_cell_density_cv must be a boolean");
                }
                config.metrics.compute_cell_density_cv = metrics["compute_cell_density_cv"].get<bool>();
            }
            if (metrics.contains("compute_radii_cv")) {
                if (!metrics["compute_radii_cv"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_radii_cv must be a boolean");
                }
                config.metrics.compute_radii_cv = metrics["compute_radii_cv"].get<bool>();
            }
            if (metrics.contains("compute_clearance")) {
                if (!metrics["compute_clearance"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_clearance must be a boolean");
                }
                config.metrics.compute_clearance = metrics["compute_clearance"].get<bool>();
            }
            if (metrics.contains("compute_largest_component_ratio")) {
                if (!metrics["compute_largest_component_ratio"].is_boolean()) {
                    throw std::runtime_error("metrics.compute_largest_component_ratio must be a boolean");
                }
                config.metrics.compute_largest_component_ratio =
                    metrics["compute_largest_component_ratio"].get<bool>();
            }
            if (metrics.contains("obstacle_area_ratio_method")) {
                if (!metrics["obstacle_area_ratio_method"].is_string()) {
                    throw std::runtime_error("metrics.obstacle_area_ratio_method must be a string");
                }
                config.metrics.obstacle_area_ratio_method =
                    obstacle_area_ratio_method_from_string(metrics["obstacle_area_ratio_method"].get<std::string>());
            }
            if (metrics.contains("obstacle_area_samples")) {
                if (!metrics["obstacle_area_samples"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.obstacle_area_samples must be an unsigned integer");
                }
                config.metrics.obstacle_area_samples = metrics["obstacle_area_samples"].get<std::size_t>();
            }
            if (metrics.contains("obstacle_area_grid_resolution")) {
                if (!metrics["obstacle_area_grid_resolution"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.obstacle_area_grid_resolution must be an unsigned integer");
                }
                config.metrics.obstacle_area_grid_resolution =
                    metrics["obstacle_area_grid_resolution"].get<std::size_t>();
            }
            if (metrics.contains("boundary_angle_samples")) {
                if (!metrics["boundary_angle_samples"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.boundary_angle_samples must be an unsigned integer");
                }
                config.metrics.boundary_angle_samples = metrics["boundary_angle_samples"].get<std::size_t>();
            }
            if (metrics.contains("cell_grid_x")) {
                if (!metrics["cell_grid_x"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.cell_grid_x must be an unsigned integer");
                }
                config.metrics.cell_grid_x = metrics["cell_grid_x"].get<std::size_t>();
            }
            if (metrics.contains("cell_grid_y")) {
                if (!metrics["cell_grid_y"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.cell_grid_y must be an unsigned integer");
                }
                config.metrics.cell_grid_y = metrics["cell_grid_y"].get<std::size_t>();
            }
            if (metrics.contains("cell_density_samples")) {
                if (!metrics["cell_density_samples"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.cell_density_samples must be an unsigned integer");
                }
                config.metrics.cell_density_samples = metrics["cell_density_samples"].get<std::size_t>();
            }
            if (metrics.contains("clearance_grid_x")) {
                if (!metrics["clearance_grid_x"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.clearance_grid_x must be an unsigned integer");
                }
                config.metrics.clearance_grid_x = metrics["clearance_grid_x"].get<std::size_t>();
            }
            if (metrics.contains("clearance_grid_y")) {
                if (!metrics["clearance_grid_y"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.clearance_grid_y must be an unsigned integer");
                }
                config.metrics.clearance_grid_y = metrics["clearance_grid_y"].get<std::size_t>();
            }
            if (metrics.contains("clearance_quantile")) {
                if (!metrics["clearance_quantile"].is_number()) {
                    throw std::runtime_error("metrics.clearance_quantile must be a number");
                }
                config.metrics.clearance_quantile = metrics["clearance_quantile"].get<double>();
            }
            if (metrics.contains("clearance_region_mode")) {
                if (!metrics["clearance_region_mode"].is_string()) {
                    throw std::runtime_error("metrics.clearance_region_mode must be a string");
                }
                config.metrics.clearance_region_mode =
                    clearance_region_mode_from_string(metrics["clearance_region_mode"].get<std::string>());
            }
            if (metrics.contains("connectivity_grid_x")) {
                if (!metrics["connectivity_grid_x"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.connectivity_grid_x must be an unsigned integer");
                }
                config.metrics.connectivity_grid_x = metrics["connectivity_grid_x"].get<std::size_t>();
            }
            if (metrics.contains("connectivity_grid_y")) {
                if (!metrics["connectivity_grid_y"].is_number_unsigned()) {
                    throw std::runtime_error("metrics.connectivity_grid_y must be an unsigned integer");
                }
                config.metrics.connectivity_grid_y = metrics["connectivity_grid_y"].get<std::size_t>();
            }
        }

        if (j.contains("output")) {
            if (!j["output"].is_object()) {
                throw std::runtime_error("output must be an object");
            }
            const auto& output = j["output"];
            if (output.contains("save_debug_structures")) {
                if (!output["save_debug_structures"].is_boolean()) {
                    throw std::runtime_error("output.save_debug_structures must be a boolean");
                }
                config.save_debug_structures = output["save_debug_structures"].get<bool>();
            }
        }

        if (config.instance_count == 0) {
            throw std::runtime_error("instance_count must be > 0");
        }
        config.start_goal.start.validate("start_goal.start");
        config.start_goal.goal.validate("start_goal.goal");
        config.obstacles.count_distribution.validate("obstacles.count_distribution");
        config.obstacles.radius_distribution.validate("obstacles.radius_distribution");
        config.obstacles.center_distribution.validate("obstacles.center_distribution");

        if (config.metrics.obstacle_area_samples == 0) {
            throw std::runtime_error("metrics.obstacle_area_samples must be > 0");
        }
        if (config.metrics.obstacle_area_grid_resolution == 0) {
            throw std::runtime_error("metrics.obstacle_area_grid_resolution must be > 0");
        }
        if (config.metrics.boundary_angle_samples == 0) {
            throw std::runtime_error("metrics.boundary_angle_samples must be > 0");
        }
        if (config.metrics.cell_grid_x == 0 || config.metrics.cell_grid_y == 0) {
            throw std::runtime_error("metrics.cell_grid_x and metrics.cell_grid_y must be > 0");
        }
        if (config.metrics.cell_density_samples == 0) {
            throw std::runtime_error("metrics.cell_density_samples must be > 0");
        }
        if (config.metrics.clearance_grid_x == 0 || config.metrics.clearance_grid_y == 0) {
            throw std::runtime_error("metrics.clearance_grid_x and metrics.clearance_grid_y must be > 0");
        }
        if (!std::isfinite(config.metrics.clearance_quantile) ||
            config.metrics.clearance_quantile < 0.0 ||
            config.metrics.clearance_quantile > 1.0) {
            throw std::runtime_error("metrics.clearance_quantile must be in [0, 1]");
        }
        if (config.metrics.connectivity_grid_x == 0 || config.metrics.connectivity_grid_y == 0) {
            throw std::runtime_error("metrics.connectivity_grid_x and metrics.connectivity_grid_y must be > 0");
        }

        return config;
    }

    bool GeneratorConfigIO::save_to_file(const GeneratorConfig& config, const std::string& filepath) {
        try {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                return false;
            }
            file << to_json(config).dump(4);
            return true;
        } catch (...) {
            return false;
        }
    }

    GeneratorConfig GeneratorConfigIO::load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open generator config file: " + filepath);
        }

        try {
            nlohmann::json j;
            file >> j;
            return from_json(j);
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Failed to parse generator config JSON: " + std::string(e.what()));
        }
    }

} // namespace experiment

