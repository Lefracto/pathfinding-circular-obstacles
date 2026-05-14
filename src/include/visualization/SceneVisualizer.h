#ifndef VISUALIZATION_SCENE_VISUALIZER_H
#define VISUALIZATION_SCENE_VISUALIZER_H

#include "../geometry/Scene.h"
#include "../geometry/Path.h"
#include "../experiment/core/ExperimentModel.h"
#include "SceneRenderer.h"
#include "GraphRenderer.h"
#include "UIManager.h"
#include "CameraController.h"
#include "../algorithms/GraphBuilder.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <memory>
#include <atomic>
#include <future>
#include <string>
#include <functional>
#include <filesystem>

namespace visualization {

    class SceneVisualizer {
    public:
        explicit SceneVisualizer(unsigned int window_width = 800, unsigned int window_height = 600);

        bool visualize(const geometry::Scene& scene,
                      const std::optional<geometry::Path>& path = std::nullopt,
                      bool show_grid = true,
                      bool show_graph = false);

        bool run_experiment_workspace();
        void set_grid_step(double step) { renderer_.set_grid_step(step); }
        void set_obstacle_color(const sf::Color& color) { renderer_.set_obstacle_color(color); }
        void set_start_color(const sf::Color& color) { renderer_.set_start_color(color); }
        void set_goal_color(const sf::Color& color) { renderer_.set_goal_color(color); }
        void set_path_color(const sf::Color& color) { renderer_.set_path_color(color); }
        void set_grid_color(const sf::Color& color) { renderer_.set_grid_color(color); }
        void apply_preset(const std::string& preset_name);
        void apply_config(const RenderConfig& config);
        const RenderConfig& get_config() const { return renderer_.get_config(); }
        void set_graph_builder(std::shared_ptr<algorithms::graph::GraphBuilder> builder,
                              std::function<geometry::Point(std::size_t)> get_node_point) {
            graph_builder_ = builder;
            get_node_point_func_ = get_node_point;
        }

    private:
        unsigned int window_width_;
        unsigned int window_height_;
        
        SceneRenderer renderer_;
        GraphRenderer graph_renderer_;
        UIManager ui_manager_;
        CameraController camera_controller_;
        
        std::shared_ptr<algorithms::graph::GraphBuilder> graph_builder_;
        std::function<geometry::Point(std::size_t)> get_node_point_func_;

        std::optional<experiment::Experiment> active_experiment_;
        std::optional<std::filesystem::path> active_experiment_path_;
        int selected_scene_index_ = -1;
        std::string status_message_;
        bool metrics_task_running_ = false;
        std::size_t metrics_task_total_ = 0;
        std::shared_ptr<std::atomic<std::size_t>> metrics_task_completed_;
        std::future<std::vector<experiment::SceneMetrics>> metrics_task_future_;

        const experiment::SceneInstance* selected_scene_instance() const;
        void select_first_generated_scene();
    };

}

#endif
