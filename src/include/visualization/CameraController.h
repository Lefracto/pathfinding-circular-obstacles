//
// Created by USER on 26/12/2025.
//

#ifndef VISUALIZATION_CAMERA_CONTROLLER_H
#define VISUALIZATION_CAMERA_CONTROLLER_H

#include "../geometry/Scene.h"
#include "../geometry/Point.h"
#include <SFML/Graphics.hpp>
#include <functional>

namespace visualization {

    /**
     * Manages camera (zoom, panning), scrollbars, and coordinate transformations
     */
    class CameraController {
    public:
        CameraController(unsigned int initial_window_width, unsigned int initial_window_height);

        void reset();
        void set_window_size(unsigned int width, unsigned int height);
        
        // Camera control
        void set_zoom(double zoom);
        double get_zoom() const { return zoom_level_; }
        void zoom_at_point(const sf::Vector2i& mouse_pos, const geometry::Scene& scene, double zoom_factor);
        void pan(const sf::Vector2i& delta, const geometry::Scene& scene);
        
        // Scrollbar control
        bool is_point_on_scrollbar(const sf::Vector2i& point, const geometry::Scene& scene, 
                                   bool& is_horizontal, bool& is_on_thumb);
        void start_scrollbar_drag(const sf::Vector2i& mouse_pos, const geometry::Scene& scene, 
                                  bool is_horizontal, bool is_on_thumb);
        void update_scrollbar_drag(const sf::Vector2i& mouse_pos, const geometry::Scene& scene);
        void stop_scrollbar_drag();
        void draw_scrollbars(sf::RenderWindow& window, const geometry::Scene& scene);
        
        // Coordinate transformations
        sf::Vector2f scene_to_screen(const geometry::Point& p, const geometry::Scene& scene) const;
        geometry::Point screen_to_scene(const sf::Vector2i& screen_pos, const geometry::Scene& scene) const;
        double get_scale(const geometry::Scene& scene) const;
        
        // Event handling
        bool handle_mouse_press(const sf::Vector2i& mouse_pos, const geometry::Scene& scene);
        void handle_mouse_release();
        void handle_mouse_move(const sf::Vector2i& mouse_pos, const geometry::Scene& scene);
        bool is_dragging() const { return is_dragging_; }
        bool is_dragging_scrollbar() const { return is_dragging_scrollbar_h_ || is_dragging_scrollbar_v_; }

    private:
        unsigned int window_width_;
        unsigned int window_height_;
        unsigned int initial_window_width_;
        unsigned int initial_window_height_;
        
        double zoom_level_ = 1.0;
        sf::Vector2f camera_offset_{0.0f, 0.0f};
        
        bool is_dragging_ = false;
        sf::Vector2i last_mouse_pos_{0, 0};
        
        bool is_dragging_scrollbar_h_ = false;
        bool is_dragging_scrollbar_v_ = false;
        float scrollbar_drag_offset_h_ = 0.0f;
        float scrollbar_drag_offset_v_ = 0.0f;

        float max_offset_x(const geometry::Scene& scene) const;
        float max_offset_y(const geometry::Scene& scene) const;
        
        void clamp_camera_offset(const geometry::Scene& scene);
    };

}

#endif //VISUALIZATION_CAMERA_CONTROLLER_H






