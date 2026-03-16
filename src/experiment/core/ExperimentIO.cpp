#include "experiment/core/ExperimentIO.h"
#include "experiment/config/GeneratorConfigIO.h"
#include "serialization/SceneSerializer.h"

#include <fstream>
#include <stdexcept>

namespace experiment {

    namespace {

        using json = nlohmann::json;

        std::string to_string(const ExperimentStatus status) {
            switch (status) {
                case ExperimentStatus::Draft:
                    return "Draft";
                case ExperimentStatus::Configured:
                    return "Configured";
                case ExperimentStatus::Generated:
                    return "Generated";
                case ExperimentStatus::Running:
                    return "Running";
                case ExperimentStatus::Completed:
                    return "Completed";
                case ExperimentStatus::PartiallyCompleted:
                    return "PartiallyCompleted";
                case ExperimentStatus::Failed:
                    return "Failed";
            }
            throw std::runtime_error("Unknown ExperimentStatus");
        }

        ExperimentStatus experiment_status_from_string(const std::string& value) {
            if (value == "Draft") {
                return ExperimentStatus::Draft;
            }
            if (value == "Configured") {
                return ExperimentStatus::Configured;
            }
            if (value == "Generated") {
                return ExperimentStatus::Generated;
            }
            if (value == "Running") {
                return ExperimentStatus::Running;
            }
            if (value == "Completed") {
                return ExperimentStatus::Completed;
            }
            if (value == "PartiallyCompleted") {
                return ExperimentStatus::PartiallyCompleted;
            }
            if (value == "Failed") {
                return ExperimentStatus::Failed;
            }
            throw std::runtime_error("Unknown experiment status: " + value);
        }

        std::string to_string(const SceneGenerationStatus status) {
            switch (status) {
                case SceneGenerationStatus::NotGenerated:
                    return "NotGenerated";
                case SceneGenerationStatus::Generated:
                    return "Generated";
                case SceneGenerationStatus::Failed:
                    return "Failed";
            }
            throw std::runtime_error("Unknown SceneGenerationStatus");
        }

        SceneGenerationStatus scene_generation_status_from_string(const std::string& value) {
            if (value == "NotGenerated") {
                return SceneGenerationStatus::NotGenerated;
            }
            if (value == "Generated") {
                return SceneGenerationStatus::Generated;
            }
            if (value == "Failed") {
                return SceneGenerationStatus::Failed;
            }
            throw std::runtime_error("Unknown scene generation status: " + value);
        }

        std::string to_string(const SceneRunStatus status) {
            switch (status) {
                case SceneRunStatus::NotRun:
                    return "NotRun";
                case SceneRunStatus::Ready:
                    return "Ready";
                case SceneRunStatus::Running:
                    return "Running";
                case SceneRunStatus::Completed:
                    return "Completed";
                case SceneRunStatus::Failed:
                    return "Failed";
            }
            throw std::runtime_error("Unknown SceneRunStatus");
        }

        SceneRunStatus scene_run_status_from_string(const std::string& value) {
            if (value == "NotRun") {
                return SceneRunStatus::NotRun;
            }
            if (value == "Ready") {
                return SceneRunStatus::Ready;
            }
            if (value == "Running") {
                return SceneRunStatus::Running;
            }
            if (value == "Completed") {
                return SceneRunStatus::Completed;
            }
            if (value == "Failed") {
                return SceneRunStatus::Failed;
            }
            throw std::runtime_error("Unknown scene run status: " + value);
        }

        std::string to_string(const AlgorithmRunStatus status) {
            switch (status) {
                case AlgorithmRunStatus::NotRun:
                    return "NotRun";
                case AlgorithmRunStatus::Running:
                    return "Running";
                case AlgorithmRunStatus::Success:
                    return "Success";
                case AlgorithmRunStatus::NoPath:
                    return "NoPath";
                case AlgorithmRunStatus::Failed:
                    return "Failed";
            }
            throw std::runtime_error("Unknown AlgorithmRunStatus");
        }

        AlgorithmRunStatus algorithm_run_status_from_string(const std::string& value) {
            if (value == "NotRun") {
                return AlgorithmRunStatus::NotRun;
            }
            if (value == "Running") {
                return AlgorithmRunStatus::Running;
            }
            if (value == "Success") {
                return AlgorithmRunStatus::Success;
            }
            if (value == "NoPath") {
                return AlgorithmRunStatus::NoPath;
            }
            if (value == "Failed") {
                return AlgorithmRunStatus::Failed;
            }
            throw std::runtime_error("Unknown algorithm run status: " + value);
        }

        template <typename T>
        void set_optional(json& j, const char* key, const std::optional<T>& value) {
            if (value.has_value()) {
                j[key] = value.value();
            } else {
                j[key] = nullptr;
            }
        }

        std::optional<double> optional_double(const json& j, const char* key, const std::string& path) {
            if (!j.contains(key) || j[key].is_null()) {
                return std::nullopt;
            }
            if (!j[key].is_number()) {
                throw std::runtime_error(path + "." + key + " must be a number or null");
            }
            return j[key].get<double>();
        }

        json algorithm_config_to_json(const AlgorithmConfig& config) {
            return json{{"id", config.id}, {"enabled", config.enabled}, {"params", config.params}};
        }

        AlgorithmConfig algorithm_config_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            if (!j.contains("id") || !j["id"].is_string()) {
                throw std::runtime_error(path + ".id must be a string");
            }
            AlgorithmConfig config;
            config.id = j["id"].get<std::string>();
            if (j.contains("enabled")) {
                if (!j["enabled"].is_boolean()) {
                    throw std::runtime_error(path + ".enabled must be a boolean");
                }
                config.enabled = j["enabled"].get<bool>();
            }
            if (j.contains("params")) {
                if (!j["params"].is_object()) {
                    throw std::runtime_error(path + ".params must be an object");
                }
                config.params = j["params"];
            }
            return config;
        }

        json scene_metrics_to_json(const SceneMetrics& metrics) {
            json j;
            set_optional(j, "obstacle_area_ratio", metrics.obstacle_area_ratio);
            set_optional(j, "boundary_density", metrics.boundary_density);
            set_optional(j, "cell_density_cv", metrics.cell_density_cv);
            set_optional(j, "radii_cv", metrics.radii_cv);
            set_optional(j, "clearance", metrics.clearance);
            set_optional(j, "largest_component_ratio", metrics.largest_component_ratio);
            return j;
        }

        SceneMetrics scene_metrics_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            SceneMetrics metrics;
            metrics.obstacle_area_ratio = optional_double(j, "obstacle_area_ratio", path);
            metrics.boundary_density = optional_double(j, "boundary_density", path);
            metrics.cell_density_cv = optional_double(j, "cell_density_cv", path);
            metrics.radii_cv = optional_double(j, "radii_cv", path);
            metrics.clearance = optional_double(j, "clearance", path);
            metrics.largest_component_ratio = optional_double(j, "largest_component_ratio", path);
            return metrics;
        }

        json path_metrics_to_json(const PathMetrics& metrics) {
            json j;
            set_optional(j, "path_length", metrics.path_length);
            set_optional(j, "relative_path_length", metrics.relative_path_length);
            set_optional(j, "stability_variance", metrics.stability_variance);
            return j;
        }

        PathMetrics path_metrics_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            PathMetrics metrics;
            metrics.path_length = optional_double(j, "path_length", path);
            metrics.relative_path_length = optional_double(j, "relative_path_length", path);
            metrics.stability_variance = optional_double(j, "stability_variance", path);
            return metrics;
        }

        json run_result_to_json(const AlgorithmRunResult& run) {
            json j;
            j["algorithm_id"] = run.algorithm_id;
            j["status"] = to_string(run.status);
            j["success"] = run.success;
            j["runtime_ms"] = run.runtime_ms;
            j["path_found"] = run.path.has_value();
            j["path"] = run.path.has_value() ? serialization::SceneSerializer::to_json(run.path.value()) : json(nullptr);
            j["path_metrics"] = path_metrics_to_json(run.path_metrics);
            j["debug_info"] = run.debug_info;
            j["error_message"] = run.error_message;
            return j;
        }

        AlgorithmRunResult run_result_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }
            AlgorithmRunResult run;

            if (!j.contains("algorithm_id") || !j["algorithm_id"].is_string()) {
                throw std::runtime_error(path + ".algorithm_id must be a string");
            }
            run.algorithm_id = j["algorithm_id"].get<std::string>();

            if (j.contains("status")) {
                if (!j["status"].is_string()) {
                    throw std::runtime_error(path + ".status must be a string");
                }
                run.status = algorithm_run_status_from_string(j["status"].get<std::string>());
            }
            if (j.contains("success")) {
                if (!j["success"].is_boolean()) {
                    throw std::runtime_error(path + ".success must be a boolean");
                }
                run.success = j["success"].get<bool>();
            }
            if (j.contains("runtime_ms")) {
                if (!j["runtime_ms"].is_number()) {
                    throw std::runtime_error(path + ".runtime_ms must be a number");
                }
                run.runtime_ms = j["runtime_ms"].get<double>();
            }

            if (j.contains("path_found") && !j["path_found"].is_boolean()) {
                throw std::runtime_error(path + ".path_found must be a boolean");
            }

            if (j.contains("path") && !j["path"].is_null()) {
                run.path = serialization::SceneSerializer::path_from_json(j["path"]);
            }

            if (j.contains("path_metrics")) {
                run.path_metrics = path_metrics_from_json(j["path_metrics"], path + ".path_metrics");
            }
            if (j.contains("debug_info")) {
                run.debug_info = j["debug_info"];
                if (!run.debug_info.is_object()) {
                    throw std::runtime_error(path + ".debug_info must be an object");
                }
            }
            if (j.contains("error_message")) {
                if (!j["error_message"].is_string()) {
                    throw std::runtime_error(path + ".error_message must be a string");
                }
                run.error_message = j["error_message"].get<std::string>();
            }

            return run;
        }

        json scene_instance_to_json(const SceneInstance& instance) {
            json j;
            j["instance_id"] = instance.instance_id;
            j["seed"] = instance.seed;
            j["generation_status"] = to_string(instance.generation_status);
            j["run_status"] = to_string(instance.run_status);
            j["scene_data"] = serialization::SceneSerializer::to_json(instance.scene_data);
            j["scene_metrics"] = scene_metrics_to_json(instance.scene_metrics);
            j["error_message"] = instance.error_message;

            json runs = json::array();
            for (const auto& run : instance.runs) {
                runs.push_back(run_result_to_json(run));
            }
            j["runs"] = runs;
            return j;
        }

        SceneInstance scene_instance_from_json(const json& j, const std::string& path) {
            if (!j.is_object()) {
                throw std::runtime_error(path + " must be an object");
            }

            SceneInstance instance;
            if (!j.contains("instance_id") || !j["instance_id"].is_string()) {
                throw std::runtime_error(path + ".instance_id must be a string");
            }
            instance.instance_id = j["instance_id"].get<std::string>();

            if (!j.contains("seed") || !j["seed"].is_number_unsigned()) {
                throw std::runtime_error(path + ".seed must be an unsigned integer");
            }
            instance.seed = j["seed"].get<std::uint32_t>();

            if (j.contains("generation_status")) {
                if (!j["generation_status"].is_string()) {
                    throw std::runtime_error(path + ".generation_status must be a string");
                }
                instance.generation_status =
                    scene_generation_status_from_string(j["generation_status"].get<std::string>());
            }
            if (j.contains("run_status")) {
                if (!j["run_status"].is_string()) {
                    throw std::runtime_error(path + ".run_status must be a string");
                }
                instance.run_status = scene_run_status_from_string(j["run_status"].get<std::string>());
            }

            if (!j.contains("scene_data")) {
                throw std::runtime_error(path + ".scene_data is required");
            }
            instance.scene_data = serialization::SceneSerializer::from_json(j["scene_data"]);

            if (j.contains("scene_metrics")) {
                instance.scene_metrics = scene_metrics_from_json(j["scene_metrics"], path + ".scene_metrics");
            }
            if (j.contains("error_message")) {
                if (!j["error_message"].is_string()) {
                    throw std::runtime_error(path + ".error_message must be a string");
                }
                instance.error_message = j["error_message"].get<std::string>();
            }

            if (j.contains("runs")) {
                if (!j["runs"].is_array()) {
                    throw std::runtime_error(path + ".runs must be an array");
                }
                for (std::size_t i = 0; i < j["runs"].size(); ++i) {
                    instance.runs.push_back(run_result_from_json(
                        j["runs"][i], path + ".runs[" + std::to_string(i) + "]"));
                }
            }

            return instance;
        }

    } // namespace

    nlohmann::json ExperimentIO::to_json(const Experiment& experiment) {
        json j;
        j["version"] = experiment.version;
        j["experiment_name"] = experiment.experiment_name;
        j["created_at"] = experiment.created_at;
        j["last_saved_at"] = experiment.last_saved_at;
        j["global_seed"] = experiment.global_seed;
        j["status"] = to_string(experiment.status);
        j["source_config_path"] = experiment.source_config_path;
        j["generator_config_snapshot"] = GeneratorConfigIO::to_json(experiment.generator_config_snapshot);

        json algorithms = json::array();
        for (const auto& algorithm : experiment.algorithms) {
            algorithms.push_back(algorithm_config_to_json(algorithm));
        }
        j["algorithms"] = algorithms;

        json instances = json::array();
        for (const auto& instance : experiment.instances) {
            instances.push_back(scene_instance_to_json(instance));
        }
        j["instances"] = instances;

        return j;
    }

    Experiment ExperimentIO::from_json(const nlohmann::json& j) {
        if (!j.is_object()) {
            throw std::runtime_error("Experiment JSON must be an object");
        }

        Experiment experiment;

        if (j.contains("version")) {
            if (!j["version"].is_number_integer()) {
                throw std::runtime_error("version must be an integer");
            }
            experiment.version = j["version"].get<int>();
        }
        if (j.contains("experiment_name")) {
            if (!j["experiment_name"].is_string()) {
                throw std::runtime_error("experiment_name must be a string");
            }
            experiment.experiment_name = j["experiment_name"].get<std::string>();
        }
        if (j.contains("created_at")) {
            if (!j["created_at"].is_string()) {
                throw std::runtime_error("created_at must be a string");
            }
            experiment.created_at = j["created_at"].get<std::string>();
        }
        if (j.contains("last_saved_at")) {
            if (!j["last_saved_at"].is_string()) {
                throw std::runtime_error("last_saved_at must be a string");
            }
            experiment.last_saved_at = j["last_saved_at"].get<std::string>();
        }
        if (j.contains("global_seed")) {
            if (!j["global_seed"].is_number_unsigned()) {
                throw std::runtime_error("global_seed must be an unsigned integer");
            }
            experiment.global_seed = j["global_seed"].get<std::uint32_t>();
        }
        if (j.contains("status")) {
            if (!j["status"].is_string()) {
                throw std::runtime_error("status must be a string");
            }
            experiment.status = experiment_status_from_string(j["status"].get<std::string>());
        }
        if (j.contains("source_config_path")) {
            if (!j["source_config_path"].is_string()) {
                throw std::runtime_error("source_config_path must be a string");
            }
            experiment.source_config_path = j["source_config_path"].get<std::string>();
        }

        if (!j.contains("generator_config_snapshot")) {
            throw std::runtime_error("generator_config_snapshot is required");
        }
        experiment.generator_config_snapshot = GeneratorConfigIO::from_json(j["generator_config_snapshot"]);

        experiment.algorithms.clear();
        if (j.contains("algorithms")) {
            if (!j["algorithms"].is_array()) {
                throw std::runtime_error("algorithms must be an array");
            }
            for (std::size_t i = 0; i < j["algorithms"].size(); ++i) {
                experiment.algorithms.push_back(
                    algorithm_config_from_json(j["algorithms"][i], "algorithms[" + std::to_string(i) + "]"));
            }
        }

        experiment.instances.clear();
        if (j.contains("instances")) {
            if (!j["instances"].is_array()) {
                throw std::runtime_error("instances must be an array");
            }
            for (std::size_t i = 0; i < j["instances"].size(); ++i) {
                experiment.instances.push_back(
                    scene_instance_from_json(j["instances"][i], "instances[" + std::to_string(i) + "]"));
            }
        }

        return experiment;
    }

    bool ExperimentIO::save_to_file(const Experiment& experiment, const std::string& filepath) {
        try {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                return false;
            }
            file << to_json(experiment).dump(4);
            return true;
        } catch (...) {
            return false;
        }
    }

    Experiment ExperimentIO::load_from_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open experiment file: " + filepath);
        }

        try {
            nlohmann::json j;
            file >> j;
            return from_json(j);
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Failed to parse experiment JSON: " + std::string(e.what()));
        }
    }

} // namespace experiment


