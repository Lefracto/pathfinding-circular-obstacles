//
// Created by USER on 26/12/2025.
//

#include "../include/visualization/SceneVisualizer.h"
#include "../include/experiment/core/ExperimentIO.h"
#include "../include/experiment/core/ExperimentService.h"
#include "../include/experiment/config/GeneratorConfigIO.h"
#include "../include/experiment/metrics/SceneMetricsBatchCalculator.h"
#include "../include/algorithms/GridGraphBuilder.h"
#include <imgui.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shobjidl.h>
#endif

namespace {

    std::optional<std::string> show_save_screenshot_dialog() {
#ifdef _WIN32
        char file_path[MAX_PATH] = "scene_screenshot.png";
        char filter[] = "PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";

        OPENFILENAMEA dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = file_path;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        dialog.lpstrDefExt = "png";

        if (!GetSaveFileNameA(&dialog)) {
            return std::nullopt;
        }

        std::string selected_path(file_path);
        auto to_lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        if (selected_path.size() < 4 ||
            to_lower(selected_path[selected_path.size() - 4]) != '.' ||
            to_lower(selected_path[selected_path.size() - 3]) != 'p' ||
            to_lower(selected_path[selected_path.size() - 2]) != 'n' ||
            to_lower(selected_path[selected_path.size() - 1]) != 'g') {
            selected_path += ".png";
        }

        return selected_path;
#else
        return std::nullopt;
#endif
    }

    std::optional<std::string> show_open_json_dialog(const char* title) {
#ifdef _WIN32
        char file_path[MAX_PATH] = "";
        char filter[] = "JSON File (*.json)\0*.json\0All Files (*.*)\0*.*\0";

        OPENFILENAMEA dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = file_path;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        dialog.lpstrTitle = title;
        dialog.lpstrDefExt = "json";

        if (!GetOpenFileNameA(&dialog)) {
            return std::nullopt;
        }

        return std::string(file_path);
#else
        (void)title;
        return std::nullopt;
#endif
    }

    std::optional<std::string> show_save_json_dialog(const char* title, const char* default_name) {
#ifdef _WIN32
        char file_path[MAX_PATH] = "";
        if (default_name != nullptr) {
            std::snprintf(file_path, MAX_PATH, "%s", default_name);
        }

        char filter[] = "JSON File (*.json)\0*.json\0All Files (*.*)\0*.*\0";

        OPENFILENAMEA dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = filter;
        dialog.lpstrFile = file_path;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        dialog.lpstrTitle = title;
        dialog.lpstrDefExt = "json";

        if (!GetSaveFileNameA(&dialog)) {
            return std::nullopt;
        }

        std::string selected_path(file_path);
        auto to_lower = [](unsigned char c) { return static_cast<char>(std::tolower(c)); };
        if (selected_path.size() < 5 ||
            to_lower(selected_path[selected_path.size() - 5]) != '.' ||
            to_lower(selected_path[selected_path.size() - 4]) != 'j' ||
            to_lower(selected_path[selected_path.size() - 3]) != 's' ||
            to_lower(selected_path[selected_path.size() - 2]) != 'o' ||
            to_lower(selected_path[selected_path.size() - 1]) != 'n') {
            selected_path += ".json";
        }

        return selected_path;
#else
        (void)title;
        (void)default_name;
        return std::nullopt;
#endif
    }

    std::optional<std::string> show_select_folder_dialog(const char* title) {
#ifdef _WIN32
        const HRESULT init_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        const bool should_uninitialize = SUCCEEDED(init_result);

        IFileDialog* dialog = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&dialog));
        if (FAILED(hr) || dialog == nullptr) {
            if (should_uninitialize) {
                CoUninitialize();
            }
            return std::nullopt;
        }

        DWORD options = 0;
        if (SUCCEEDED(dialog->GetOptions(&options))) {
            dialog->SetOptions(options | FOS_PICKFOLDERS | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM);
        }

        std::wstring wide_title;
        if (title != nullptr) {
            const std::string narrow_title(title);
            wide_title.assign(narrow_title.begin(), narrow_title.end());
            dialog->SetTitle(wide_title.c_str());
        }

        hr = dialog->Show(nullptr);
        if (FAILED(hr)) {
            dialog->Release();
            if (should_uninitialize) {
                CoUninitialize();
            }
            return std::nullopt;
        }

        IShellItem* result_item = nullptr;
        hr = dialog->GetResult(&result_item);
        if (FAILED(hr) || result_item == nullptr) {
            dialog->Release();
            if (should_uninitialize) {
                CoUninitialize();
            }
            return std::nullopt;
        }

        PWSTR folder_path = nullptr;
        hr = result_item->GetDisplayName(SIGDN_FILESYSPATH, &folder_path);
        std::optional<std::string> selected_path = std::nullopt;
        if (SUCCEEDED(hr) && folder_path != nullptr) {
            selected_path = std::filesystem::path(folder_path).string();
            CoTaskMemFree(folder_path);
        }

        result_item->Release();
        dialog->Release();
        if (should_uninitialize) {
            CoUninitialize();
        }

        return selected_path;
#else
        (void)title;
        return std::nullopt;
#endif
    }

    struct BatchStudyProgressState {
        std::atomic<std::size_t> total_configs{0};
        std::atomic<std::size_t> processed_configs{0};
        std::atomic<std::size_t> total_scene_count{0};
        std::atomic<std::size_t> processed_scene_count{0};
        std::atomic<std::size_t> current_scene_total{0};
        std::atomic<std::size_t> current_scene_completed{0};
        std::atomic<std::size_t> current_algorithm_total{0};
        std::atomic<std::size_t> current_algorithm_completed{0};
        std::mutex text_mutex;
        std::string selected_folder;
        std::string output_folder;
        std::string current_config_file;
        std::string current_stage = "Idle";
        std::string last_saved_file;
        std::string note;
    };

    struct BatchStudyProgressSnapshot {
        std::size_t total_configs = 0;
        std::size_t processed_configs = 0;
        std::size_t total_scene_count = 0;
        std::size_t processed_scene_count = 0;
        std::size_t current_scene_total = 0;
        std::size_t current_scene_completed = 0;
        std::size_t current_algorithm_total = 0;
        std::size_t current_algorithm_completed = 0;
        std::string selected_folder;
        std::string output_folder;
        std::string current_config_file;
        std::string current_stage;
        std::string last_saved_file;
        std::string note;
    };

    struct BatchStudyResult {
        std::filesystem::path output_directory;
        std::filesystem::path last_experiment_path;
        std::optional<experiment::Experiment> last_experiment;
        std::size_t total_valid_configs = 0;
        std::size_t saved_experiment_count = 0;
        std::size_t failed_config_count = 0;
        std::size_t skipped_json_count = 0;
    };

    void set_batch_study_text_state(const std::shared_ptr<BatchStudyProgressState>& progress,
                                    const std::string& stage,
                                    const std::string& current_config_file,
                                    const std::string& note = "") {
        if (progress == nullptr) {
            return;
        }

        std::lock_guard<std::mutex> lock(progress->text_mutex);
        progress->current_stage = stage;
        progress->current_config_file = current_config_file;
        progress->note = note;
    }

    BatchStudyProgressSnapshot snapshot_batch_study_progress(
        const std::shared_ptr<BatchStudyProgressState>& progress) {
        BatchStudyProgressSnapshot snapshot;
        if (progress == nullptr) {
            return snapshot;
        }

        snapshot.total_configs = progress->total_configs.load();
        snapshot.processed_configs = progress->processed_configs.load();
        snapshot.total_scene_count = progress->total_scene_count.load();
        snapshot.processed_scene_count = progress->processed_scene_count.load();
        snapshot.current_scene_total = progress->current_scene_total.load();
        snapshot.current_scene_completed = progress->current_scene_completed.load();
        snapshot.current_algorithm_total = progress->current_algorithm_total.load();
        snapshot.current_algorithm_completed = progress->current_algorithm_completed.load();

        std::lock_guard<std::mutex> lock(progress->text_mutex);
        snapshot.selected_folder = progress->selected_folder;
        snapshot.output_folder = progress->output_folder;
        snapshot.current_config_file = progress->current_config_file;
        snapshot.current_stage = progress->current_stage;
        snapshot.last_saved_file = progress->last_saved_file;
        snapshot.note = progress->note;
        return snapshot;
    }

    std::string lowercase_ascii(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    BatchStudyResult run_config_folder_batch(const std::filesystem::path& folder_path,
                                             const std::shared_ptr<BatchStudyProgressState>& progress) {
        BatchStudyResult result;

        if (progress == nullptr) {
            throw std::runtime_error("Batch progress state is not initialized");
        }

        progress->total_configs.store(0);
        progress->processed_configs.store(0);
        progress->total_scene_count.store(0);
        progress->processed_scene_count.store(0);
        progress->current_scene_total.store(0);
        progress->current_scene_completed.store(0);
        progress->current_algorithm_total.store(0);
        progress->current_algorithm_completed.store(0);

        {
            std::lock_guard<std::mutex> lock(progress->text_mutex);
            progress->selected_folder = folder_path.string();
            progress->output_folder.clear();
            progress->current_config_file.clear();
            progress->current_stage = "Scanning Folder";
            progress->last_saved_file.clear();
            progress->note.clear();
        }

        if (!std::filesystem::exists(folder_path) || !std::filesystem::is_directory(folder_path)) {
            throw std::runtime_error("Selected path is not a directory");
        }

        std::vector<std::filesystem::path> json_files;
        for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            if (lowercase_ascii(entry.path().extension().string()) != ".json") {
                continue;
            }

            json_files.push_back(entry.path());
        }

        std::sort(json_files.begin(), json_files.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.filename().string() < rhs.filename().string();
        });

        struct ConfigEntry {
            std::filesystem::path path;
            experiment::GeneratorConfig config;
        };

        std::vector<ConfigEntry> configs;
        configs.reserve(json_files.size());

        std::size_t total_scene_count = 0;
        for (const auto& json_path : json_files) {
            try {
                const auto config = experiment::GeneratorConfigIO::load_from_file(json_path.string());
                total_scene_count += config.instance_count;
                configs.push_back({json_path, config});
            } catch (...) {
                ++result.skipped_json_count;
            }
        }

        if (configs.empty()) {
            throw std::runtime_error("No valid generator configs found in selected folder");
        }

        result.total_valid_configs = configs.size();
        progress->total_configs.store(result.total_valid_configs);
        progress->total_scene_count.store(total_scene_count);

        result.output_directory = folder_path / "experiment_runs";
        std::filesystem::create_directories(result.output_directory);

        {
            std::lock_guard<std::mutex> lock(progress->text_mutex);
            progress->output_folder = result.output_directory.string();
            if (result.skipped_json_count > 0) {
                progress->note = "Skipped " + std::to_string(result.skipped_json_count) +
                                 " non-generator JSON file(s).";
            }
        }

        for (const auto& entry : configs) {
            const std::string config_file = entry.path.filename().string();

            try {
                const std::string experiment_name =
                    entry.config.name.empty() ? entry.path.stem().string() : entry.config.name;
                experiment::Experiment experiment =
                    experiment::ExperimentService::create_experiment(entry.config,
                                                                    experiment_name,
                                                                    entry.path.string());

                progress->current_scene_total.store(entry.config.instance_count);
                progress->current_scene_completed.store(0);
                progress->current_algorithm_total.store(0);
                progress->current_algorithm_completed.store(0);

                set_batch_study_text_state(progress, "Generating Scenes", config_file);
                experiment::ExperimentService::generate_instances(
                    experiment,
                    &progress->current_scene_completed);
                progress->current_scene_completed.store(entry.config.instance_count);

                if (!entry.config.metrics.auto_compute_on_generation) {
                    set_batch_study_text_state(progress, "Computing Metrics", config_file);
                    progress->current_scene_completed.store(0);
                    const experiment::SceneMetricsBatchResult metrics_result =
                        experiment::SceneMetricsBatchCalculator::compute(
                            experiment.instances,
                            entry.config.metrics,
                            &progress->current_scene_completed);

                    const std::size_t metrics_count =
                        std::min(experiment.instances.size(), metrics_result.metrics.size());
                    for (std::size_t i = 0; i < metrics_count; ++i) {
                        experiment.instances[i].scene_metrics = metrics_result.metrics[i];
                    }
                    progress->current_scene_completed.store(entry.config.instance_count);
                } else {
                    set_batch_study_text_state(progress, "Metrics Ready", config_file,
                                               "Metrics were auto-computed during generation.");
                }

                const std::size_t algorithm_total =
                    experiment::ExperimentService::estimate_total_algorithm_runs(experiment);
                progress->current_algorithm_total.store(algorithm_total);
                progress->current_algorithm_completed.store(0);

                set_batch_study_text_state(progress, "Running Algorithms", config_file);
                if (algorithm_total > 0) {
                    experiment::ExperimentService::run_algorithms(
                        experiment,
                        &progress->current_algorithm_completed);
                    progress->current_algorithm_completed.store(algorithm_total);
                } else {
                    set_batch_study_text_state(progress,
                                               "Running Algorithms",
                                               config_file,
                                               "No enabled algorithms or generated scenes to benchmark.");
                }

                set_batch_study_text_state(progress, "Saving Experiment", config_file);
                progress->current_scene_total.store(0);
                progress->current_scene_completed.store(0);
                progress->current_algorithm_total.store(0);
                progress->current_algorithm_completed.store(0);

                const std::filesystem::path save_path =
                    result.output_directory / (entry.path.stem().string() + "_experiment.json");
                if (!experiment::ExperimentIO::save_to_file(experiment, save_path.string())) {
                    throw std::runtime_error("Failed to save experiment file: " + save_path.string());
                }

                result.last_experiment_path = save_path;
                result.last_experiment = std::move(experiment);
                ++result.saved_experiment_count;

                progress->processed_configs.fetch_add(1);
                progress->processed_scene_count.fetch_add(entry.config.instance_count);
                {
                    std::lock_guard<std::mutex> lock(progress->text_mutex);
                    progress->last_saved_file = save_path.string();
                    if (result.skipped_json_count == 0) {
                        progress->note.clear();
                    }
                }
            } catch (const std::exception& e) {
                ++result.failed_config_count;
                progress->processed_configs.fetch_add(1);
                progress->processed_scene_count.fetch_add(entry.config.instance_count);
                progress->current_scene_total.store(0);
                progress->current_scene_completed.store(0);
                progress->current_algorithm_total.store(0);
                progress->current_algorithm_completed.store(0);
                set_batch_study_text_state(progress, "Config Failed", config_file, e.what());
            }
        }

        std::string summary_note =
            "Saved " + std::to_string(result.saved_experiment_count) + " experiment(s).";
        if (result.failed_config_count > 0) {
            summary_note += " Failed configs: " + std::to_string(result.failed_config_count) + ".";
        }
        if (result.skipped_json_count > 0) {
            summary_note += " Skipped non-config JSON files: " + std::to_string(result.skipped_json_count) + ".";
        }

        set_batch_study_text_state(progress, "Completed", "", summary_note);
        return result;
    }

    bool save_window_screenshot(sf::RenderWindow& window, const std::string& output_path) {
        const sf::Vector2u window_size = window.getSize();
        if (window_size.x == 0 || window_size.y == 0) {
            return false;
        }

        sf::Texture texture;
        if (!texture.resize(window_size)) {
            return false;
        }

        texture.update(window);
        const sf::Image image = texture.copyToImage();
        return image.saveToFile(output_path);
    }

    std::string experiment_status_to_text(experiment::SceneGenerationStatus status) {
        switch (status) {
            case experiment::SceneGenerationStatus::NotGenerated:
                return "NotGenerated";
            case experiment::SceneGenerationStatus::Generated:
                return "Generated";
            case experiment::SceneGenerationStatus::Failed:
                return "Failed";
        }
        return "Unknown";
    }

    std::string run_status_to_text(experiment::SceneRunStatus status) {
        switch (status) {
            case experiment::SceneRunStatus::NotRun:
                return "NotRun";
            case experiment::SceneRunStatus::Ready:
                return "Ready";
            case experiment::SceneRunStatus::Running:
                return "Running";
            case experiment::SceneRunStatus::Completed:
                return "Completed";
            case experiment::SceneRunStatus::Failed:
                return "Failed";
        }
        return "Unknown";
    }

    std::string algorithm_status_to_text(experiment::AlgorithmRunStatus status) {
        switch (status) {
            case experiment::AlgorithmRunStatus::NotRun:
                return "NotRun";
            case experiment::AlgorithmRunStatus::Running:
                return "Running";
            case experiment::AlgorithmRunStatus::Success:
                return "Success";
            case experiment::AlgorithmRunStatus::NoPath:
                return "NoPath";
            case experiment::AlgorithmRunStatus::Failed:
                return "Failed";
        }
        return "Unknown";
    }

    std::string metrics_execution_mode_to_text(experiment::MetricsExecutionMode mode) {
        switch (mode) {
            case experiment::MetricsExecutionMode::Serial:
                return "serial";
            case experiment::MetricsExecutionMode::Parallel:
                return "parallel";
        }
        return "unknown";
    }

    std::string obstacle_area_method_to_text(experiment::ObstacleAreaRatioMethod method) {
        switch (method) {
            case experiment::ObstacleAreaRatioMethod::MonteCarlo:
                return "monte_carlo";
            case experiment::ObstacleAreaRatioMethod::GridSampling:
                return "grid_sampling";
        }
        return "unknown";
    }

    std::string clearance_region_mode_to_text(experiment::ClearanceRegionMode mode) {
        switch (mode) {
            case experiment::ClearanceRegionMode::LargestFreeComponent:
                return "largest_free_component";
            case experiment::ClearanceRegionMode::StartGoalComponent:
                return "start_goal_component";
            case experiment::ClearanceRegionMode::StartGoalOrLargest:
                return "start_goal_or_largest";
        }
        return "unknown";
    }

    void draw_optional_metric(const char* label,
                              const std::optional<double>& value,
                              const bool show_as_percent = false) {
        if (!value.has_value()) {
            ImGui::Text("%s: N/A", label);
            return;
        }

        if (show_as_percent) {
            ImGui::Text("%s: %.2f%%", label, value.value() * 100.0);
        } else {
            ImGui::Text("%s: %.5f", label, value.value());
        }
    }

    struct TangentVisibilityArcOverlay {
        geometry::Point center;
        double radius = 0.0;
        double start_angle = 0.0;
        double end_angle = 0.0;
    };

    struct TangentVisibilityOverlayData {
        std::vector<std::pair<geometry::Point, geometry::Point>> tangent_segments;
        std::vector<TangentVisibilityArcOverlay> boundary_arcs;
    };

    constexpr double kPi = 3.14159265358979323846;

    double normalize_angle(double angle) {
        while (angle < 0.0) {
            angle += 2.0 * kPi;
        }
        while (angle >= 2.0 * kPi) {
            angle -= 2.0 * kPi;
        }
        return angle;
    }

    double positive_angle_delta(double from, double to) {
        double delta = normalize_angle(to) - normalize_angle(from);
        if (delta < 0.0) {
            delta += 2.0 * kPi;
        }
        return delta;
    }

    bool point_in_scene_bounds(const geometry::Point& point, const geometry::Scene& scene) {
        return point.x >= 0.0 && point.x <= scene.width &&
               point.y >= 0.0 && point.y <= scene.height;
    }

    double distance_sq_point_to_segment(const geometry::Point& point,
                                        const geometry::Point& segment_start,
                                        const geometry::Point& segment_end) {
        const double vx = segment_end.x - segment_start.x;
        const double vy = segment_end.y - segment_start.y;
        const double len_sq = vx * vx + vy * vy;
        if (len_sq <= 1e-12) {
            const double dx = point.x - segment_start.x;
            const double dy = point.y - segment_start.y;
            return dx * dx + dy * dy;
        }

        const double wx = point.x - segment_start.x;
        const double wy = point.y - segment_start.y;
        const double t_unclamped = (wx * vx + wy * vy) / len_sq;
        const double t = std::max(0.0, std::min(1.0, t_unclamped));

        const double cx = segment_start.x + t * vx;
        const double cy = segment_start.y + t * vy;
        const double dx = point.x - cx;
        const double dy = point.y - cy;
        return dx * dx + dy * dy;
    }

    bool index_is_ignored(const std::size_t index, const std::vector<std::size_t>& ignored_indices) {
        return std::find(ignored_indices.begin(), ignored_indices.end(), index) != ignored_indices.end();
    }

    bool segment_is_collision_free(const geometry::Point& a,
                                   const geometry::Point& b,
                                   const geometry::Scene& scene,
                                   const std::vector<std::size_t>& ignored_obstacles = {}) {
        if (!point_in_scene_bounds(a, scene) || !point_in_scene_bounds(b, scene)) {
            return false;
        }

        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            if (index_is_ignored(obstacle_index, ignored_obstacles)) {
                continue;
            }

            const auto& obstacle = scene.obstacles[obstacle_index];
            if (obstacle.radius <= 0.0) {
                continue;
            }
            const double distance_sq = distance_sq_point_to_segment(obstacle.center, a, b);
            const double radius_sq = obstacle.radius * obstacle.radius;
            if (distance_sq < radius_sq - 1e-9) {
                return false;
            }
        }

        return true;
    }

    bool point_is_inside_other_obstacle(const geometry::Point& point,
                                        const geometry::Scene& scene,
                                        const std::size_t self_obstacle_index) {
        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            if (obstacle_index == self_obstacle_index) {
                continue;
            }

            const auto& obstacle = scene.obstacles[obstacle_index];
            const double dist = point.distance(obstacle.center);
            if (dist < obstacle.radius - 1e-9) {
                return true;
            }
        }

        return false;
    }

    bool arc_is_collision_free(const geometry::Scene& scene,
                               const std::size_t obstacle_index,
                               const double start_angle,
                               const double end_angle) {
        if (obstacle_index >= scene.obstacles.size()) {
            return false;
        }

        const auto& obstacle = scene.obstacles[obstacle_index];
        if (obstacle.radius <= 0.0) {
            return false;
        }

        const double delta = positive_angle_delta(start_angle, end_angle);
        if (delta <= 1e-9) {
            return false;
        }

        const int samples = std::max(8, static_cast<int>(std::ceil(delta / (kPi / 24.0))));
        for (int i = 1; i < samples; ++i) {
            const double t = static_cast<double>(i) / static_cast<double>(samples);
            const double angle = start_angle + delta * t;
            const geometry::Point point{
                obstacle.center.x + obstacle.radius * std::cos(angle),
                obstacle.center.y + obstacle.radius * std::sin(angle)
            };

            if (!point_in_scene_bounds(point, scene)) {
                return false;
            }
            if (point_is_inside_other_obstacle(point, scene, obstacle_index)) {
                return false;
            }
        }

        return true;
    }

    bool points_are_close(const geometry::Point& a, const geometry::Point& b, const double eps = 1e-5) {
        return a.distance(b) <= eps;
    }

    void append_unique_point(std::vector<geometry::Point>& points, const geometry::Point& candidate) {
        for (const auto& existing : points) {
            if (points_are_close(existing, candidate)) {
                return;
            }
        }
        points.push_back(candidate);
    }

    bool append_unique_segment(std::vector<std::pair<geometry::Point, geometry::Point>>& segments,
                               const geometry::Point& a,
                               const geometry::Point& b) {
        for (const auto& segment : segments) {
            const bool same_direction = points_are_close(segment.first, a) && points_are_close(segment.second, b);
            const bool reversed_direction = points_are_close(segment.first, b) && points_are_close(segment.second, a);
            if (same_direction || reversed_direction) {
                return false;
            }
        }

        segments.emplace_back(a, b);
        return true;
    }

    std::vector<std::pair<geometry::Point, geometry::Point>> circle_circle_tangents(const geometry::Disk& first,
                                                                                     const geometry::Disk& second) {
        std::vector<std::pair<geometry::Point, geometry::Point>> tangents;
        const double dx = second.center.x - first.center.x;
        const double dy = second.center.y - first.center.y;
        const double z = dx * dx + dy * dy;
        if (z <= 1e-12) {
            return tangents;
        }

        for (int sign = -1; sign <= 1; sign += 2) {
            const double r = first.radius - static_cast<double>(sign) * second.radius;
            const double d_sq = z - r * r;
            if (d_sq < -1e-9) {
                continue;
            }

            const double d = d_sq <= 0.0 ? 0.0 : std::sqrt(d_sq);
            for (int orient = -1; orient <= 1; orient += 2) {
                const double nx = (dx * r + -dy * d * static_cast<double>(orient)) / z;
                const double ny = (dy * r + dx * d * static_cast<double>(orient)) / z;

                const geometry::Point on_first{
                    first.center.x + first.radius * nx,
                    first.center.y + first.radius * ny
                };
                const geometry::Point on_second{
                    second.center.x + static_cast<double>(sign) * second.radius * nx,
                    second.center.y + static_cast<double>(sign) * second.radius * ny
                };
                tangents.emplace_back(on_first, on_second);
            }
        }

        return tangents;
    }

    std::vector<geometry::Point> point_circle_tangency_points(const geometry::Point& point,
                                                              const geometry::Disk& obstacle) {
        std::vector<geometry::Point> tangency_points;
        const double dx = point.x - obstacle.center.x;
        const double dy = point.y - obstacle.center.y;
        const double dist_sq = dx * dx + dy * dy;
        const double radius_sq = obstacle.radius * obstacle.radius;
        if (dist_sq <= radius_sq + 1e-12) {
            return tangency_points;
        }

        const double l = radius_sq / dist_sq;
        const double m = obstacle.radius * std::sqrt(dist_sq - radius_sq) / dist_sq;

        tangency_points.push_back({
            obstacle.center.x + l * dx - m * dy,
            obstacle.center.y + l * dy + m * dx
        });
        tangency_points.push_back({
            obstacle.center.x + l * dx + m * dy,
            obstacle.center.y + l * dy - m * dx
        });
        return tangency_points;
    }

    TangentVisibilityOverlayData build_tangent_visibility_overlay(const geometry::Scene& scene) {
        TangentVisibilityOverlayData overlay;
        std::vector<std::vector<geometry::Point>> obstacle_tangent_points(scene.obstacles.size());

        auto try_add_segment = [&](const geometry::Point& a,
                                   const geometry::Point& b,
                                   const std::vector<std::size_t>& ignored_obstacles,
                                   const std::optional<std::size_t> obstacle_a = std::nullopt,
                                   const std::optional<std::size_t> obstacle_b = std::nullopt) {
            if (!segment_is_collision_free(a, b, scene, ignored_obstacles)) {
                return;
            }
            if (!append_unique_segment(overlay.tangent_segments, a, b)) {
                return;
            }

            if (obstacle_a.has_value()) {
                append_unique_point(obstacle_tangent_points[obstacle_a.value()], a);
            }
            if (obstacle_b.has_value()) {
                append_unique_point(obstacle_tangent_points[obstacle_b.value()], b);
            }
        };

        // Direct visibility between start and goal.
        try_add_segment(scene.start, scene.goal, {});

        // Tangents from start/goal to each obstacle.
        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];

            for (const auto& tangent_point : point_circle_tangency_points(scene.start, obstacle)) {
                try_add_segment(scene.start, tangent_point, {obstacle_index}, std::nullopt, obstacle_index);
            }
            for (const auto& tangent_point : point_circle_tangency_points(scene.goal, obstacle)) {
                try_add_segment(scene.goal, tangent_point, {obstacle_index}, std::nullopt, obstacle_index);
            }
        }

        // Bitangents between obstacle pairs.
        for (std::size_t i = 0; i < scene.obstacles.size(); ++i) {
            for (std::size_t j = i + 1; j < scene.obstacles.size(); ++j) {
                const auto tangent_candidates = circle_circle_tangents(scene.obstacles[i], scene.obstacles[j]);
                for (const auto& tangent : tangent_candidates) {
                    try_add_segment(tangent.first, tangent.second, {i, j}, i, j);
                }
            }
        }

        // Boundary arcs between consecutive tangent points on each obstacle.
        for (std::size_t obstacle_index = 0; obstacle_index < scene.obstacles.size(); ++obstacle_index) {
            const auto& obstacle = scene.obstacles[obstacle_index];
            auto& points = obstacle_tangent_points[obstacle_index];
            if (points.size() < 2) {
                continue;
            }

            std::vector<std::pair<double, geometry::Point>> by_angle;
            by_angle.reserve(points.size());
            for (const auto& point : points) {
                const double angle = normalize_angle(std::atan2(point.y - obstacle.center.y,
                                                                point.x - obstacle.center.x));
                by_angle.emplace_back(angle, point);
            }

            std::sort(by_angle.begin(), by_angle.end(),
                      [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

            std::vector<double> unique_angles;
            unique_angles.reserve(by_angle.size());
            for (const auto& entry : by_angle) {
                if (unique_angles.empty() ||
                    std::abs(positive_angle_delta(unique_angles.back(), entry.first)) > 1e-4) {
                    unique_angles.push_back(entry.first);
                }
            }
            if (unique_angles.size() >= 2 &&
                positive_angle_delta(unique_angles.back(), unique_angles.front()) < 1e-4) {
                unique_angles.pop_back();
            }
            if (unique_angles.size() < 2) {
                continue;
            }

            for (std::size_t k = 0; k < unique_angles.size(); ++k) {
                const double start_angle = unique_angles[k];
                const double end_angle = unique_angles[(k + 1) % unique_angles.size()];

                if (!arc_is_collision_free(scene, obstacle_index, start_angle, end_angle)) {
                    continue;
                }

                overlay.boundary_arcs.push_back({
                    obstacle.center,
                    obstacle.radius,
                    start_angle,
                    end_angle
                });
            }
        }

        return overlay;
    }

    void draw_tangent_visibility_overlay(sf::RenderWindow& window,
                                         const TangentVisibilityOverlayData& overlay,
                                         const std::function<sf::Vector2f(const geometry::Point&)>& scene_to_screen) {
        const sf::Color tangent_color(30, 90, 240, 220);
        const sf::Color arc_color(240, 120, 20, 230);

        for (const auto& segment : overlay.tangent_segments) {
            const sf::Vector2f a = scene_to_screen(segment.first);
            const sf::Vector2f b = scene_to_screen(segment.second);
            sf::Vertex line[] = {sf::Vertex(a, tangent_color), sf::Vertex(b, tangent_color)};
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        for (const auto& arc : overlay.boundary_arcs) {
            const double delta = positive_angle_delta(arc.start_angle, arc.end_angle);
            const int segment_count = std::max(10, static_cast<int>(std::ceil(delta / (kPi / 40.0))));
            geometry::Point prev_point{
                arc.center.x + arc.radius * std::cos(arc.start_angle),
                arc.center.y + arc.radius * std::sin(arc.start_angle)
            };

            for (int i = 1; i <= segment_count; ++i) {
                const double t = static_cast<double>(i) / static_cast<double>(segment_count);
                const double angle = arc.start_angle + delta * t;
                const geometry::Point next_point{
                    arc.center.x + arc.radius * std::cos(angle),
                    arc.center.y + arc.radius * std::sin(angle)
                };

                const sf::Vector2f from = scene_to_screen(prev_point);
                const sf::Vector2f to = scene_to_screen(next_point);
                sf::Vertex line[] = {sf::Vertex(from, arc_color), sf::Vertex(to, arc_color)};
                window.draw(line, 2, sf::PrimitiveType::Lines);

                prev_point = next_point;
            }
        }
    }

    struct RRTDemoNode {
        geometry::Point point;
        std::size_t parent = 0;
    };

    enum class RRTDemoStatus {
        Idle,
        Running,
        Succeeded,
        Failed
    };

    struct RRTDemoConfig {
        int tick_ms = 30;
        std::size_t max_iterations = 5000;
        double step_size = 2.5;
        double goal_sample_rate = 0.10;
        double goal_tolerance = 2.0;
        std::uint32_t seed = 42;
        bool show_tree = true;
        bool show_path = true;
    };

    struct RRTDemoState {
        RRTDemoStatus status = RRTDemoStatus::Idle;
        int bound_scene_index = -1;
        std::size_t iterations_done = 0;
        RRTDemoConfig config;
        std::vector<RRTDemoNode> nodes;
        geometry::Path final_path;
        std::mt19937 rng;
        std::chrono::steady_clock::time_point last_tick_time{};
        bool tick_time_initialized = false;
    };

    bool point_inside_obstacles(const geometry::Point& point, const geometry::Scene& scene) {
        for (const auto& obstacle : scene.obstacles) {
            if (obstacle.contains(point)) {
                return true;
            }
        }
        return false;
    }

    geometry::Point steer_towards(const geometry::Point& from,
                                  const geometry::Point& to,
                                  const double step_size) {
        const double dx = to.x - from.x;
        const double dy = to.y - from.y;
        const double dist = std::hypot(dx, dy);
        if (dist <= step_size || dist <= 1e-12) {
            return to;
        }
        const double alpha = step_size / dist;
        return {from.x + dx * alpha, from.y + dy * alpha};
    }

    std::size_t find_nearest_rrt_node(const std::vector<RRTDemoNode>& nodes, const geometry::Point& target) {
        std::size_t best_index = 0;
        double best_dist_sq = std::numeric_limits<double>::infinity();

        for (std::size_t i = 0; i < nodes.size(); ++i) {
            const double dx = nodes[i].point.x - target.x;
            const double dy = nodes[i].point.y - target.y;
            const double dist_sq = dx * dx + dy * dy;
            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_index = i;
            }
        }

        return best_index;
    }

    geometry::Path reconstruct_rrt_path(const std::vector<RRTDemoNode>& nodes, std::size_t node_index) {
        geometry::Path path;
        std::vector<geometry::Point> reversed_points;
        reversed_points.reserve(nodes.size());

        while (true) {
            reversed_points.push_back(nodes[node_index].point);
            if (nodes[node_index].parent == node_index) {
                break;
            }

            const std::size_t next = nodes[node_index].parent;
            if (next >= nodes.size() || next == node_index) {
                break;
            }
            node_index = next;
        }

        path.points.assign(reversed_points.rbegin(), reversed_points.rend());
        return path;
    }

    RRTDemoConfig normalized_rrt_config(const RRTDemoConfig& raw_config) {
        RRTDemoConfig config = raw_config;
        config.tick_ms = std::max(1, config.tick_ms);
        config.max_iterations = std::max<std::size_t>(1, config.max_iterations);
        config.step_size = std::max(0.05, config.step_size);
        config.goal_sample_rate = std::clamp(config.goal_sample_rate, 0.0, 1.0);
        config.goal_tolerance = std::max(0.05, config.goal_tolerance);
        return config;
    }

    bool start_rrt_demo(RRTDemoState& state,
                        const geometry::Scene& scene,
                        const int scene_index,
                        const RRTDemoConfig& raw_config,
                        std::string& error_message) {
        state = RRTDemoState{};
        state.config = normalized_rrt_config(raw_config);
        state.bound_scene_index = scene_index;

        if (!point_in_scene_bounds(scene.start, scene) || !point_in_scene_bounds(scene.goal, scene)) {
            error_message = "RRT demo: start/goal is out of scene bounds";
            state.status = RRTDemoStatus::Failed;
            return false;
        }
        if (point_inside_obstacles(scene.start, scene) || point_inside_obstacles(scene.goal, scene)) {
            error_message = "RRT demo: start/goal is inside obstacle";
            state.status = RRTDemoStatus::Failed;
            return false;
        }

        state.rng.seed(state.config.seed);
        state.nodes.push_back({scene.start, 0});
        state.status = RRTDemoStatus::Running;
        state.last_tick_time = std::chrono::steady_clock::now();
        state.tick_time_initialized = true;

        if (segment_is_collision_free(scene.start, scene.goal, scene)) {
            state.status = RRTDemoStatus::Succeeded;
            state.final_path.points = {scene.start, scene.goal};
        }

        return true;
    }

    void reset_rrt_demo(RRTDemoState& state) {
        state = RRTDemoState{};
    }

    void advance_rrt_demo_once(RRTDemoState& state, const geometry::Scene& scene) {
        if (state.status != RRTDemoStatus::Running) {
            return;
        }
        if (state.iterations_done >= state.config.max_iterations) {
            state.status = RRTDemoStatus::Failed;
            return;
        }

        ++state.iterations_done;

        std::bernoulli_distribution goal_sample_distribution(state.config.goal_sample_rate);
        std::uniform_real_distribution<double> x_distribution(0.0, scene.width);
        std::uniform_real_distribution<double> y_distribution(0.0, scene.height);

        geometry::Point sampled;
        if (goal_sample_distribution(state.rng)) {
            sampled = scene.goal;
        } else {
            bool found_free_sample = false;
            for (int attempt = 0; attempt < 32; ++attempt) {
                sampled = {x_distribution(state.rng), y_distribution(state.rng)};
                if (!point_inside_obstacles(sampled, scene)) {
                    found_free_sample = true;
                    break;
                }
            }
            if (!found_free_sample) {
                if (state.iterations_done >= state.config.max_iterations) {
                    state.status = RRTDemoStatus::Failed;
                }
                return;
            }
        }

        const std::size_t nearest_index = find_nearest_rrt_node(state.nodes, sampled);
        const geometry::Point candidate = steer_towards(
            state.nodes[nearest_index].point, sampled, state.config.step_size);

        if (!point_in_scene_bounds(candidate, scene) ||
            point_inside_obstacles(candidate, scene) ||
            !segment_is_collision_free(state.nodes[nearest_index].point, candidate, scene)) {
            if (state.iterations_done >= state.config.max_iterations) {
                state.status = RRTDemoStatus::Failed;
            }
            return;
        }

        const std::size_t new_index = state.nodes.size();
        state.nodes.push_back({candidate, nearest_index});

        if (candidate.distance(scene.goal) <= state.config.goal_tolerance &&
            segment_is_collision_free(candidate, scene.goal, scene)) {
            const std::size_t goal_index = state.nodes.size();
            state.nodes.push_back({scene.goal, new_index});
            state.final_path = reconstruct_rrt_path(state.nodes, goal_index);
            state.status = RRTDemoStatus::Succeeded;
            return;
        }

        if (state.iterations_done >= state.config.max_iterations) {
            state.status = RRTDemoStatus::Failed;
        }
    }

    void advance_rrt_demo_by_time(RRTDemoState& state, const geometry::Scene& scene) {
        if (state.status != RRTDemoStatus::Running) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!state.tick_time_initialized) {
            state.last_tick_time = now;
            state.tick_time_initialized = true;
        }

        const auto tick = std::chrono::milliseconds(std::max(1, state.config.tick_ms));
        int safety = 0;
        while ((now - state.last_tick_time) >= tick && state.status == RRTDemoStatus::Running && safety < 128) {
            advance_rrt_demo_once(state, scene);
            state.last_tick_time += tick;
            ++safety;
        }
    }

    void draw_rrt_demo_overlay(sf::RenderWindow& window,
                               const RRTDemoState& state,
                               const std::function<sf::Vector2f(const geometry::Point&)>& scene_to_screen) {
        if (state.config.show_tree && state.nodes.size() > 1) {
            const sf::Color tree_edge_color(255, 120, 0, 170);
            const sf::Color tree_node_color(255, 170, 70, 220);

            for (std::size_t i = 1; i < state.nodes.size(); ++i) {
                const auto& node = state.nodes[i];
                if (node.parent >= state.nodes.size()) {
                    continue;
                }

                const sf::Vector2f from = scene_to_screen(state.nodes[node.parent].point);
                const sf::Vector2f to = scene_to_screen(node.point);
                sf::Vertex edge[] = {sf::Vertex(from, tree_edge_color), sf::Vertex(to, tree_edge_color)};
                window.draw(edge, 2, sf::PrimitiveType::Lines);
            }

            for (const auto& node : state.nodes) {
                sf::CircleShape point_marker(1.8f);
                point_marker.setFillColor(tree_node_color);
                point_marker.setPointCount(10);
                const sf::Vector2f p = scene_to_screen(node.point);
                point_marker.setPosition(p - sf::Vector2f(1.8f, 1.8f));
                window.draw(point_marker);
            }
        }

        if (state.config.show_path &&
            state.status == RRTDemoStatus::Succeeded &&
            !state.final_path.points.empty()) {
            const sf::Color path_color(35, 35, 255, 240);

            for (std::size_t i = 0; i + 1 < state.final_path.points.size(); ++i) {
                const sf::Vector2f from = scene_to_screen(state.final_path.points[i]);
                const sf::Vector2f to = scene_to_screen(state.final_path.points[i + 1]);
                sf::Vertex edge[] = {sf::Vertex(from, path_color), sf::Vertex(to, path_color)};
                window.draw(edge, 2, sf::PrimitiveType::Lines);
            }

            for (const auto& point : state.final_path.points) {
                sf::CircleShape marker(2.6f);
                marker.setFillColor(path_color);
                marker.setPointCount(14);
                const sf::Vector2f p = scene_to_screen(point);
                marker.setPosition(p - sf::Vector2f(2.6f, 2.6f));
                window.draw(marker);
            }
        }
    }

    sf::Color algorithm_path_color_by_index(const std::size_t index) {
        static const std::vector<sf::Color> palette = {
            sf::Color(230, 50, 60, 230),
            sf::Color(30, 140, 245, 230),
            sf::Color(40, 180, 90, 230),
            sf::Color(230, 160, 30, 230),
            sf::Color(170, 70, 220, 230),
            sf::Color(20, 200, 200, 230),
            sf::Color(220, 90, 150, 230),
            sf::Color(120, 120, 120, 230)
        };
        return palette[index % palette.size()];
    }

    void draw_path_overlay(sf::RenderWindow& window,
                           const geometry::Path& path,
                           const sf::Color& color,
                           const std::function<sf::Vector2f(const geometry::Point&)>& scene_to_screen) {
        if (path.points.empty()) {
            return;
        }

        for (std::size_t i = 0; i + 1 < path.points.size(); ++i) {
            const sf::Vector2f from = scene_to_screen(path.points[i]);
            const sf::Vector2f to = scene_to_screen(path.points[i + 1]);
            sf::Vertex edge[] = {sf::Vertex(from, color), sf::Vertex(to, color)};
            window.draw(edge, 2, sf::PrimitiveType::Lines);
        }

        for (const auto& point : path.points) {
            sf::CircleShape marker(2.2f);
            marker.setFillColor(color);
            marker.setPointCount(12);
            const sf::Vector2f p = scene_to_screen(point);
            marker.setPosition(p - sf::Vector2f(2.2f, 2.2f));
            window.draw(marker);
        }
    }

} // namespace

namespace visualization {

    SceneVisualizer::SceneVisualizer(unsigned int window_width, unsigned int window_height)
        : window_width_(window_width), window_height_(window_height),
          camera_controller_(window_width, window_height) {}

    bool SceneVisualizer::visualize(const geometry::Scene& scene,
                                    const std::optional<geometry::Path>& path,
                                    bool show_grid,
                                    bool show_graph) {
        // Enable antialiasing for smooth rendering
        sf::ContextSettings settings;
        settings.antiAliasingLevel = 8;

        sf::RenderWindow window(sf::VideoMode({window_width_, window_height_}),
                               "Scene Visualization", sf::State::Windowed, settings);
        window.setFramerateLimit(60);

        // Initialize UI Manager
        if (!ui_manager_.initialize(window)) {
            return false;
        }

        // Reset camera
        camera_controller_.reset();

        sf::Clock deltaClock;

        while (window.isOpen()) {
            sf::Time deltaTime = deltaClock.restart();

            // Update UI
            ui_manager_.update(window, deltaTime);

            while (std::optional<sf::Event> event = window.pollEvent()) {
                // Process UI events first - need to pass to ImGui directly
                ImGui::SFML::ProcessEvent(window, *event);

                // Skip other event processing if UI wants to capture the event
                if (ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard) {
                    if (event->is<sf::Event::Closed>()) {
                        window.close();
                    }
                    continue;
                }

                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (auto* resized = event->getIf<sf::Event::Resized>()) {
                    window_width_ = resized->size.x;
                    window_height_ = resized->size.y;
                    camera_controller_.set_window_size(window_width_, window_height_);
                    sf::FloatRect visibleArea(sf::Vector2f(0, 0),
                                            sf::Vector2f(static_cast<float>(window_width_), static_cast<float>(window_height_)));
                    window.setView(sf::View(visibleArea));
                } else if (auto* mouseButton = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouseButton->button == sf::Mouse::Button::Left) {
                        sf::Vector2i mouse_pos(mouseButton->position.x, mouseButton->position.y);
                        camera_controller_.handle_mouse_press(mouse_pos, scene);
                    }
                } else if (auto* mouseButton = event->getIf<sf::Event::MouseButtonReleased>()) {
                    if (mouseButton->button == sf::Mouse::Button::Left) {
                        camera_controller_.handle_mouse_release();
                    }
                } else if (auto* mouseMove = event->getIf<sf::Event::MouseMoved>()) {
                    sf::Vector2i mouse_pos(mouseMove->position.x, mouseMove->position.y);
                    camera_controller_.handle_mouse_move(mouse_pos, scene);
                } else if (auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    sf::Vector2i mouse_pos(mouseWheel->position.x, mouseWheel->position.y);
                    double zoom_factor = (mouseWheel->delta > 0) ? 1.1 : 1.0 / 1.1;
                    camera_controller_.zoom_at_point(mouse_pos, scene, zoom_factor);
                }
            }

            window.clear(sf::Color::White);

            // Create lambda functions for coordinate transformation
            auto scene_to_screen_func = [this, &scene](const geometry::Point& p) {
                return camera_controller_.scene_to_screen(p, scene);
            };
            auto get_scale_func = [this, &scene]() {
                return camera_controller_.get_scale(scene);
            };

            if (show_graph && graph_builder_ && get_node_point_func_) {
                // Draw graph mode
                // Build graph from scene
                algorithms::graph::Graph graph = graph_builder_->build(scene);

                // Draw graph
                graph_renderer_.draw_graph(window, graph, get_node_point_func_, scene_to_screen_func);

                // Draw start/goal points on top
                renderer_.draw_start_goal(window, scene, scene_to_screen_func);

                if (path.has_value()) {
                    renderer_.draw_path(window, scene, path.value(), scene_to_screen_func);
                }
            } else {
                // Draw scene mode (normal)
                if (show_grid) {
                    renderer_.draw_grid(window, scene, scene_to_screen_func, get_scale_func);
                }

                renderer_.draw_obstacles(window, scene, scene_to_screen_func, get_scale_func);
                renderer_.draw_start_goal(window, scene, scene_to_screen_func);

                if (path.has_value()) {
                    renderer_.draw_path(window, scene, path.value(), scene_to_screen_func);
                }
            }

            // Draw scrollbars
            camera_controller_.draw_scrollbars(window, scene);

            // Render UI
            ui_manager_.render(window);

            if (ui_manager_.consume_save_screenshot_request()) {
                const std::optional<std::string> output_path = show_save_screenshot_dialog();
                if (output_path.has_value()) {
                    if (!save_window_screenshot(window, output_path.value())) {
                        std::cerr << "Failed to save screenshot to: " << output_path.value() << "\n";
                    } else {
                        std::cout << "Screenshot saved to: " << output_path.value() << "\n";
                    }
                }
            }

            window.display();
        }

        ui_manager_.shutdown();

        return true;
    }

    bool SceneVisualizer::run_experiment_workspace() {
        sf::ContextSettings settings;
        settings.antiAliasingLevel = 8;

        sf::RenderWindow window(sf::VideoMode({window_width_, window_height_}),
                                "Experiment Workspace", sf::State::Windowed, settings);
        window.setFramerateLimit(60);

        if (!ui_manager_.initialize(window)) {
            return false;
        }

        camera_controller_.reset();
        selected_scene_index_ = -1;
        status_message_ = "Create experiment, then open generator config and generate scenes.";
        bool show_grid_layer = true;
        bool show_obstacles_layer = true;
        bool show_start_goal_layer = true;
        bool show_scrollbars = true;
        bool enable_camera_navigation = true;
        TangentVisibilityOverlayData tangent_visibility_overlay;
        bool tangent_visibility_overlay_ready = false;
        int tangent_visibility_overlay_scene_index = -1;
        double grid_graph_cell_size = 5.0;
        bool grid_graph_allow_diagonal = true;
        algorithms::graph::GridGraphBuilder grid_graph_builder(grid_graph_cell_size, grid_graph_allow_diagonal);
        algorithms::graph::Graph grid_graph_overlay;
        bool grid_graph_overlay_ready = false;
        int grid_graph_overlay_scene_index = -1;
        RRTDemoState rrt_demo_state;
        int rrt_tick_ms = 30;
        int rrt_max_iterations = 5000;
        double rrt_step_size = 2.5;
        double rrt_goal_sample_rate = 0.10;
        double rrt_goal_tolerance = 2.0;
        bool rrt_use_scene_seed = true;
        int rrt_custom_seed = 42;
        bool rrt_show_tree = true;
        bool rrt_show_path = true;
        enum class AlgorithmRunScope {
            AllScenes,
            SelectedScene
        };
        bool algorithm_run_task_running = false;
        std::size_t algorithm_run_task_total = 0;
        std::shared_ptr<std::atomic<std::size_t>> algorithm_run_task_completed;
        std::future<experiment::Experiment> algorithm_run_task_future;
        AlgorithmRunScope algorithm_run_task_scope = AlgorithmRunScope::AllScenes;
        int algorithm_run_task_scene_index = -1;
        bool batch_folder_task_running = false;
        std::shared_ptr<BatchStudyProgressState> batch_study_progress;
        std::future<BatchStudyResult> batch_folder_task_future;
        bool show_algorithm_paths_overlay = true;
        std::unordered_map<std::string, bool> algorithm_path_visibility;

        sf::Clock deltaClock;
        while (window.isOpen()) {
            const sf::Time delta_time = deltaClock.restart();
            ui_manager_.update(window, delta_time);

            if (metrics_task_running_ && metrics_task_future_.valid()) {
                const auto future_status = metrics_task_future_.wait_for(std::chrono::milliseconds(0));
                if (future_status == std::future_status::ready) {
                    try {
                        const std::vector<experiment::SceneMetrics> computed_metrics = metrics_task_future_.get();
                        if (active_experiment_.has_value() &&
                            computed_metrics.size() == active_experiment_->instances.size()) {
                            for (std::size_t i = 0; i < computed_metrics.size(); ++i) {
                                active_experiment_->instances[i].scene_metrics = computed_metrics[i];
                            }
                            status_message_ = "Metrics calculation completed for " +
                                              std::to_string(computed_metrics.size()) + " scene(s).";
                        } else {
                            status_message_ =
                                "Metrics calculation finished, but experiment changed during computation.";
                        }
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Metrics calculation failed: ") + e.what();
                    } catch (...) {
                        status_message_ = "Metrics calculation failed: unknown error";
                    }
                    metrics_task_running_ = false;
                    metrics_task_total_ = 0;
                    metrics_task_completed_.reset();
                }
            }

            if (algorithm_run_task_running && algorithm_run_task_future.valid()) {
                const auto future_status = algorithm_run_task_future.wait_for(std::chrono::milliseconds(0));
                if (future_status == std::future_status::ready) {
                    try {
                        experiment::Experiment updated_experiment = algorithm_run_task_future.get();
                        active_experiment_ = std::move(updated_experiment);

                        if (selected_scene_index_ < 0 ||
                            selected_scene_index_ >= static_cast<int>(active_experiment_->instances.size())) {
                            select_first_generated_scene();
                        }

                        std::size_t success_runs = 0;
                        std::size_t no_path_runs = 0;
                        std::size_t failed_runs = 0;
                        if (algorithm_run_task_scope == AlgorithmRunScope::SelectedScene &&
                            algorithm_run_task_scene_index >= 0 &&
                            algorithm_run_task_scene_index < static_cast<int>(active_experiment_->instances.size())) {
                            const auto& instance =
                                active_experiment_->instances[static_cast<std::size_t>(algorithm_run_task_scene_index)];
                            for (const auto& run : instance.runs) {
                                if (run.status == experiment::AlgorithmRunStatus::Success) {
                                    ++success_runs;
                                } else if (run.status == experiment::AlgorithmRunStatus::NoPath) {
                                    ++no_path_runs;
                                } else if (run.status == experiment::AlgorithmRunStatus::Failed) {
                                    ++failed_runs;
                                }
                            }
                        } else {
                            for (const auto& instance : active_experiment_->instances) {
                                for (const auto& run : instance.runs) {
                                    if (run.status == experiment::AlgorithmRunStatus::Success) {
                                        ++success_runs;
                                    } else if (run.status == experiment::AlgorithmRunStatus::NoPath) {
                                        ++no_path_runs;
                                    } else if (run.status == experiment::AlgorithmRunStatus::Failed) {
                                        ++failed_runs;
                                    }
                                }
                            }
                        }

                        const std::string scope_label =
                            (algorithm_run_task_scope == AlgorithmRunScope::SelectedScene)
                                ? "selected scene"
                                : "all scenes";
                        status_message_ = "Algorithm benchmark completed (" + scope_label + "). Success=" +
                                          std::to_string(success_runs) +
                                          ", NoPath=" + std::to_string(no_path_runs) +
                                          ", Failed=" + std::to_string(failed_runs);
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Algorithm benchmark failed: ") + e.what();
                    } catch (...) {
                        status_message_ = "Algorithm benchmark failed: unknown error";
                    }

                    algorithm_run_task_running = false;
                    algorithm_run_task_total = 0;
                    algorithm_run_task_completed.reset();
                    algorithm_run_task_scene_index = -1;
                }
            }

            if (batch_folder_task_running && batch_folder_task_future.valid()) {
                const auto future_status = batch_folder_task_future.wait_for(std::chrono::milliseconds(0));
                if (future_status == std::future_status::ready) {
                    try {
                        BatchStudyResult batch_result = batch_folder_task_future.get();
                        if (batch_result.last_experiment.has_value()) {
                            active_experiment_ = std::move(batch_result.last_experiment);
                            if (!batch_result.last_experiment_path.empty()) {
                                active_experiment_path_ = batch_result.last_experiment_path;
                            } else {
                                active_experiment_path_ = std::nullopt;
                            }
                            select_first_generated_scene();
                            tangent_visibility_overlay_ready = false;
                            tangent_visibility_overlay_scene_index = -1;
                            grid_graph_overlay_ready = false;
                            grid_graph_overlay_scene_index = -1;
                            reset_rrt_demo(rrt_demo_state);
                            algorithm_path_visibility.clear();
                        }

                        status_message_ = "Config-folder batch completed. Saved " +
                                          std::to_string(batch_result.saved_experiment_count) +
                                          " experiment(s)";
                        if (batch_result.failed_config_count > 0) {
                            status_message_ += ", failed " +
                                              std::to_string(batch_result.failed_config_count);
                        }
                        if (batch_result.skipped_json_count > 0) {
                            status_message_ += ", skipped " +
                                              std::to_string(batch_result.skipped_json_count) +
                                              " non-config JSON file(s)";
                        }
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Config-folder batch failed: ") + e.what();
                    } catch (...) {
                        status_message_ = "Config-folder batch failed: unknown error";
                    }

                    batch_folder_task_running = false;
                }
            }

            const float panel_gap = 8.0f;
            const float outer_margin = panel_gap;
            const float top_offset = ImGui::GetFrameHeight() + 8.0f;
            const float left_width = 300.0f;
            const float right_width = 320.0f;
            const float workspace_x = outer_margin + left_width + panel_gap;
            const float workspace_y = top_offset;
            const float workspace_w = std::max(
                220.0f,
                static_cast<float>(window_width_) - left_width - right_width - panel_gap * 4.0f);
            const float workspace_h = std::max(80.0f, static_cast<float>(window_height_) - top_offset - outer_margin);
            const float right_x = workspace_x + workspace_w + panel_gap;
            const float right_h = workspace_h;
            const float right_half_h = (right_h - panel_gap) * 0.5f;
            const float details_y = top_offset;
            const float settings_y = top_offset + right_half_h + panel_gap;

            const experiment::SceneInstance* selected_instance = selected_scene_instance();
            const bool can_draw_scene = selected_instance != nullptr &&
                selected_instance->generation_status == experiment::SceneGenerationStatus::Generated;

            if (can_draw_scene) {
                camera_controller_.set_window_size(static_cast<unsigned int>(workspace_w),
                                                   static_cast<unsigned int>(workspace_h));
            }

            if (rrt_demo_state.status == RRTDemoStatus::Running) {
                if (!can_draw_scene || rrt_demo_state.bound_scene_index != selected_scene_index_) {
                    reset_rrt_demo(rrt_demo_state);
                    status_message_ = "RRT demo stopped: active scene changed.";
                } else {
                    advance_rrt_demo_by_time(rrt_demo_state, selected_instance->scene_data);
                    if (rrt_demo_state.status == RRTDemoStatus::Succeeded) {
                        status_message_ = "RRT demo completed: path found in " +
                                          std::to_string(rrt_demo_state.iterations_done) + " iterations.";
                    } else if (rrt_demo_state.status == RRTDemoStatus::Failed) {
                        status_message_ = "RRT demo finished: no path within iteration budget.";
                    }
                }
            }

            while (std::optional<sf::Event> event = window.pollEvent()) {
                ImGui::SFML::ProcessEvent(window, *event);

                if (event->is<sf::Event::Closed>()) {
                    window.close();
                    continue;
                }

                if (auto* resized = event->getIf<sf::Event::Resized>()) {
                    window_width_ = resized->size.x;
                    window_height_ = resized->size.y;
                    sf::FloatRect visible_area(sf::Vector2f(0, 0),
                                               sf::Vector2f(static_cast<float>(window_width_),
                                                            static_cast<float>(window_height_)));
                    window.setView(sf::View(visible_area));
                    continue;
                }

                if (!can_draw_scene) {
                    continue;
                }

                auto to_local = [workspace_x, workspace_y](const sf::Vector2i& point) {
                    return sf::Vector2i(static_cast<int>(point.x - workspace_x),
                                        static_cast<int>(point.y - workspace_y));
                };

                auto inside_workspace = [workspace_x, workspace_y, workspace_w, workspace_h](const sf::Vector2i& point) {
                    return point.x >= static_cast<int>(workspace_x) &&
                           point.x < static_cast<int>(workspace_x + workspace_w) &&
                           point.y >= static_cast<int>(workspace_y) &&
                           point.y < static_cast<int>(workspace_y + workspace_h);
                };

                const bool io_captures =
                    ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
                if (io_captures &&
                    !camera_controller_.is_dragging() &&
                    !camera_controller_.is_dragging_scrollbar()) {
                    continue;
                }

                if (!enable_camera_navigation) {
                    continue;
                }

                const auto& scene = selected_instance->scene_data;
                if (auto* mouse_button = event->getIf<sf::Event::MouseButtonPressed>()) {
                    if (mouse_button->button == sf::Mouse::Button::Left) {
                        sf::Vector2i mouse_pos(mouse_button->position.x, mouse_button->position.y);
                        if (inside_workspace(mouse_pos)) {
                            camera_controller_.handle_mouse_press(to_local(mouse_pos), scene);
                        }
                    }
                } else if (auto* mouse_button = event->getIf<sf::Event::MouseButtonReleased>()) {
                    if (mouse_button->button == sf::Mouse::Button::Left) {
                        camera_controller_.handle_mouse_release();
                    }
                } else if (auto* mouse_move = event->getIf<sf::Event::MouseMoved>()) {
                    sf::Vector2i mouse_pos(mouse_move->position.x, mouse_move->position.y);
                    if (camera_controller_.is_dragging() ||
                        camera_controller_.is_dragging_scrollbar() ||
                        inside_workspace(mouse_pos)) {
                        camera_controller_.handle_mouse_move(to_local(mouse_pos), scene);
                    }
                } else if (auto* mouse_wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                    sf::Vector2i mouse_pos(mouse_wheel->position.x, mouse_wheel->position.y);
                    if (inside_workspace(mouse_pos)) {
                        double zoom_factor = (mouse_wheel->delta > 0) ? 1.1 : 1.0 / 1.1;
                        camera_controller_.zoom_at_point(to_local(mouse_pos), scene, zoom_factor);
                    }
                }
            }

            window.clear(sf::Color::White);

            if (can_draw_scene && workspace_w > 1.0f && workspace_h > 1.0f) {
                const auto& scene = selected_instance->scene_data;
                const sf::View previous_view = window.getView();

                sf::View workspace_view(
                    sf::FloatRect(sf::Vector2f(0.0f, 0.0f), sf::Vector2f(workspace_w, workspace_h)));
                workspace_view.setViewport(
                    sf::FloatRect(sf::Vector2f(workspace_x / static_cast<float>(window_width_),
                                               workspace_y / static_cast<float>(window_height_)),
                                  sf::Vector2f(workspace_w / static_cast<float>(window_width_),
                                               workspace_h / static_cast<float>(window_height_))));
                window.setView(workspace_view);

                sf::RectangleShape workspace_bg(sf::Vector2f(workspace_w, workspace_h));
                workspace_bg.setFillColor(sf::Color(255, 255, 255, 255));
                workspace_bg.setPosition(sf::Vector2f(0.0f, 0.0f));
                window.draw(workspace_bg);

                auto scene_to_screen = [this, &scene](const geometry::Point& point) {
                    return camera_controller_.scene_to_screen(point, scene);
                };
                auto get_scale = [this, &scene]() {
                    return camera_controller_.get_scale(scene);
                };

                if (show_grid_layer) {
                    renderer_.draw_grid(window, scene, scene_to_screen, get_scale);
                }
                if (show_obstacles_layer) {
                    renderer_.draw_obstacles(window, scene, scene_to_screen, get_scale);
                }
                if (show_start_goal_layer) {
                    renderer_.draw_start_goal(window, scene, scene_to_screen);
                }
                if (show_algorithm_paths_overlay && selected_instance != nullptr) {
                    for (std::size_t run_index = 0; run_index < selected_instance->runs.size(); ++run_index) {
                        const auto& run = selected_instance->runs[run_index];
                        if (!run.path.has_value()) {
                            continue;
                        }

                        const std::string visibility_key =
                            selected_instance->instance_id + "::" + run.algorithm_id;
                        auto it = algorithm_path_visibility.find(visibility_key);
                        if (it == algorithm_path_visibility.end()) {
                            it = algorithm_path_visibility.emplace(visibility_key, true).first;
                        }
                        if (!it->second) {
                            continue;
                        }

                        draw_path_overlay(
                            window,
                            run.path.value(),
                            algorithm_path_color_by_index(run_index),
                            scene_to_screen);
                    }
                }
                if (grid_graph_overlay_ready &&
                    grid_graph_overlay_scene_index == selected_scene_index_) {
                    graph_renderer_.set_show_edges(true);
                    graph_renderer_.set_show_nodes(true);
                    graph_renderer_.set_edge_color(sf::Color(30, 170, 70, 150));
                    graph_renderer_.set_node_color(sf::Color(10, 120, 40, 230));
                    graph_renderer_.set_node_radius(2.0);
                    graph_renderer_.draw_graph(
                        window,
                        grid_graph_overlay,
                        [&grid_graph_builder](const std::size_t node_id) {
                            return grid_graph_builder.get_node_point(node_id);
                        },
                        scene_to_screen);
                }
                if (tangent_visibility_overlay_ready &&
                    tangent_visibility_overlay_scene_index == selected_scene_index_) {
                    draw_tangent_visibility_overlay(window, tangent_visibility_overlay, scene_to_screen);
                }
                if (rrt_demo_state.status != RRTDemoStatus::Idle &&
                    rrt_demo_state.bound_scene_index == selected_scene_index_) {
                    draw_rrt_demo_overlay(window, rrt_demo_state, scene_to_screen);
                }
                if (show_scrollbars) {
                    camera_controller_.draw_scrollbars(window, scene);
                }

                window.setView(previous_view);
            }

            sf::RectangleShape workspace_border(sf::Vector2f(workspace_w, workspace_h));
            workspace_border.setPosition(sf::Vector2f(workspace_x, workspace_y));
            workspace_border.setFillColor(sf::Color::Transparent);
            workspace_border.setOutlineThickness(1.0f);
            workspace_border.setOutlineColor(sf::Color(160, 160, 160, 255));
            window.draw(workspace_border);

            ImGui::SetNextWindowPos(ImVec2(outer_margin, top_offset), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(left_width, std::max(80.0f, static_cast<float>(window_height_) - top_offset - outer_margin)),
                                     ImGuiCond_Always);
            ImGui::Begin("Scenes", nullptr,
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize);

            if (!active_experiment_.has_value()) {
                ImGui::TextWrapped("No experiment loaded.");
                ImGui::Separator();
                ImGui::TextWrapped("File -> Create Experiment...");
                ImGui::TextWrapped("File -> Open Config...");
                ImGui::TextWrapped("or File -> Open Experiment...");
            } else {
                ImGui::Text("Experiment: %s", active_experiment_->experiment_name.c_str());
                ImGui::Separator();

                for (std::size_t i = 0; i < active_experiment_->instances.size(); ++i) {
                    const auto& instance = active_experiment_->instances[i];
                    std::string label = instance.instance_id;
                    const bool selected = (selected_scene_index_ == static_cast<int>(i));
                    if (ImGui::Selectable(label.c_str(), selected)) {
                        const int new_index = static_cast<int>(i);
                        if (selected_scene_index_ != new_index) {
                            selected_scene_index_ = new_index;
                            camera_controller_.reset();
                            tangent_visibility_overlay_ready = false;
                            tangent_visibility_overlay_scene_index = -1;
                            grid_graph_overlay_ready = false;
                            grid_graph_overlay_scene_index = -1;
                            reset_rrt_demo(rrt_demo_state);
                            algorithm_path_visibility.clear();
                        }
                    }
                }
            }

            ImGui::Separator();
            ImGui::TextWrapped("%s", status_message_.c_str());
            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(workspace_x, workspace_y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(workspace_w, workspace_h),
                                     ImGuiCond_Always);
            ImGuiWindowFlags workspace_flags =
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;
            if (can_draw_scene) {
                workspace_flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;
            }
            ImGui::Begin("Workspace", nullptr,
                         workspace_flags);

            if (!active_experiment_.has_value()) {
                const char* message = "Create a new experiment,\nthen open config and generate scenes.";
                ImVec2 text_size = ImGui::CalcTextSize(message);
                ImVec2 available = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPos(ImVec2(std::max(0.0f, (available.x - text_size.x) * 0.5f),
                                           std::max(0.0f, (available.y - text_size.y) * 0.5f)));
                ImGui::TextUnformatted(message);
            } else if (active_experiment_->instances.empty()) {
                ImGui::TextWrapped("No scenes generated yet.");
                ImGui::TextWrapped("Use File -> Open Config... to generate.");
            } else if (selected_scene_index_ < 0 || selected_scene_index_ >= static_cast<int>(active_experiment_->instances.size())) {
                ImGui::TextWrapped("Select a scene in the left panel.");
            } else {
                const auto& instance = active_experiment_->instances[static_cast<std::size_t>(selected_scene_index_)];
                ImGui::Text("ID: %s", instance.instance_id.c_str());
                ImGui::Text("Status: %s", experiment_status_to_text(instance.generation_status).c_str());
                if (!instance.error_message.empty()) {
                    ImGui::Separator();
                    ImGui::TextWrapped("Error: %s", instance.error_message.c_str());
                }
            }

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(right_x, details_y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(right_width, right_half_h), ImGuiCond_Always);
            ImGui::Begin("Details", nullptr,
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize);

            if (!active_experiment_.has_value()) {
                ImGui::TextWrapped("No experiment loaded.");
            } else if (selected_scene_index_ < 0 || selected_scene_index_ >= static_cast<int>(active_experiment_->instances.size())) {
                ImGui::TextWrapped("Select scene to inspect details.");
            } else {
                const auto& instance = active_experiment_->instances[static_cast<std::size_t>(selected_scene_index_)];

                if (ImGui::CollapsingHeader("Scene Overview")) {
                    ImGui::Text("Scene ID: %s", instance.instance_id.c_str());
                    ImGui::Text("Seed: %u", instance.seed);
                    ImGui::Text("Generation: %s", experiment_status_to_text(instance.generation_status).c_str());
                    ImGui::Text("Run: %s", run_status_to_text(instance.run_status).c_str());
                    ImGui::Separator();
                    ImGui::Text("Size: %.1f x %.1f", instance.scene_data.width, instance.scene_data.height);
                    ImGui::Text("Obstacles: %zu", instance.scene_data.obstacles.size());
                    ImGui::Text("Runs: %zu", instance.runs.size());
                    ImGui::Text("Zoom: %.2f", camera_controller_.get_zoom());
                }

                if (ImGui::CollapsingHeader("Environment Metrics")) {
                    draw_optional_metric("Obstacle Area Ratio", instance.scene_metrics.obstacle_area_ratio, true);
                    draw_optional_metric("Boundary Density", instance.scene_metrics.boundary_density);
                    draw_optional_metric("Cell Density CV", instance.scene_metrics.cell_density_cv);
                    draw_optional_metric("Radii CV", instance.scene_metrics.radii_cv);
                    draw_optional_metric("Clearance", instance.scene_metrics.clearance);
                    draw_optional_metric("Largest Component Ratio", instance.scene_metrics.largest_component_ratio, true);
                }

                if (ImGui::CollapsingHeader("Algorithm Results")) {
                    if (instance.runs.empty()) {
                        ImGui::TextWrapped("No algorithm results for this scene yet.");
                    } else {
                        for (const auto& run : instance.runs) {
                            ImGui::Separator();
                            ImGui::Text("%s", run.algorithm_id.c_str());
                            ImGui::Text("Status: %s", algorithm_status_to_text(run.status).c_str());
                            ImGui::Text("Runtime: %.3f ms", run.runtime_ms);
                            draw_optional_metric("Path Length", run.path_metrics.path_length);
                            draw_optional_metric("Relative Path Length", run.path_metrics.relative_path_length);
                            if (run.path.has_value()) {
                                ImGui::Text("Path Points: %zu", run.path->points.size());
                            } else {
                                ImGui::Text("Path Points: N/A");
                            }
                            if (!run.error_message.empty()) {
                                ImGui::TextWrapped("Error: %s", run.error_message.c_str());
                            }
                        }
                    }
                }
            }

            ImGui::End();

            ImGui::SetNextWindowPos(ImVec2(right_x, settings_y), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(right_width, right_half_h), ImGuiCond_Always);
            ImGui::Begin("Settings", nullptr,
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoResize);

            if (ImGui::CollapsingHeader("Render Layers")) {
                ImGui::Checkbox("Show Grid", &show_grid_layer);
                ImGui::Checkbox("Show Obstacles", &show_obstacles_layer);
                ImGui::Checkbox("Show Start/Goal", &show_start_goal_layer);
                ImGui::Checkbox("Show Scrollbars", &show_scrollbars);
            }

            if (ImGui::CollapsingHeader("Navigation")) {
                ImGui::Checkbox("Enable Camera Navigation", &enable_camera_navigation);
                if (ImGui::Button("Reset Camera")) {
                    camera_controller_.reset();
                }
            }

            if (ImGui::CollapsingHeader("Config Folder Batch", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (batch_study_progress == nullptr) {
                    ImGui::TextWrapped("Use File -> Run Config Folder... to process every JSON config in a folder.");
                } else {
                    const BatchStudyProgressSnapshot batch_snapshot =
                        snapshot_batch_study_progress(batch_study_progress);

                    if (!batch_snapshot.selected_folder.empty()) {
                        ImGui::TextWrapped("Folder: %s", batch_snapshot.selected_folder.c_str());
                    }
                    if (!batch_snapshot.output_folder.empty()) {
                        ImGui::TextWrapped("Output: %s", batch_snapshot.output_folder.c_str());
                    }

                    ImGui::Text("Stage: %s", batch_snapshot.current_stage.c_str());
                    if (!batch_snapshot.current_config_file.empty()) {
                        ImGui::TextWrapped("Current Config: %s", batch_snapshot.current_config_file.c_str());
                    }

                    if (batch_snapshot.total_configs > 0) {
                        const float config_progress =
                            static_cast<float>(batch_snapshot.processed_configs) /
                            static_cast<float>(batch_snapshot.total_configs);
                        ImGui::ProgressBar(config_progress, ImVec2(-1.0f, 0.0f));
                        const std::size_t remaining_configs =
                            batch_snapshot.processed_configs <= batch_snapshot.total_configs
                                ? (batch_snapshot.total_configs - batch_snapshot.processed_configs)
                                : 0;
                        ImGui::Text("Processed Configs: %zu / %zu, Remaining: %zu",
                                    batch_snapshot.processed_configs,
                                    batch_snapshot.total_configs,
                                    remaining_configs);
                    }

                    if (batch_snapshot.total_scene_count > 0) {
                        const float scene_progress =
                            static_cast<float>(batch_snapshot.processed_scene_count) /
                            static_cast<float>(batch_snapshot.total_scene_count);
                        ImGui::ProgressBar(scene_progress, ImVec2(-1.0f, 0.0f));
                        const std::size_t remaining_scenes =
                            batch_snapshot.processed_scene_count <= batch_snapshot.total_scene_count
                                ? (batch_snapshot.total_scene_count - batch_snapshot.processed_scene_count)
                                : 0;
                        ImGui::Text("Processed-Config Scenes: %zu / %zu, Remaining: %zu",
                                    batch_snapshot.processed_scene_count,
                                    batch_snapshot.total_scene_count,
                                    remaining_scenes);
                    }

                    if (batch_snapshot.current_scene_total > 0) {
                        const float current_scene_progress =
                            static_cast<float>(batch_snapshot.current_scene_completed) /
                            static_cast<float>(batch_snapshot.current_scene_total);
                        ImGui::ProgressBar(current_scene_progress, ImVec2(-1.0f, 0.0f));
                        const std::size_t remaining_stage_scenes =
                            batch_snapshot.current_scene_completed <= batch_snapshot.current_scene_total
                                ? (batch_snapshot.current_scene_total - batch_snapshot.current_scene_completed)
                                : 0;
                        ImGui::Text("Current Stage Scenes: %zu / %zu, Remaining: %zu",
                                    batch_snapshot.current_scene_completed,
                                    batch_snapshot.current_scene_total,
                                    remaining_stage_scenes);
                    }

                    if (batch_snapshot.current_algorithm_total > 0) {
                        const float algorithm_progress =
                            static_cast<float>(batch_snapshot.current_algorithm_completed) /
                            static_cast<float>(batch_snapshot.current_algorithm_total);
                        ImGui::ProgressBar(algorithm_progress, ImVec2(-1.0f, 0.0f));
                        const std::size_t remaining_algorithm_runs =
                            batch_snapshot.current_algorithm_completed <= batch_snapshot.current_algorithm_total
                                ? (batch_snapshot.current_algorithm_total -
                                   batch_snapshot.current_algorithm_completed)
                                : 0;
                        ImGui::Text("Current Algorithm Runs: %zu / %zu, Remaining: %zu",
                                    batch_snapshot.current_algorithm_completed,
                                    batch_snapshot.current_algorithm_total,
                                    remaining_algorithm_runs);
                    }

                    if (!batch_snapshot.last_saved_file.empty()) {
                        ImGui::TextWrapped("Last Saved: %s", batch_snapshot.last_saved_file.c_str());
                    }
                    if (!batch_snapshot.note.empty()) {
                        ImGui::Separator();
                        ImGui::TextWrapped("%s", batch_snapshot.note.c_str());
                    }
                    if (!batch_folder_task_running) {
                        ImGui::Separator();
                        ImGui::TextWrapped("Batch is idle. Start a new one from File -> Run Config Folder...");
                    }
                }
            }

            if (ImGui::CollapsingHeader("Grid Graph Demo")) {
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("Cell Size", &grid_graph_cell_size, 0.5, 2.0, "%.2f")) {
                    grid_graph_cell_size = std::max(0.2, grid_graph_cell_size);
                    grid_graph_overlay_ready = false;
                    grid_graph_overlay_scene_index = -1;
                }
                if (ImGui::Checkbox("Grid Diagonal", &grid_graph_allow_diagonal)) {
                    grid_graph_overlay_ready = false;
                    grid_graph_overlay_scene_index = -1;
                }

                if (!can_draw_scene) {
                    ImGui::TextWrapped("Select a generated scene to build grid graph.");
                } else {
                    if (ImGui::Button("Build Grid Graph")) {
                        try {
                            const experiment::SceneInstance* scene_for_overlay = selected_scene_instance();
                            if (scene_for_overlay == nullptr ||
                                scene_for_overlay->generation_status != experiment::SceneGenerationStatus::Generated) {
                                throw std::runtime_error("No generated scene selected");
                            }

                            grid_graph_builder.set_grid_step(std::max(0.2, grid_graph_cell_size));
                            grid_graph_builder.set_allow_diagonal(grid_graph_allow_diagonal);
                            grid_graph_overlay = grid_graph_builder.build(scene_for_overlay->scene_data);
                            grid_graph_overlay_ready = true;
                            grid_graph_overlay_scene_index = selected_scene_index_;

                            std::size_t edge_count = 0;
                            for (const auto& adjacency : grid_graph_overlay.adj) {
                                edge_count += adjacency.size();
                            }
                            edge_count /= 2;
                            status_message_ = "Grid graph built: nodes=" +
                                              std::to_string(grid_graph_overlay.adj.size()) +
                                              ", edges=" + std::to_string(edge_count) +
                                              ", cell=" + std::to_string(grid_graph_cell_size);
                        } catch (const std::exception& e) {
                            grid_graph_overlay_ready = false;
                            grid_graph_overlay_scene_index = -1;
                            status_message_ = std::string("Grid graph build failed: ") + e.what();
                        }
                    }

                    if (grid_graph_overlay_ready &&
                        grid_graph_overlay_scene_index == selected_scene_index_) {
                        ImGui::TextWrapped("Grid overlay is visible for current scene.");
                    } else {
                        ImGui::TextWrapped("Grid overlay is not built for current scene.");
                    }
                }
            }

            if (ImGui::CollapsingHeader("Tangent Visibility Graph Demo")) {
                if (!can_draw_scene) {
                    ImGui::TextWrapped("Select a generated scene to build tangent graph.");
                } else {
                    if (ImGui::Button("Build Tangent Graph")) {
                        try {
                            const experiment::SceneInstance* scene_for_overlay = selected_scene_instance();
                            if (scene_for_overlay == nullptr ||
                                scene_for_overlay->generation_status != experiment::SceneGenerationStatus::Generated) {
                                throw std::runtime_error("No generated scene selected");
                            }

                            tangent_visibility_overlay = build_tangent_visibility_overlay(scene_for_overlay->scene_data);
                            tangent_visibility_overlay_ready = true;
                            tangent_visibility_overlay_scene_index = selected_scene_index_;

                            status_message_ = "Tangent graph built: tangents=" +
                                              std::to_string(tangent_visibility_overlay.tangent_segments.size()) +
                                              ", arcs=" + std::to_string(tangent_visibility_overlay.boundary_arcs.size());
                        } catch (const std::exception& e) {
                            tangent_visibility_overlay_ready = false;
                            tangent_visibility_overlay_scene_index = -1;
                            status_message_ = std::string("Tangent graph build failed: ") + e.what();
                        }
                    }

                    if (tangent_visibility_overlay_ready &&
                        tangent_visibility_overlay_scene_index == selected_scene_index_) {
                        ImGui::TextWrapped("Overlay is visible for current scene.");
                    } else {
                        ImGui::TextWrapped("Overlay is not built for current scene.");
                    }
                }
            }

            ImGui::Separator();
            if (ImGui::CollapsingHeader("RRT Demo")) {
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputInt("RRT Tick (ms)", &rrt_tick_ms)) {
                    rrt_tick_ms = std::max(1, rrt_tick_ms);
                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputInt("RRT Max Iterations", &rrt_max_iterations)) {
                    rrt_max_iterations = std::max(1, rrt_max_iterations);
                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("RRT Step Size", &rrt_step_size, 0.1, 0.5, "%.2f")) {
                    rrt_step_size = std::max(0.05, rrt_step_size);
                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("RRT Goal Sample Rate", &rrt_goal_sample_rate, 0.01, 0.05, "%.2f")) {
                    rrt_goal_sample_rate = std::clamp(rrt_goal_sample_rate, 0.0, 1.0);
                }
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::InputDouble("RRT Goal Tolerance", &rrt_goal_tolerance, 0.1, 0.5, "%.2f")) {
                    rrt_goal_tolerance = std::max(0.05, rrt_goal_tolerance);
                }

                ImGui::Checkbox("RRT Use Scene Seed", &rrt_use_scene_seed);
                if (!rrt_use_scene_seed) {
                    ImGui::SetNextItemWidth(-1.0f);
                    ImGui::InputInt("RRT Seed", &rrt_custom_seed);
                }

                ImGui::Checkbox("RRT Show Tree", &rrt_show_tree);
                ImGui::Checkbox("RRT Show Path", &rrt_show_path);
                if (rrt_demo_state.status != RRTDemoStatus::Idle &&
                    rrt_demo_state.bound_scene_index == selected_scene_index_) {
                    rrt_demo_state.config.show_tree = rrt_show_tree;
                    rrt_demo_state.config.show_path = rrt_show_path;
                }

                if (!can_draw_scene) {
                    ImGui::TextWrapped("Select a generated scene to run RRT demo.");
                } else {
                    if (rrt_demo_state.status == RRTDemoStatus::Running) {
                        if (ImGui::Button("Stop RRT Demo")) {
                            reset_rrt_demo(rrt_demo_state);
                            status_message_ = "RRT demo stopped.";
                        }
                    } else {
                        if (ImGui::Button("Start RRT Demo")) {
                            const experiment::SceneInstance* scene_for_rrt = selected_scene_instance();
                            if (scene_for_rrt == nullptr ||
                                scene_for_rrt->generation_status != experiment::SceneGenerationStatus::Generated) {
                                status_message_ = "RRT demo start failed: no generated scene selected.";
                            } else {
                                RRTDemoConfig config;
                                config.tick_ms = std::max(1, rrt_tick_ms);
                                config.max_iterations = static_cast<std::size_t>(std::max(1, rrt_max_iterations));
                                config.step_size = std::max(0.05, rrt_step_size);
                                config.goal_sample_rate = std::clamp(rrt_goal_sample_rate, 0.0, 1.0);
                                config.goal_tolerance = std::max(0.05, rrt_goal_tolerance);
                                config.seed = rrt_use_scene_seed
                                    ? scene_for_rrt->seed
                                    : static_cast<std::uint32_t>(std::max(0, rrt_custom_seed));
                                config.show_tree = rrt_show_tree;
                                config.show_path = rrt_show_path;

                                std::string rrt_error;
                                if (start_rrt_demo(rrt_demo_state,
                                                   scene_for_rrt->scene_data,
                                                   selected_scene_index_,
                                                   config,
                                                   rrt_error)) {
                                    status_message_ = "RRT demo started.";
                                    if (rrt_demo_state.status == RRTDemoStatus::Succeeded) {
                                        status_message_ = "RRT demo: direct path found immediately.";
                                    }
                                } else {
                                    status_message_ = rrt_error;
                                }
                            }
                        }
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Reset RRT Demo")) {
                        reset_rrt_demo(rrt_demo_state);
                        status_message_ = "RRT demo reset.";
                    }

                    if (rrt_demo_state.status != RRTDemoStatus::Idle &&
                        rrt_demo_state.bound_scene_index == selected_scene_index_) {
                        const char* rrt_status_text = "Idle";
                        if (rrt_demo_state.status == RRTDemoStatus::Running) {
                            rrt_status_text = "Running";
                        } else if (rrt_demo_state.status == RRTDemoStatus::Succeeded) {
                            rrt_status_text = "Succeeded";
                        } else if (rrt_demo_state.status == RRTDemoStatus::Failed) {
                            rrt_status_text = "Failed";
                        }

                        ImGui::Text("RRT Status: %s", rrt_status_text);
                        ImGui::Text("RRT Iterations: %zu / %zu",
                                    rrt_demo_state.iterations_done,
                                    rrt_demo_state.config.max_iterations);
                        ImGui::Text("RRT Nodes: %zu", rrt_demo_state.nodes.size());
                    } else {
                        ImGui::TextWrapped("RRT demo is not started for current scene.");
                    }
                }
            }

            if (ImGui::CollapsingHeader("Algorithm Benchmark")) {
                if (!active_experiment_.has_value() || active_experiment_->instances.empty()) {
                    ImGui::TextWrapped("Load or generate scenes to run benchmark.");
                } else {
                    std::size_t generated_scene_count = 0;
                    std::size_t enabled_algorithm_count = 0;
                    for (const auto& instance : active_experiment_->instances) {
                        if (instance.generation_status == experiment::SceneGenerationStatus::Generated) {
                            ++generated_scene_count;
                        }
                    }
                    for (const auto& algorithm : active_experiment_->algorithms) {
                        if (algorithm.enabled) {
                            ++enabled_algorithm_count;
                        }
                    }

                    ImGui::Text("Generated Scenes: %zu", generated_scene_count);
                    ImGui::Text("Enabled Algorithms: %zu", enabled_algorithm_count);
                    const experiment::SceneInstance* selected_scene = selected_scene_instance();
                    const bool selected_scene_ready =
                        selected_scene != nullptr &&
                        selected_scene->generation_status == experiment::SceneGenerationStatus::Generated;
                    if (selected_scene_ready) {
                        ImGui::Text("Selected Scene: %s", selected_scene->instance_id.c_str());
                    } else {
                        ImGui::TextWrapped("Selected Scene: none or not generated");
                    }

                    if (algorithm_run_task_running) {
                        const std::size_t completed = algorithm_run_task_completed != nullptr
                            ? algorithm_run_task_completed->load()
                            : 0;
                        const float progress = algorithm_run_task_total == 0
                            ? 0.0f
                            : static_cast<float>(completed) / static_cast<float>(algorithm_run_task_total);
                        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
                        const std::size_t remaining = completed <= algorithm_run_task_total
                            ? (algorithm_run_task_total - completed)
                            : 0;
                        ImGui::Text("Completed Runs: %zu / %zu, Remaining: %zu",
                                    completed,
                                    algorithm_run_task_total,
                                    remaining);
                        ImGui::Text("Run Scope: %s",
                                    algorithm_run_task_scope == AlgorithmRunScope::SelectedScene
                                        ? "selected scene"
                                        : "all scenes");
                    } else {
                        if (ImGui::Button("Run All Algorithms")) {
                            if (metrics_task_running_ || batch_folder_task_running) {
                                status_message_ =
                                    "Cannot run algorithms while metrics or config-folder batch are running.";
                            } else {
                                algorithm_run_task_total =
                                    experiment::ExperimentService::estimate_total_algorithm_runs(active_experiment_.value());
                                if (algorithm_run_task_total == 0) {
                                    status_message_ =
                                        "No benchmark runs available. Check generated scenes and enabled algorithms.";
                                } else {
                                    algorithm_run_task_completed = std::make_shared<std::atomic<std::size_t>>(0);
                                    experiment::Experiment snapshot = active_experiment_.value();
                                    auto progress_counter = algorithm_run_task_completed;

                                    algorithm_run_task_future = std::async(
                                        std::launch::async,
                                        [snapshot = std::move(snapshot), progress_counter]() mutable {
                                            experiment::ExperimentService::run_algorithms(
                                                snapshot,
                                                progress_counter.get());
                                            return snapshot;
                                        });

                                    algorithm_run_task_running = true;
                                    algorithm_run_task_scope = AlgorithmRunScope::AllScenes;
                                    algorithm_run_task_scene_index = -1;
                                    status_message_ = "Algorithm benchmark started...";
                                }
                            }
                        }

                        if (selected_scene_ready) {
                            ImGui::SameLine();
                            if (ImGui::Button("Run Selected Scene")) {
                                if (metrics_task_running_ || batch_folder_task_running) {
                                    status_message_ =
                                        "Cannot run algorithms while metrics or config-folder batch are running.";
                                } else {
                                    const std::size_t scene_index = static_cast<std::size_t>(selected_scene_index_);
                                    algorithm_run_task_total =
                                        experiment::ExperimentService::estimate_total_algorithm_runs_for_scene(
                                            active_experiment_.value(),
                                            scene_index);
                                    if (algorithm_run_task_total == 0) {
                                        status_message_ = "No runs for selected scene. Check scene and enabled algorithms.";
                                    } else {
                                        algorithm_run_task_completed = std::make_shared<std::atomic<std::size_t>>(0);
                                        experiment::Experiment snapshot = active_experiment_.value();
                                        auto progress_counter = algorithm_run_task_completed;

                                        algorithm_run_task_future = std::async(
                                            std::launch::async,
                                            [snapshot = std::move(snapshot), scene_index, progress_counter]() mutable {
                                                experiment::ExperimentService::run_algorithms_for_scene(
                                                    snapshot,
                                                    scene_index,
                                                    progress_counter.get());
                                                return snapshot;
                                            });

                                        algorithm_run_task_running = true;
                                        algorithm_run_task_scope = AlgorithmRunScope::SelectedScene;
                                        algorithm_run_task_scene_index = static_cast<int>(scene_index);
                                        status_message_ = "Selected scene benchmark started...";
                                    }
                                }
                            }
                        }
                    }

                    if (ImGui::Button("Export Benchmark JSON")) {
                        const std::string default_name =
                            active_experiment_->experiment_name.empty()
                                ? "benchmark_report.json"
                                : active_experiment_->experiment_name + "_benchmark_report.json";
                        const auto selected_path =
                            show_save_json_dialog("Export Benchmark Report", default_name.c_str());
                        if (selected_path.has_value()) {
                            if (experiment::ExperimentService::save_algorithm_report_json(
                                active_experiment_.value(), selected_path.value())) {
                                status_message_ = "Benchmark report saved: " + selected_path.value();
                            } else {
                                status_message_ = "Failed to save benchmark report.";
                            }
                        }
                    }

                    ImGui::Separator();
                    ImGui::Checkbox("Show Algorithm Paths Overlay", &show_algorithm_paths_overlay);
                    if (!selected_scene_ready) {
                        ImGui::TextWrapped("Select a generated scene to inspect algorithm paths.");
                    } else if (selected_scene->runs.empty()) {
                        ImGui::TextWrapped("No algorithm results for selected scene yet.");
                    } else {
                        ImGui::Text("Selected Scene Paths");
                        for (std::size_t run_index = 0; run_index < selected_scene->runs.size(); ++run_index) {
                            const auto& run = selected_scene->runs[run_index];
                            if (!run.path.has_value()) {
                                continue;
                            }

                            const std::string visibility_key =
                                selected_scene->instance_id + "::" + run.algorithm_id;
                            auto it = algorithm_path_visibility.find(visibility_key);
                            if (it == algorithm_path_visibility.end()) {
                                it = algorithm_path_visibility.emplace(visibility_key, true).first;
                            }
                            bool visible = it->second;
                            const std::string checkbox_label =
                                run.algorithm_id + "##path_vis_" + std::to_string(run_index);
                            if (ImGui::Checkbox(checkbox_label.c_str(), &visible)) {
                                it->second = visible;
                            }
                            const sf::Color color = algorithm_path_color_by_index(run_index);
                            const ImVec4 im_color(
                                static_cast<float>(color.r) / 255.0f,
                                static_cast<float>(color.g) / 255.0f,
                                static_cast<float>(color.b) / 255.0f,
                                1.0f);
                            ImGui::SameLine();
                            const std::string color_id = "##path_color_" + std::to_string(run_index);
                            ImGui::ColorButton(color_id.c_str(), im_color,
                                               ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
                                               ImVec2(10.0f, 10.0f));
                            ImGui::SameLine();
                            if (run.path_metrics.path_length.has_value()) {
                                ImGui::Text("len=%.3f, t=%.3f ms",
                                            run.path_metrics.path_length.value(),
                                            run.runtime_ms);
                            } else {
                                ImGui::Text("len=N/A, t=%.3f ms", run.runtime_ms);
                            }
                        }
                    }
                }
            }

            if (ImGui::CollapsingHeader("Metrics")) {
                if (!active_experiment_.has_value() || active_experiment_->instances.empty()) {
                    ImGui::TextWrapped("Load or generate scenes to enable metrics calculation.");
                } else if (metrics_task_running_) {
                    const std::size_t completed = metrics_task_completed_ != nullptr
                        ? metrics_task_completed_->load()
                        : 0;
                    const float progress = metrics_task_total_ == 0
                        ? 0.0f
                        : static_cast<float>(completed) / static_cast<float>(metrics_task_total_);
                    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
                    const std::size_t remaining = (completed <= metrics_task_total_)
                        ? (metrics_task_total_ - completed)
                        : 0;
                    ImGui::Text("Computed: %zu / %zu, Remaining: %zu",
                                completed,
                                metrics_task_total_,
                                remaining);
                } else {
                    if (algorithm_run_task_running || batch_folder_task_running) {
                        ImGui::TextWrapped(
                            "Wait for algorithm benchmark or config-folder batch completion to compute metrics.");
                    } else {
                        if (ImGui::Button("Compute Metrics")) {
                            metrics_task_total_ = active_experiment_->instances.size();
                            metrics_task_completed_ = std::make_shared<std::atomic<std::size_t>>(0);

                            const auto instances_snapshot = active_experiment_->instances;
                            const auto metrics_config = active_experiment_->generator_config_snapshot.metrics;
                            auto progress_counter = metrics_task_completed_;

                            metrics_task_future_ = std::async(
                                std::launch::async,
                                [instances = std::move(instances_snapshot), metrics_config, progress_counter]() mutable {
                                    experiment::SceneMetricsBatchResult result =
                                        experiment::SceneMetricsBatchCalculator::compute(
                                            instances,
                                            metrics_config,
                                            progress_counter.get());
                                    return result.metrics;
                                });

                            metrics_task_running_ = true;
                            status_message_ = "Metrics calculation started...";
                        }
                    }
                    ImGui::TextWrapped("Use generator config metrics section to control sampling, method and threading.");
                }

                if (active_experiment_.has_value()) {
                    const auto& metrics_cfg = active_experiment_->generator_config_snapshot.metrics;
                    ImGui::Separator();
                    ImGui::Text("Current Metrics Parameters");
                    ImGui::Text("Auto Compute on Generation: %s",
                                metrics_cfg.auto_compute_on_generation ? "true" : "false");
                    ImGui::Text("Execution Mode: %s",
                                metrics_execution_mode_to_text(metrics_cfg.execution_mode).c_str());
                    if (metrics_cfg.worker_threads == 0) {
                        ImGui::Text("Worker Threads: auto");
                    } else {
                        ImGui::Text("Worker Threads: %zu", metrics_cfg.worker_threads);
                    }

                    ImGui::Separator();
                    ImGui::Text("Enabled Metrics");
                    ImGui::Text("Obstacle Area Ratio: %s", metrics_cfg.compute_obstacle_area_ratio ? "on" : "off");
                    ImGui::Text("Boundary Density: %s", metrics_cfg.compute_boundary_density ? "on" : "off");
                    ImGui::Text("Cell Density CV: %s", metrics_cfg.compute_cell_density_cv ? "on" : "off");
                    ImGui::Text("Radii CV: %s", metrics_cfg.compute_radii_cv ? "on" : "off");
                    ImGui::Text("Clearance: %s", metrics_cfg.compute_clearance ? "on" : "off");
                    ImGui::Text("Largest Component Ratio: %s",
                                metrics_cfg.compute_largest_component_ratio ? "on" : "off");

                    ImGui::Separator();
                    ImGui::Text("Sampling/Discretization");
                    ImGui::Text("Obstacle Area Method: %s",
                                obstacle_area_method_to_text(metrics_cfg.obstacle_area_ratio_method).c_str());
                    ImGui::Text("Obstacle Area Samples: %zu", metrics_cfg.obstacle_area_samples);
                    ImGui::Text("Obstacle Area Grid Resolution: %zu", metrics_cfg.obstacle_area_grid_resolution);
                    ImGui::Text("Boundary Angle Samples: %zu", metrics_cfg.boundary_angle_samples);
                    ImGui::Text("Cell Grid: %zu x %zu", metrics_cfg.cell_grid_x, metrics_cfg.cell_grid_y);
                    ImGui::Text("Cell Density Samples: %zu", metrics_cfg.cell_density_samples);
                    ImGui::Text("Clearance Grid: %zu x %zu",
                                metrics_cfg.clearance_grid_x,
                                metrics_cfg.clearance_grid_y);
                    ImGui::Text("Clearance Quantile: %.3f", metrics_cfg.clearance_quantile);
                    ImGui::Text("Clearance Region Mode: %s",
                                clearance_region_mode_to_text(metrics_cfg.clearance_region_mode).c_str());
                    ImGui::Text("Connectivity Grid: %zu x %zu",
                                metrics_cfg.connectivity_grid_x,
                                metrics_cfg.connectivity_grid_y);
                }
            }

            ImGui::Separator();
            ImGui::TextWrapped("These controls are contextual and can be expanded later.");
            ImGui::End();

            ui_manager_.render(window);

            if (ui_manager_.consume_create_experiment_request()) {
                if (metrics_task_running_ || algorithm_run_task_running || batch_folder_task_running) {
                    status_message_ =
                        "Cannot create experiment while background calculations or config-folder batch are running.";
                } else {
                    try {
                        active_experiment_ = experiment::ExperimentService::create_empty_experiment("untitled_experiment");
                        active_experiment_path_ = std::nullopt;
                        selected_scene_index_ = -1;
                        tangent_visibility_overlay_ready = false;
                        tangent_visibility_overlay_scene_index = -1;
                        grid_graph_overlay_ready = false;
                        grid_graph_overlay_scene_index = -1;
                        reset_rrt_demo(rrt_demo_state);
                        algorithm_path_visibility.clear();
                        status_message_ = "Experiment created. Open config to generate scenes.";
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Create failed: ") + e.what();
                    }
                }
            }

            if (ui_manager_.consume_open_config_request()) {
                if (metrics_task_running_ || algorithm_run_task_running || batch_folder_task_running) {
                    status_message_ =
                        "Cannot open config while background calculations or config-folder batch are running.";
                } else {
                    try {
                        const auto config_path = show_open_json_dialog("Open Generator Config");
                        if (config_path.has_value()) {
                            const auto config = experiment::GeneratorConfigIO::load_from_file(config_path.value());

                            if (!active_experiment_.has_value()) {
                                const std::string experiment_name =
                                    config.name.empty() ? "untitled_experiment" : config.name;
                                active_experiment_ = experiment::ExperimentService::create_empty_experiment(experiment_name);
                            }

                            active_experiment_->source_config_path = config_path.value();
                            active_experiment_->generator_config_snapshot = config;
                            active_experiment_->algorithms = config.algorithms;
                            active_experiment_->global_seed = config.global_seed;

                            experiment::ExperimentService::generate_instances(active_experiment_.value());
                            active_experiment_path_ = std::nullopt;
                            select_first_generated_scene();
                            tangent_visibility_overlay_ready = false;
                            tangent_visibility_overlay_scene_index = -1;
                            grid_graph_overlay_ready = false;
                            grid_graph_overlay_scene_index = -1;
                            reset_rrt_demo(rrt_demo_state);
                            algorithm_path_visibility.clear();

                            std::size_t generated_count = 0;
                            for (const auto& instance : active_experiment_->instances) {
                                if (instance.generation_status == experiment::SceneGenerationStatus::Generated) {
                                    ++generated_count;
                                }
                            }
                            status_message_ = "Generated scenes: " + std::to_string(generated_count) +
                                              " of " + std::to_string(active_experiment_->instances.size());
                            if (!active_experiment_->generator_config_snapshot.metrics.auto_compute_on_generation) {
                                status_message_ += ". Run 'Compute Metrics' in the Settings panel.";
                            }
                        }
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Open config failed: ") + e.what();
                    }
                }
            }

            if (ui_manager_.consume_run_config_folder_request()) {
                if (metrics_task_running_ || algorithm_run_task_running || batch_folder_task_running) {
                    status_message_ =
                        "Cannot start config-folder batch while another background task is running.";
                } else {
                    try {
                        const auto selected_folder = show_select_folder_dialog("Select Config Folder");
                        if (selected_folder.has_value()) {
                            batch_study_progress = std::make_shared<BatchStudyProgressState>();
                            const std::filesystem::path folder_path(selected_folder.value());
                            batch_folder_task_future = std::async(
                                std::launch::async,
                                [folder_path, progress = batch_study_progress]() {
                                    return run_config_folder_batch(folder_path, progress);
                                });
                            batch_folder_task_running = true;
                            status_message_ = "Config-folder batch started...";
                        }
                    } catch (const std::exception& e) {
                        batch_study_progress.reset();
                        status_message_ = std::string("Config-folder batch start failed: ") + e.what();
                    }
                }
            }

            if (ui_manager_.consume_open_experiment_request()) {
                if (metrics_task_running_ || algorithm_run_task_running || batch_folder_task_running) {
                    status_message_ =
                        "Cannot open experiment while background calculations or config-folder batch are running.";
                } else {
                    try {
                        const auto experiment_path = show_open_json_dialog("Open Experiment File");
                        if (experiment_path.has_value()) {
                            active_experiment_ = experiment::ExperimentIO::load_from_file(experiment_path.value());
                            active_experiment_path_ = std::filesystem::path(experiment_path.value());
                            select_first_generated_scene();
                            tangent_visibility_overlay_ready = false;
                            tangent_visibility_overlay_scene_index = -1;
                            grid_graph_overlay_ready = false;
                            grid_graph_overlay_scene_index = -1;
                            reset_rrt_demo(rrt_demo_state);
                            algorithm_path_visibility.clear();
                            status_message_ = "Experiment loaded: " + active_experiment_->experiment_name;
                        }
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Open failed: ") + e.what();
                    }
                }
            }

            if (ui_manager_.consume_save_experiment_request()) {
                if (!active_experiment_.has_value()) {
                    status_message_ = "No experiment to save.";
                } else if (algorithm_run_task_running || batch_folder_task_running) {
                    status_message_ =
                        "Cannot save experiment while algorithm benchmark or config-folder batch is running.";
                } else {
                    try {
                        std::filesystem::path save_path;
                        if (active_experiment_path_.has_value()) {
                            save_path = active_experiment_path_.value();
                        } else {
                            const std::string default_name =
                                active_experiment_->experiment_name.empty()
                                    ? "experiment.json"
                                    : active_experiment_->experiment_name + ".json";
                            const auto selected_path =
                                show_save_json_dialog("Save Experiment File", default_name.c_str());
                            if (!selected_path.has_value()) {
                                save_path.clear();
                            } else {
                                save_path = std::filesystem::path(selected_path.value());
                            }
                        }

                        if (!save_path.empty()) {
                            if (experiment::ExperimentIO::save_to_file(active_experiment_.value(), save_path.string())) {
                                active_experiment_path_ = save_path;
                                status_message_ = "Experiment saved: " + save_path.string();
                            } else {
                                status_message_ = "Failed to save experiment file.";
                            }
                        }
                    } catch (const std::exception& e) {
                        status_message_ = std::string("Save failed: ") + e.what();
                    }
                }
            }

            if (ui_manager_.consume_save_screenshot_request()) {
                const std::optional<std::string> output_path = show_save_screenshot_dialog();
                if (output_path.has_value()) {
                    if (!save_window_screenshot(window, output_path.value())) {
                        status_message_ = "Failed to save screenshot: " + output_path.value();
                    } else {
                        status_message_ = "Screenshot saved: " + output_path.value();
                    }
                }
            }

            window.display();
        }

        ui_manager_.shutdown();
        return true;
    }

    void SceneVisualizer::apply_preset(const std::string& preset_name) {
        if (preset_name == "default") {
            renderer_.apply_config(RenderConfig::default_preset());
        } else if (preset_name == "alternative") {
            renderer_.apply_config(RenderConfig::alternative_preset());
        }
    }

    void SceneVisualizer::apply_config(const RenderConfig& config) {
        renderer_.apply_config(config);
    }

    const experiment::SceneInstance* SceneVisualizer::selected_scene_instance() const {
        if (!active_experiment_.has_value()) {
            return nullptr;
        }
        if (selected_scene_index_ < 0 ||
            selected_scene_index_ >= static_cast<int>(active_experiment_->instances.size())) {
            return nullptr;
        }
        return &active_experiment_->instances[static_cast<std::size_t>(selected_scene_index_)];
    }

    void SceneVisualizer::select_first_generated_scene() {
        selected_scene_index_ = -1;
        if (!active_experiment_.has_value()) {
            return;
        }

        for (std::size_t i = 0; i < active_experiment_->instances.size(); ++i) {
            if (active_experiment_->instances[i].generation_status == experiment::SceneGenerationStatus::Generated) {
                selected_scene_index_ = static_cast<int>(i);
                camera_controller_.reset();
                return;
            }
        }
    }

} // namespace visualization







