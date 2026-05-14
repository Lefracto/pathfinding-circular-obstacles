#ifndef EXPERIMENT_EXPERIMENT_MODEL_H
#define EXPERIMENT_EXPERIMENT_MODEL_H

#include "../config/GeneratorConfig.h"
#include "../../geometry/Path.h"
#include "../../geometry/Scene.h"
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace experiment {

    enum class ExperimentStatus {
        Draft,
        Configured,
        Generated,
        Running,
        Completed,
        PartiallyCompleted,
        Failed
    };

    enum class SceneGenerationStatus {
        NotGenerated,
        Generated,
        Failed
    };

    enum class SceneRunStatus {
        NotRun,
        Ready,
        Running,
        Completed,
        Failed
    };

    enum class AlgorithmRunStatus {
        NotRun,
        Running,
        Success,
        NoPath,
        Failed
    };

    struct SceneMetrics {
        std::optional<double> obstacle_area_ratio = std::nullopt;
        std::optional<double> boundary_density = std::nullopt;
        std::optional<double> cell_density_cv = std::nullopt;
        std::optional<double> radii_cv = std::nullopt;
        std::optional<double> clearance = std::nullopt;
        std::optional<double> largest_component_ratio = std::nullopt;
    };

    struct PathMetrics {
        std::optional<double> path_length = std::nullopt;
        std::optional<double> relative_path_length = std::nullopt;
        std::optional<double> stability_variance = std::nullopt;
    };

    struct AlgorithmRunResult {
        std::string algorithm_id;
        AlgorithmRunStatus status = AlgorithmRunStatus::NotRun;
        bool success = false;
        double runtime_ms = 0.0;
        std::optional<double> graph_build_time_ms = std::nullopt;
        std::optional<double> graph_search_time_ms = std::nullopt;
        std::optional<double> graph_path_extraction_time_ms = std::nullopt;
        std::optional<std::size_t> graph_node_count = std::nullopt;
        std::optional<std::size_t> graph_edge_count = std::nullopt;
        std::optional<std::size_t> graph_expanded_nodes = std::nullopt;
        std::optional<geometry::Path> path = std::nullopt;
        PathMetrics path_metrics;
        nlohmann::json debug_info = nlohmann::json::object();
        std::string error_message;
    };

    struct SceneInstance {
        std::string instance_id;
        std::uint32_t seed = 0;
        SceneGenerationStatus generation_status = SceneGenerationStatus::NotGenerated;
        SceneRunStatus run_status = SceneRunStatus::NotRun;
        geometry::Scene scene_data;
        SceneMetrics scene_metrics;
        std::vector<AlgorithmRunResult> runs;
        std::string error_message;
    };

    struct Experiment {
        int version = 1;
        std::string experiment_name = "experiment";
        std::string created_at;
        std::string last_saved_at;
        std::uint32_t global_seed = 0;
        ExperimentStatus status = ExperimentStatus::Draft;
        std::string source_config_path;
        nlohmann::json metadata = nlohmann::json::object();
        GeneratorConfig generator_config_snapshot;
        std::vector<AlgorithmConfig> algorithms;
        std::vector<SceneInstance> instances;
    };

}

#endif
