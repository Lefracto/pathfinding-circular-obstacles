//
// Created by USER on 26/12/2025.
//

#ifndef VISUALIZATION_SCENE_RENDERER_H
#define VISUALIZATION_SCENE_RENDERER_H

#include "../geometry/Scene.h"
#include "../geometry/Path.h"
#include <SFML/Graphics.hpp>
#include <optional>
#include <functional>

namespace visualization {

    /**
     * Configuration structure for rendering colors and settings
     */
    struct RenderConfig {
        // Grid settings
        double grid_step = 10.0;
        sf::Color grid_color = sf::Color(200, 200, 200, 100);
        
        // Obstacle settings
        sf::Color obstacle_color = sf::Color(255, 255, 200, 120);
        sf::Color obstacle_outline_color = sf::Color(200, 200, 150, 200);
        float obstacle_outline_thickness = 1.0f;
        
        // Start/Goal settings
        sf::Color start_color = sf::Color::Green;
        sf::Color goal_color = sf::Color::Blue;
        sf::Color start_goal_outline_color = sf::Color::Black;
        float start_goal_outline_thickness = 1.0f;
        double start_goal_radius = 5.0;
        
        // Path settings
        sf::Color path_color = sf::Color::Yellow;
        double path_point_radius = 3.0;
        
        /**
         * Get default preset configuration
         */
        static RenderConfig default_preset() {
            RenderConfig config;
            // Values are already set as defaults above
            return config;
        }
        
        /**
         * Get alternative preset with sharper grid and yellow semi-transparent circles
         */
        static RenderConfig alternative_preset() {
            RenderConfig config;
            config.grid_step = 10.0;
            config.grid_color = sf::Color(150, 150, 150, 200);
            config.obstacle_color = sf::Color(255, 255, 0, 150);
            config.obstacle_outline_color = sf::Color(255, 220, 0, 200);
            config.obstacle_outline_thickness = 1.5f;
            config.start_color = sf::Color::Green;
            config.goal_color = sf::Color::Blue;
            config.path_color = sf::Color::Magenta;
            return config;
        }
    };

    /**
     * Handles rendering of scene elements (grid, obstacles, path, etc.)
     */
    class SceneRenderer {
    public:
        SceneRenderer();

        /**
         * Apply a render configuration preset
         */
        void apply_config(const RenderConfig& config);

        // Legacy setters for backward compatibility
        void set_grid_step(double step) { config_.grid_step = step; }
        void set_obstacle_color(const sf::Color& color) { config_.obstacle_color = color; }
        void set_start_color(const sf::Color& color) { config_.start_color = color; }
        void set_goal_color(const sf::Color& color) { config_.goal_color = color; }
        void set_path_color(const sf::Color& color) { config_.path_color = color; }
        void set_grid_color(const sf::Color& color) { config_.grid_color = color; }
        
        /**
         * Get current configuration
         */
        const RenderConfig& get_config() const { return config_; }

        void draw_grid(sf::RenderWindow& window, const geometry::Scene& scene,
                      std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen,
                      std::function<double()> get_scale);
        void draw_obstacles(sf::RenderWindow& window, const geometry::Scene& scene,
                           std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen,
                           std::function<double()> get_scale);
        void draw_start_goal(sf::RenderWindow& window, const geometry::Scene& scene,
                            std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen);
        void draw_path(sf::RenderWindow& window, const geometry::Scene& scene, const geometry::Path& path,
                      std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen);

    private:
        RenderConfig config_;
    };

}

#endif //VISUALIZATION_SCENE_RENDERER_H


