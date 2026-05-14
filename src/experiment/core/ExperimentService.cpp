#include "experiment/core/ExperimentService.h"
#include "experiment/generation/SceneBatchGenerator.h"
#include "experiment/config/GeneratorConfigIO.h"
#include "experiment/metrics/SceneMetricsBatchCalculator.h"
#include "algorithms/PlannerFactory.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <limits>
#include <numeric>
#include <optional>
#include <thread>
#include <vector>

namespace experiment {

    namespace {

        std::string utc_timestamp_now() {
            const auto now = std::chrono::system_clock::now();
            const std::time_t now_time = std::chrono::system_clock::to_time_t(now);

            std::tm utc_tm{};
#ifdef _WIN32
            gmtime_s(&utc_tm, &now_time);
#else
            gmtime_r(&now_time, &utc_tm);
#endif

            char buffer[32];
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc_tm);
            return buffer;
        }

        std::size_t enabled_algorithm_count(const std::vector<AlgorithmConfig>& algorithms) {
            std::size_t enabled = 0;
            for (const auto& algorithm : algorithms) {
                if (algorithm.enabled) {
                    ++enabled;
                }
            }
            return enabled;
        }

        std::size_t resolve_worker_count(const std::size_t total_items) {
            if (total_items <= 1) {
                return total_items;
            }

            std::size_t workers = static_cast<std::size_t>(std::thread::hardware_concurrency());
            if (workers == 0) {
                workers = 4;
            }

            workers = std::max<std::size_t>(1, workers);
            workers = std::min(workers, total_items);
            return workers;
        }

        double compute_path_length(const geometry::Path& path) {
            if (path.points.size() < 2) {
                return 0.0;
            }

            double total = 0.0;
            for (std::size_t i = 0; i + 1 < path.points.size(); ++i) {
                total += path.points[i].distance(path.points[i + 1]);
            }
            return total;
        }

        std::optional<double> compute_relative_path_length(const geometry::Scene& scene, const double path_length) {
            const double baseline = scene.start.distance(scene.goal);
            if (baseline <= 1e-12) {
                return std::nullopt;
            }
            return path_length / baseline;
        }

        AlgorithmRunResult execute_algorithm_run(const AlgorithmConfig& algorithm_config,
                                                 const SceneInstance& instance) {
            AlgorithmRunResult result;
            result.algorithm_id = algorithm_config.id;
            result.status = AlgorithmRunStatus::Running;
            result.success = false;
            result.runtime_ms = 0.0;
            result.debug_info = nlohmann::json::object();
            result.debug_info["params"] = algorithm_config.params;
            result.debug_info["scene_seed"] = instance.seed;
            result.debug_info["timing_clock"] = "steady_clock";

            try {
                nlohmann::json runtime_params = algorithm_config.params;
                if (algorithm_config.id == "rrt" && !runtime_params.contains("seed")) {
                    runtime_params["seed"] = instance.seed;
                }

                auto planner = algorithms::PlannerFactory::create(algorithm_config.id, runtime_params);
                result.debug_info["resolved_params"] = runtime_params;
                result.debug_info["planner_name"] = planner->name();

                const algorithms::BenchmarkResult benchmark = planner->plan(instance.scene_data);
                result.runtime_ms = benchmark.runtime_ms;
                result.graph_build_time_ms = benchmark.graph_build_time_ms;
                result.graph_search_time_ms = benchmark.graph_search_time_ms;
                result.graph_path_extraction_time_ms = benchmark.graph_path_extraction_time_ms;
                result.graph_node_count = benchmark.graph_node_count;
                result.graph_edge_count = benchmark.graph_edge_count;
                result.graph_expanded_nodes = benchmark.graph_expanded_nodes;

                if (benchmark.graph_build_time_ms.has_value() ||
                    benchmark.graph_search_time_ms.has_value()) {
                    result.debug_info["graph_timing"] = nlohmann::json::object();
                    result.debug_info["graph_timing"]["build_time_ms"] =
                        benchmark.graph_build_time_ms.has_value()
                            ? nlohmann::json(benchmark.graph_build_time_ms.value())
                            : nlohmann::json(nullptr);
                    result.debug_info["graph_timing"]["search_time_ms"] =
                        benchmark.graph_search_time_ms.has_value()
                            ? nlohmann::json(benchmark.graph_search_time_ms.value())
                            : nlohmann::json(nullptr);
                    result.debug_info["graph_timing"]["path_extraction_time_ms"] =
                        benchmark.graph_path_extraction_time_ms.has_value()
                            ? nlohmann::json(benchmark.graph_path_extraction_time_ms.value())
                            : nlohmann::json(nullptr);
                    result.debug_info["graph_timing"]["node_count"] =
                        benchmark.graph_node_count.has_value()
                            ? nlohmann::json(benchmark.graph_node_count.value())
                            : nlohmann::json(nullptr);
                    result.debug_info["graph_timing"]["edge_count"] =
                        benchmark.graph_edge_count.has_value()
                            ? nlohmann::json(benchmark.graph_edge_count.value())
                            : nlohmann::json(nullptr);
                    result.debug_info["graph_timing"]["expanded_nodes"] =
                        benchmark.graph_expanded_nodes.has_value()
                            ? nlohmann::json(benchmark.graph_expanded_nodes.value())
                            : nlohmann::json(nullptr);
                }

                if (!benchmark.path.points.empty()) {
                    result.path = benchmark.path;
                    result.success = true;
                    result.status = AlgorithmRunStatus::Success;

                    const double path_length = benchmark.exact_path_length.value_or(
                        compute_path_length(benchmark.path));
                    result.path_metrics.path_length = path_length;
                    result.path_metrics.relative_path_length =
                        compute_relative_path_length(instance.scene_data, path_length);
                    result.debug_info["path_length_source"] = benchmark.exact_path_length.has_value()
                        ? "planner_exact"
                        : "polyline_fallback";
                } else {
                    result.success = false;
                    result.status = AlgorithmRunStatus::NoPath;
                }
            } catch (const std::exception& e) {
                result.success = false;
                result.status = AlgorithmRunStatus::Failed;
                result.error_message = e.what();
            } catch (...) {
                result.success = false;
                result.status = AlgorithmRunStatus::Failed;
                result.error_message = "Unknown algorithm error";
            }

            return result;
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
            return "Unknown";
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
            return "Unknown";
        }

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
            return "Unknown";
        }

        void run_enabled_algorithms_for_instance(SceneInstance& instance,
                                                 const std::vector<AlgorithmConfig>& algorithms,
                                                 std::atomic<std::size_t>* completed_runs) {
            instance.runs.clear();

            if (instance.generation_status != SceneGenerationStatus::Generated) {
                instance.run_status = SceneRunStatus::Failed;
                return;
            }

            instance.run_status = SceneRunStatus::Running;
            bool has_failed_algorithm = false;
            bool has_non_failed_algorithm = false;

            for (const auto& algorithm : algorithms) {
                if (!algorithm.enabled) {
                    continue;
                }

                AlgorithmRunResult run_result = execute_algorithm_run(algorithm, instance);
                if (run_result.status == AlgorithmRunStatus::Failed) {
                    has_failed_algorithm = true;
                } else {
                    has_non_failed_algorithm = true;
                }

                instance.runs.push_back(std::move(run_result));
                if (completed_runs != nullptr) {
                    completed_runs->fetch_add(1);
                }
            }

            if (has_failed_algorithm || !has_non_failed_algorithm) {
                instance.run_status = SceneRunStatus::Failed;
            } else {
                instance.run_status = SceneRunStatus::Completed;
            }
        }

        void run_enabled_algorithms_for_instance_parallel(SceneInstance& instance,
                                                          const std::vector<AlgorithmConfig>& algorithms,
                                                          std::atomic<std::size_t>* completed_runs) {
            instance.runs.clear();

            if (instance.generation_status != SceneGenerationStatus::Generated) {
                instance.run_status = SceneRunStatus::Failed;
                return;
            }

            std::vector<const AlgorithmConfig*> enabled_algorithms;
            enabled_algorithms.reserve(algorithms.size());
            for (const auto& algorithm : algorithms) {
                if (algorithm.enabled) {
                    enabled_algorithms.push_back(&algorithm);
                }
            }

            if (enabled_algorithms.empty()) {
                instance.run_status = SceneRunStatus::Failed;
                return;
            }

            instance.run_status = SceneRunStatus::Running;
            std::vector<AlgorithmRunResult> run_results(enabled_algorithms.size());

            std::atomic<std::size_t> next_index{0};
            const std::size_t worker_count = resolve_worker_count(enabled_algorithms.size());
            std::vector<std::thread> workers;
            workers.reserve(worker_count);

            for (std::size_t worker = 0; worker < worker_count; ++worker) {
                workers.emplace_back([&instance,
                                      &enabled_algorithms,
                                      &run_results,
                                      &next_index,
                                      completed_runs]() {
                    while (true) {
                        const std::size_t index = next_index.fetch_add(1);
                        if (index >= enabled_algorithms.size()) {
                            return;
                        }

                        run_results[index] =
                            execute_algorithm_run(*enabled_algorithms[index], instance);
                        if (completed_runs != nullptr) {
                            completed_runs->fetch_add(1);
                        }
                    }
                });
            }

            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }

            bool has_failed_algorithm = false;
            bool has_non_failed_algorithm = false;
            for (auto& run_result : run_results) {
                if (run_result.status == AlgorithmRunStatus::Failed) {
                    has_failed_algorithm = true;
                } else {
                    has_non_failed_algorithm = true;
                }
                instance.runs.push_back(std::move(run_result));
            }

            if (has_failed_algorithm || !has_non_failed_algorithm) {
                instance.run_status = SceneRunStatus::Failed;
            } else {
                instance.run_status = SceneRunStatus::Completed;
            }
        }

        void recalculate_experiment_status_after_runs(Experiment& experiment) {
            std::size_t generated_scene_count = 0;
            std::size_t completed_scene_count = 0;
            std::size_t failed_scene_count = 0;

            for (const auto& instance : experiment.instances) {
                if (instance.generation_status != SceneGenerationStatus::Generated) {
                    continue;
                }

                ++generated_scene_count;
                if (instance.run_status == SceneRunStatus::Completed) {
                    ++completed_scene_count;
                } else if (instance.run_status == SceneRunStatus::Failed) {
                    ++failed_scene_count;
                }
            }

            if (generated_scene_count == 0) {
                experiment.status = ExperimentStatus::Failed;
            } else if (completed_scene_count == generated_scene_count) {
                experiment.status = ExperimentStatus::Completed;
            } else if (failed_scene_count == generated_scene_count) {
                experiment.status = ExperimentStatus::Failed;
            } else if (completed_scene_count > 0 || failed_scene_count > 0) {
                experiment.status = ExperimentStatus::PartiallyCompleted;
            } else {
                experiment.status = ExperimentStatus::Generated;
            }
        }

    } // namespace

    Experiment ExperimentService::create_empty_experiment(const std::string& experiment_name) {
        Experiment experiment;
        experiment.version = 1;
        experiment.experiment_name = experiment_name.empty() ? "untitled_experiment" : experiment_name;
        experiment.created_at = utc_timestamp_now();
        experiment.last_saved_at = experiment.created_at;
        experiment.status = ExperimentStatus::Draft;
        experiment.global_seed = 0;
        return experiment;
    }

    Experiment ExperimentService::create_experiment(const GeneratorConfig& config,
                                                    const std::string& experiment_name,
                                                    const std::string& source_config_path) {
        Experiment experiment;
        experiment.version = 1;
        experiment.experiment_name = experiment_name.empty() ? "experiment" : experiment_name;
        experiment.created_at = utc_timestamp_now();
        experiment.last_saved_at = experiment.created_at;
        experiment.global_seed = config.global_seed;
        experiment.status = ExperimentStatus::Configured;
        experiment.source_config_path = source_config_path;
        experiment.generator_config_snapshot = config;
        experiment.algorithms = config.algorithms;
        return experiment;
    }

    void ExperimentService::generate_instances(Experiment& experiment,
                                               std::atomic<std::size_t>* generated_counter) {
        experiment.instances =
            SceneBatchGenerator::generate_instances(experiment.generator_config_snapshot, generated_counter);

        if (experiment.generator_config_snapshot.metrics.auto_compute_on_generation) {
            const SceneMetricsBatchResult metrics_result = SceneMetricsBatchCalculator::compute(
                experiment.instances,
                experiment.generator_config_snapshot.metrics);
            const std::size_t count =
                std::min(experiment.instances.size(), metrics_result.metrics.size());
            for (std::size_t i = 0; i < count; ++i) {
                experiment.instances[i].scene_metrics = metrics_result.metrics[i];
            }
        }

        std::size_t generated_count = 0;
        for (auto& instance : experiment.instances) {
            instance.runs.clear();

            if (instance.generation_status == SceneGenerationStatus::Generated) {
                ++generated_count;
                instance.run_status = SceneRunStatus::Ready;
            } else {
                instance.run_status = SceneRunStatus::Failed;
            }

            for (const auto& algorithm : experiment.algorithms) {
                if (!algorithm.enabled) {
                    continue;
                }

                AlgorithmRunResult run;
                run.algorithm_id = algorithm.id;
                run.status = AlgorithmRunStatus::NotRun;
                run.success = false;
                instance.runs.push_back(run);
            }
        }

        if (experiment.instances.empty() || generated_count == 0) {
            experiment.status = ExperimentStatus::Failed;
        } else if (generated_count == experiment.instances.size()) {
            experiment.status = ExperimentStatus::Generated;
        } else {
            experiment.status = ExperimentStatus::PartiallyCompleted;
        }

        experiment.last_saved_at = utc_timestamp_now();
    }

    void ExperimentService::generate_instances_for_indices(Experiment& experiment,
                                                           const std::vector<std::size_t>& scene_indices,
                                                           std::atomic<std::size_t>* generated_counter) {
        experiment.instances = SceneBatchGenerator::generate_instances_for_indices(
            experiment.generator_config_snapshot,
            scene_indices,
            generated_counter);

        if (experiment.generator_config_snapshot.metrics.auto_compute_on_generation) {
            const SceneMetricsBatchResult metrics_result = SceneMetricsBatchCalculator::compute(
                experiment.instances,
                experiment.generator_config_snapshot.metrics);
            const std::size_t count =
                std::min(experiment.instances.size(), metrics_result.metrics.size());
            for (std::size_t i = 0; i < count; ++i) {
                experiment.instances[i].scene_metrics = metrics_result.metrics[i];
            }
        }

        std::size_t generated_count = 0;
        for (auto& instance : experiment.instances) {
            instance.runs.clear();

            if (instance.generation_status == SceneGenerationStatus::Generated) {
                ++generated_count;
                instance.run_status = SceneRunStatus::Ready;
            } else {
                instance.run_status = SceneRunStatus::Failed;
            }

            for (const auto& algorithm : experiment.algorithms) {
                if (!algorithm.enabled) {
                    continue;
                }

                AlgorithmRunResult run;
                run.algorithm_id = algorithm.id;
                run.status = AlgorithmRunStatus::NotRun;
                run.success = false;
                instance.runs.push_back(run);
            }
        }

        if (experiment.instances.empty() || generated_count == 0) {
            experiment.status = ExperimentStatus::Failed;
        } else if (generated_count == experiment.instances.size()) {
            experiment.status = ExperimentStatus::Generated;
        } else {
            experiment.status = ExperimentStatus::PartiallyCompleted;
        }

        experiment.last_saved_at = utc_timestamp_now();
    }

    std::size_t ExperimentService::estimate_total_algorithm_runs(const Experiment& experiment) {
        const std::size_t enabled_algorithms = enabled_algorithm_count(experiment.algorithms);
        if (enabled_algorithms == 0 || experiment.instances.empty()) {
            return 0;
        }

        std::size_t generated_scenes = 0;
        for (const auto& instance : experiment.instances) {
            if (instance.generation_status == SceneGenerationStatus::Generated) {
                ++generated_scenes;
            }
        }
        return generated_scenes * enabled_algorithms;
    }

    std::size_t ExperimentService::estimate_total_algorithm_runs_for_scene(const Experiment& experiment,
                                                                           const std::size_t scene_index) {
        if (scene_index >= experiment.instances.size()) {
            return 0;
        }

        const auto& instance = experiment.instances[scene_index];
        if (instance.generation_status != SceneGenerationStatus::Generated) {
            return 0;
        }

        return enabled_algorithm_count(experiment.algorithms);
    }

    void ExperimentService::run_algorithms(Experiment& experiment, std::atomic<std::size_t>* completed_runs) {
        if (completed_runs != nullptr) {
            completed_runs->store(0);
        }

        const std::size_t enabled_algorithms = enabled_algorithm_count(experiment.algorithms);
        if (enabled_algorithms == 0) {
            experiment.last_saved_at = utc_timestamp_now();
            return;
        }

        experiment.status = ExperimentStatus::Running;

        std::vector<std::size_t> generated_scene_indices;
        generated_scene_indices.reserve(experiment.instances.size());
        for (std::size_t i = 0; i < experiment.instances.size(); ++i) {
            if (experiment.instances[i].generation_status == SceneGenerationStatus::Generated) {
                generated_scene_indices.push_back(i);
            } else {
                experiment.instances[i].run_status = SceneRunStatus::Failed;
            }
        }

        const std::size_t worker_count = resolve_worker_count(generated_scene_indices.size());
        if (worker_count <= 1) {
            for (const std::size_t scene_index : generated_scene_indices) {
                run_enabled_algorithms_for_instance(
                    experiment.instances[scene_index],
                    experiment.algorithms,
                    completed_runs);
            }
        } else {
            std::atomic<std::size_t> next_scene_index{0};
            std::vector<std::thread> workers;
            workers.reserve(worker_count);

            for (std::size_t worker = 0; worker < worker_count; ++worker) {
                workers.emplace_back([&experiment,
                                      &generated_scene_indices,
                                      &next_scene_index,
                                      completed_runs]() {
                    while (true) {
                        const std::size_t queue_index = next_scene_index.fetch_add(1);
                        if (queue_index >= generated_scene_indices.size()) {
                            return;
                        }

                        const std::size_t scene_index = generated_scene_indices[queue_index];
                        run_enabled_algorithms_for_instance(
                            experiment.instances[scene_index],
                            experiment.algorithms,
                            completed_runs);
                    }
                });
            }

            for (auto& worker : workers) {
                if (worker.joinable()) {
                    worker.join();
                }
            }
        }

        recalculate_experiment_status_after_runs(experiment);
        experiment.last_saved_at = utc_timestamp_now();
    }

    void ExperimentService::run_algorithms_for_scene(Experiment& experiment,
                                                     const std::size_t scene_index,
                                                     std::atomic<std::size_t>* completed_runs) {
        if (completed_runs != nullptr) {
            completed_runs->store(0);
        }
        if (scene_index >= experiment.instances.size()) {
            return;
        }
        if (enabled_algorithm_count(experiment.algorithms) == 0) {
            return;
        }

        experiment.status = ExperimentStatus::Running;

        run_enabled_algorithms_for_instance_parallel(
            experiment.instances[scene_index],
            experiment.algorithms,
            completed_runs);

        recalculate_experiment_status_after_runs(experiment);
        experiment.last_saved_at = utc_timestamp_now();
    }

    nlohmann::json ExperimentService::build_algorithm_report_json(const Experiment& experiment) {
        nlohmann::json report = nlohmann::json::object();
        report["version"] = 1;
        report["exported_at"] = utc_timestamp_now();
        report["experiment_name"] = experiment.experiment_name;
        report["experiment_status"] = to_string(experiment.status);
        report["created_at"] = experiment.created_at;
        report["last_saved_at"] = experiment.last_saved_at;
        report["global_seed"] = experiment.global_seed;
        report["source_config_path"] = experiment.source_config_path;
        report["metadata"] = experiment.metadata.is_object() ? experiment.metadata : nlohmann::json::object();
        report["timing_clock"] = "steady_clock";
        report["generator_config_snapshot"] = GeneratorConfigIO::to_json(experiment.generator_config_snapshot);

        nlohmann::json algorithms = nlohmann::json::array();
        for (const auto& algorithm : experiment.algorithms) {
            algorithms.push_back(nlohmann::json{{"id", algorithm.id}, {"enabled", algorithm.enabled}, {"params", algorithm.params}});
        }
        report["algorithms"] = std::move(algorithms);

        std::size_t enabled_algorithms = 0;
        for (const auto& algorithm : experiment.algorithms) {
            if (algorithm.enabled) {
                ++enabled_algorithms;
            }
        }
        report["scene_count"] = experiment.instances.size();
        report["enabled_algorithm_count"] = enabled_algorithms;

        nlohmann::json runs = nlohmann::json::array();
        std::size_t total_runs = 0;
        std::size_t success_runs = 0;
        std::size_t no_path_runs = 0;
        std::size_t failed_runs = 0;
        double runtime_sum = 0.0;
        std::size_t runtime_count = 0;
        double graph_build_time_sum = 0.0;
        std::size_t graph_build_time_count = 0;
        double graph_search_time_sum = 0.0;
        std::size_t graph_search_time_count = 0;
        double path_length_sum = 0.0;
        std::size_t path_length_count = 0;

        for (const auto& instance : experiment.instances) {
            for (const auto& run : instance.runs) {
                ++total_runs;

                if (run.status == AlgorithmRunStatus::Success) {
                    ++success_runs;
                } else if (run.status == AlgorithmRunStatus::NoPath) {
                    ++no_path_runs;
                } else if (run.status == AlgorithmRunStatus::Failed) {
                    ++failed_runs;
                }

                if (run.runtime_ms >= 0.0) {
                    runtime_sum += run.runtime_ms;
                    ++runtime_count;
                }
                if (run.graph_build_time_ms.has_value()) {
                    graph_build_time_sum += run.graph_build_time_ms.value();
                    ++graph_build_time_count;
                }
                if (run.graph_search_time_ms.has_value()) {
                    graph_search_time_sum += run.graph_search_time_ms.value();
                    ++graph_search_time_count;
                }
                if (run.path_metrics.path_length.has_value()) {
                    path_length_sum += run.path_metrics.path_length.value();
                    ++path_length_count;
                }

                nlohmann::json row = nlohmann::json::object();
                row["scene_id"] = instance.instance_id;
                row["scene_seed"] = instance.seed;
                row["scene_run_status"] = to_string(instance.run_status);
                row["algorithm_id"] = run.algorithm_id;
                row["algorithm_status"] = to_string(run.status);
                row["success"] = run.success;
                row["runtime_ms"] = run.runtime_ms;
                row["graph_build_time_ms"] = run.graph_build_time_ms.has_value()
                                                 ? nlohmann::json(run.graph_build_time_ms.value())
                                                 : nlohmann::json(nullptr);
                row["graph_search_time_ms"] = run.graph_search_time_ms.has_value()
                                                  ? nlohmann::json(run.graph_search_time_ms.value())
                                                  : nlohmann::json(nullptr);
                row["graph_path_extraction_time_ms"] = run.graph_path_extraction_time_ms.has_value()
                                                           ? nlohmann::json(run.graph_path_extraction_time_ms.value())
                                                           : nlohmann::json(nullptr);
                row["graph_node_count"] = run.graph_node_count.has_value()
                                              ? nlohmann::json(run.graph_node_count.value())
                                              : nlohmann::json(nullptr);
                row["graph_edge_count"] = run.graph_edge_count.has_value()
                                              ? nlohmann::json(run.graph_edge_count.value())
                                              : nlohmann::json(nullptr);
                row["graph_expanded_nodes"] = run.graph_expanded_nodes.has_value()
                                                  ? nlohmann::json(run.graph_expanded_nodes.value())
                                                  : nlohmann::json(nullptr);
                row["path_found"] = run.path.has_value();
                row["algorithm_debug_info"] = run.debug_info;
                row["path_length"] = run.path_metrics.path_length.has_value()
                                         ? nlohmann::json(run.path_metrics.path_length.value())
                                         : nlohmann::json(nullptr);
                row["relative_path_length"] = run.path_metrics.relative_path_length.has_value()
                                                  ? nlohmann::json(run.path_metrics.relative_path_length.value())
                                                  : nlohmann::json(nullptr);
                row["obstacle_count"] = instance.scene_data.obstacles.size();
                row["scene_width"] = instance.scene_data.width;
                row["scene_height"] = instance.scene_data.height;

                row["scene_metrics"] = nlohmann::json::object();
                row["scene_metrics"]["obstacle_area_ratio"] = instance.scene_metrics.obstacle_area_ratio.has_value()
                    ? nlohmann::json(instance.scene_metrics.obstacle_area_ratio.value())
                    : nlohmann::json(nullptr);
                row["scene_metrics"]["boundary_density"] = instance.scene_metrics.boundary_density.has_value()
                    ? nlohmann::json(instance.scene_metrics.boundary_density.value())
                    : nlohmann::json(nullptr);
                row["scene_metrics"]["cell_density_cv"] = instance.scene_metrics.cell_density_cv.has_value()
                    ? nlohmann::json(instance.scene_metrics.cell_density_cv.value())
                    : nlohmann::json(nullptr);
                row["scene_metrics"]["radii_cv"] = instance.scene_metrics.radii_cv.has_value()
                    ? nlohmann::json(instance.scene_metrics.radii_cv.value())
                    : nlohmann::json(nullptr);
                row["scene_metrics"]["clearance"] = instance.scene_metrics.clearance.has_value()
                    ? nlohmann::json(instance.scene_metrics.clearance.value())
                    : nlohmann::json(nullptr);
                row["scene_metrics"]["largest_component_ratio"] =
                    instance.scene_metrics.largest_component_ratio.has_value()
                        ? nlohmann::json(instance.scene_metrics.largest_component_ratio.value())
                        : nlohmann::json(nullptr);

                runs.push_back(std::move(row));
            }
        }

        report["summary"] = nlohmann::json::object();
        report["summary"]["total_runs"] = total_runs;
        report["summary"]["success_runs"] = success_runs;
        report["summary"]["no_path_runs"] = no_path_runs;
        report["summary"]["failed_runs"] = failed_runs;
        report["summary"]["avg_runtime_ms"] = runtime_count > 0
            ? nlohmann::json(runtime_sum / static_cast<double>(runtime_count))
            : nlohmann::json(nullptr);
        report["summary"]["avg_graph_build_time_ms"] = graph_build_time_count > 0
            ? nlohmann::json(graph_build_time_sum / static_cast<double>(graph_build_time_count))
            : nlohmann::json(nullptr);
        report["summary"]["avg_graph_search_time_ms"] = graph_search_time_count > 0
            ? nlohmann::json(graph_search_time_sum / static_cast<double>(graph_search_time_count))
            : nlohmann::json(nullptr);
        report["summary"]["avg_path_length"] = path_length_count > 0
            ? nlohmann::json(path_length_sum / static_cast<double>(path_length_count))
            : nlohmann::json(nullptr);

        report["runs"] = std::move(runs);
        return report;
    }

    bool ExperimentService::save_algorithm_report_json(const Experiment& experiment, const std::string& filepath) {
        try {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                return false;
            }
            file << build_algorithm_report_json(experiment).dump(4);
            return true;
        } catch (...) {
            return false;
        }
    }

}
