#include "../include/visualization/SceneRenderer.h"
#include <cmath>

namespace visualization {

    SceneRenderer::SceneRenderer() : config_(RenderConfig::default_preset()) {}

    void SceneRenderer::apply_config(const RenderConfig& config) {
        config_ = config;
    }

    void SceneRenderer::draw_grid(sf::RenderWindow& window, const geometry::Scene& scene,
                                  std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen,
                                  std::function<double()> get_scale) {
        sf::VertexArray lines(sf::PrimitiveType::Lines);

        for (double x = 0; x <= scene.width; x += config_.grid_step) {
            sf::Vector2f p1 = scene_to_screen({x, 0});
            sf::Vector2f p2 = scene_to_screen({x, scene.height});
            lines.append(sf::Vertex(p1, config_.grid_color));
            lines.append(sf::Vertex(p2, config_.grid_color));
        }

        for (double y = 0; y <= scene.height; y += config_.grid_step) {
            sf::Vector2f p1 = scene_to_screen({0, y});
            sf::Vector2f p2 = scene_to_screen({scene.width, y});
            lines.append(sf::Vertex(p1, config_.grid_color));
            lines.append(sf::Vertex(p2, config_.grid_color));
        }

        window.draw(lines);
    }

    void SceneRenderer::draw_obstacles(sf::RenderWindow& window, const geometry::Scene& scene,
                                       std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen,
                                       std::function<double()> get_scale) {
        double scale = get_scale();

        for (const auto& obstacle : scene.obstacles) {
            sf::CircleShape circle(obstacle.radius * scale);
            circle.setFillColor(config_.obstacle_color);
            circle.setOutlineColor(config_.obstacle_outline_color);
            circle.setOutlineThickness(config_.obstacle_outline_thickness);
            circle.setPosition(scene_to_screen(obstacle.center) - 
                             sf::Vector2f(obstacle.radius * scale, obstacle.radius * scale));
            circle.setPointCount(64);
            window.draw(circle);
        }
    }

    void SceneRenderer::draw_start_goal(sf::RenderWindow& window, const geometry::Scene& scene,
                                         std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen) {
        sf::CircleShape start_circle(config_.start_goal_radius);
        start_circle.setFillColor(config_.start_color);
        start_circle.setOutlineColor(config_.start_goal_outline_color);
        start_circle.setOutlineThickness(config_.start_goal_outline_thickness);
        start_circle.setPointCount(32);
        sf::Vector2f start_pos = scene_to_screen(scene.start);
        start_circle.setPosition(start_pos - sf::Vector2f(config_.start_goal_radius, config_.start_goal_radius));
        window.draw(start_circle);

        sf::CircleShape goal_circle(config_.start_goal_radius);
        goal_circle.setFillColor(config_.goal_color);
        goal_circle.setOutlineColor(config_.start_goal_outline_color);
        goal_circle.setOutlineThickness(config_.start_goal_outline_thickness);
        goal_circle.setPointCount(32);
        sf::Vector2f goal_pos = scene_to_screen(scene.goal);
        goal_circle.setPosition(goal_pos - sf::Vector2f(config_.start_goal_radius, config_.start_goal_radius));
        window.draw(goal_circle);
    }

    void SceneRenderer::draw_path(sf::RenderWindow& window, const geometry::Scene& scene, const geometry::Path& path,
                                  std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen) {
        if (path.points.empty()) return;

        for (std::size_t i = 0; i < path.points.size() - 1; ++i) {
            sf::Vertex line[] = {
                sf::Vertex(scene_to_screen(path.points[i]), config_.path_color),
                sf::Vertex(scene_to_screen(path.points[i + 1]), config_.path_color)
            };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        for (const auto& point : path.points) {
            sf::CircleShape path_point(config_.path_point_radius);
            path_point.setFillColor(config_.path_color);
            path_point.setPointCount(16);
            sf::Vector2f pos = scene_to_screen(point);
            path_point.setPosition(pos - sf::Vector2f(config_.path_point_radius, config_.path_point_radius));
            window.draw(path_point);
        }
    }

}
