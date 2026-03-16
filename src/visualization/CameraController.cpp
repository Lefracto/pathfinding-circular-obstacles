//
// Created by USER on 26/12/2025.
//

#include "../include/visualization/CameraController.h"
#include <algorithm>
#include <cmath>

namespace visualization {

    CameraController::CameraController(unsigned int initial_window_width, unsigned int initial_window_height)
        : window_width_(initial_window_width), window_height_(initial_window_height),
          initial_window_width_(initial_window_width), initial_window_height_(initial_window_height) {}

    void CameraController::reset() {
        zoom_level_ = 1.0;
        camera_offset_ = sf::Vector2f(0.0f, 0.0f);
        is_dragging_ = false;
        is_dragging_scrollbar_h_ = false;
        is_dragging_scrollbar_v_ = false;
        scrollbar_drag_offset_h_ = 0.0f;
        scrollbar_drag_offset_v_ = 0.0f;
    }

    void CameraController::set_window_size(unsigned int width, unsigned int height) {
        window_width_ = width;
        window_height_ = height;
        if (width > 0 && height > 0) {
            initial_window_width_ = width;
            initial_window_height_ = height;
        }
    }

    void CameraController::set_zoom(double zoom) {
        const double min_zoom = 0.5;
        const double max_zoom = 4.0;
        zoom_level_ = std::max(min_zoom, std::min(max_zoom, zoom));
    }

    void CameraController::zoom_at_point(const sf::Vector2i& mouse_pos, const geometry::Scene& scene, double zoom_factor) {
        geometry::Point scene_point_before = screen_to_scene(mouse_pos, scene);
        
        double old_zoom = zoom_level_;
        zoom_level_ *= zoom_factor;
        const double min_zoom = 0.5;
        const double max_zoom = 4.0;
        if (zoom_level_ < min_zoom) zoom_level_ = min_zoom;
        if (zoom_level_ > max_zoom) zoom_level_ = max_zoom;
        
        geometry::Point scene_point_after = screen_to_scene(mouse_pos, scene);
        
        camera_offset_.x -= static_cast<float>(scene_point_after.x - scene_point_before.x);
        camera_offset_.y -= static_cast<float>(scene_point_after.y - scene_point_before.y);
        
        clamp_camera_offset(scene);
    }

    void CameraController::pan(const sf::Vector2i& delta, const geometry::Scene& scene) {
        double base_scale = std::min(static_cast<double>(initial_window_width_) / scene.width,
                                    static_cast<double>(initial_window_height_) / scene.height);
        double scale = base_scale * zoom_level_;
        
        camera_offset_.x -= static_cast<float>(delta.x / scale);
        camera_offset_.y += static_cast<float>(delta.y / scale);
        
        clamp_camera_offset(scene);
    }

    bool CameraController::is_point_on_scrollbar(const sf::Vector2i& point, const geometry::Scene& scene, 
                                                  bool& is_horizontal, bool& is_on_thumb) {
        const float scrollbar_size = 20.0f;
        double scale = get_scale(scene);
        double scene_width_px = scene.width * scale;
        double scene_height_px = scene.height * scale;
        
        float max_offset_x = this->max_offset_x(scene);
        float max_offset_y = this->max_offset_y(scene);
        
        // Check horizontal scrollbar
        if (scene_width_px > window_width_ - scrollbar_size) {
            double view_width = window_width_ - scrollbar_size;
            double scrollbar_width = view_width;
            double thumb_width = (view_width / scene_width_px) * scrollbar_width;
            double thumb_pos = ((camera_offset_.x + max_offset_x) / (2.0 * max_offset_x)) * (scrollbar_width - thumb_width);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_width - thumb_width));
            
            if (point.y >= window_height_ - scrollbar_size && point.y < window_height_ &&
                point.x >= 0 && point.x < window_width_ - scrollbar_size) {
                is_horizontal = true;
                is_on_thumb = (point.x >= thumb_pos && point.x < thumb_pos + thumb_width);
                return true;
            }
        }
        
        // Check vertical scrollbar
        if (scene_height_px > window_height_ - scrollbar_size) {
            double view_height = window_height_ - scrollbar_size;
            double scrollbar_height = view_height;
            double thumb_height = (view_height / scene_height_px) * scrollbar_height;
            double thumb_pos = ((max_offset_y - camera_offset_.y) / (2.0 * max_offset_y)) * (scrollbar_height - thumb_height);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_height - thumb_height));
            
            if (point.x >= window_width_ - scrollbar_size && point.x < window_width_ &&
                point.y >= 0 && point.y < window_height_ - scrollbar_size) {
                is_horizontal = false;
                is_on_thumb = (point.y >= thumb_pos && point.y < thumb_pos + thumb_height);
                return true;
            }
        }
        
        return false;
    }

    void CameraController::start_scrollbar_drag(const sf::Vector2i& mouse_pos, const geometry::Scene& scene, 
                                                 bool is_horizontal, bool is_on_thumb) {
        double scale = get_scale(scene);
        const float scrollbar_size = 20.0f;
        float max_offset_x = this->max_offset_x(scene);
        float max_offset_y = this->max_offset_y(scene);
        
        if (is_horizontal) {
            is_dragging_scrollbar_h_ = true;
            double view_width = window_width_ - scrollbar_size;
            double scene_width_px = scene.width * scale;
            double scrollbar_width = view_width;
            double thumb_width = (view_width / scene_width_px) * scrollbar_width;
            double thumb_pos = ((camera_offset_.x + max_offset_x) / (2.0 * max_offset_x)) * (scrollbar_width - thumb_width);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_width - thumb_width));
            
            if (is_on_thumb) {
                scrollbar_drag_offset_h_ = static_cast<float>(mouse_pos.x - (thumb_pos + thumb_width / 2.0));
            } else {
                scrollbar_drag_offset_h_ = 0.0f;
                double ratio = std::max(0.0, std::min(1.0, (mouse_pos.x - thumb_width / 2.0) / (scrollbar_width - thumb_width)));
                camera_offset_.x = static_cast<float>(-max_offset_x + (2.0 * max_offset_x * ratio));
            }
        } else {
            is_dragging_scrollbar_v_ = true;
            double view_height = window_height_ - scrollbar_size;
            double scene_height_px = scene.height * scale;
            double scrollbar_height = view_height;
            double thumb_height = (view_height / scene_height_px) * scrollbar_height;
            double thumb_pos = ((max_offset_y - camera_offset_.y) / (2.0 * max_offset_y)) * (scrollbar_height - thumb_height);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_height - thumb_height));
            
            if (is_on_thumb) {
                scrollbar_drag_offset_v_ = static_cast<float>(mouse_pos.y - (thumb_pos + thumb_height / 2.0));
            } else {
                scrollbar_drag_offset_v_ = 0.0f;
                double ratio = std::max(0.0, std::min(1.0, (mouse_pos.y - thumb_height / 2.0) / (scrollbar_height - thumb_height)));
                camera_offset_.y = static_cast<float>(-max_offset_y + (2.0 * max_offset_y * (1.0 - ratio)));
            }
        }
    }

    void CameraController::update_scrollbar_drag(const sf::Vector2i& mouse_pos, const geometry::Scene& scene) {
        double scale = get_scale(scene);
        double scene_width_px = scene.width * scale;
        double scene_height_px = scene.height * scale;
        const float scrollbar_size = 20.0f;
        
        if (is_dragging_scrollbar_h_) {
            double view_width = window_width_ - scrollbar_size;
            double scrollbar_width = view_width;
            double thumb_width = (view_width / scene_width_px) * scrollbar_width;
            double max_pos = scrollbar_width - thumb_width;
            
            double thumb_center_x = mouse_pos.x - scrollbar_drag_offset_h_;
            double thumb_pos = thumb_center_x - thumb_width / 2.0;
            thumb_pos = std::max(0.0, std::min(thumb_pos, max_pos));
            
            double ratio = thumb_pos / max_pos;
            
            float max_offset_x = this->max_offset_x(scene);
            camera_offset_.x = static_cast<float>(-max_offset_x + (2.0 * max_offset_x * ratio));
            camera_offset_.x = std::max(-max_offset_x, std::min(camera_offset_.x, max_offset_x));
        }
        
        if (is_dragging_scrollbar_v_) {
            double view_height = window_height_ - scrollbar_size;
            double scrollbar_height = view_height;
            double thumb_height = (view_height / scene_height_px) * scrollbar_height;
            double max_pos = scrollbar_height - thumb_height;
            
            double thumb_center_y = mouse_pos.y - scrollbar_drag_offset_v_;
            double thumb_pos = thumb_center_y - thumb_height / 2.0;
            thumb_pos = std::max(0.0, std::min(thumb_pos, max_pos));
            
            double ratio = thumb_pos / max_pos;
            
            float max_offset_y = this->max_offset_y(scene);
            camera_offset_.y = static_cast<float>(-max_offset_y + (2.0 * max_offset_y * (1.0 - ratio)));
            camera_offset_.y = std::max(-max_offset_y, std::min(camera_offset_.y, max_offset_y));
        }
    }

    void CameraController::stop_scrollbar_drag() {
        is_dragging_scrollbar_h_ = false;
        is_dragging_scrollbar_v_ = false;
    }

    void CameraController::draw_scrollbars(sf::RenderWindow& window, const geometry::Scene& scene) {
        const float scrollbar_size = 20.0f;
        double scale = get_scale(scene);
        double scene_width_px = scene.width * scale;
        double scene_height_px = scene.height * scale;
        
        float max_offset_x = this->max_offset_x(scene);
        float max_offset_y = this->max_offset_y(scene);
        
        // Draw horizontal scrollbar (bottom)
        if (scene_width_px > window_width_ - scrollbar_size) {
            double view_width = window_width_ - scrollbar_size;
            double scrollbar_width = view_width;
            double thumb_width = (view_width / scene_width_px) * scrollbar_width;
            double thumb_pos = ((camera_offset_.x + max_offset_x) / (2.0 * max_offset_x)) * (scrollbar_width - thumb_width);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_width - thumb_width));
            
            sf::RectangleShape bg(sf::Vector2f(scrollbar_width, scrollbar_size));
            bg.setPosition(sf::Vector2f(0.0f, window_height_ - scrollbar_size));
            bg.setFillColor(sf::Color(240, 240, 240, 255));
            window.draw(bg);
            
            sf::RectangleShape thumb(sf::Vector2f(static_cast<float>(thumb_width), scrollbar_size));
            thumb.setPosition(sf::Vector2f(static_cast<float>(thumb_pos), window_height_ - scrollbar_size));
            thumb.setFillColor(sf::Color(180, 180, 180, 255));
            window.draw(thumb);
        }
        
        // Draw vertical scrollbar (right)
        if (scene_height_px > window_height_ - scrollbar_size) {
            double view_height = window_height_ - scrollbar_size;
            double scrollbar_height = view_height;
            double thumb_height = (view_height / scene_height_px) * scrollbar_height;
            double thumb_pos = ((max_offset_y - camera_offset_.y) / (2.0 * max_offset_y)) * (scrollbar_height - thumb_height);
            thumb_pos = std::max(0.0, std::min(thumb_pos, scrollbar_height - thumb_height));
            
            sf::RectangleShape bg(sf::Vector2f(scrollbar_size, scrollbar_height));
            bg.setPosition(sf::Vector2f(window_width_ - scrollbar_size, 0.0f));
            bg.setFillColor(sf::Color(240, 240, 240, 255));
            window.draw(bg);
            
            sf::RectangleShape thumb(sf::Vector2f(scrollbar_size, static_cast<float>(thumb_height)));
            thumb.setPosition(sf::Vector2f(window_width_ - scrollbar_size, static_cast<float>(thumb_pos)));
            thumb.setFillColor(sf::Color(180, 180, 180, 255));
            window.draw(thumb);
        }
        
        // Draw corner
        if (scene_width_px > window_width_ - scrollbar_size && scene_height_px > window_height_ - scrollbar_size) {
            sf::RectangleShape corner(sf::Vector2f(scrollbar_size, scrollbar_size));
            corner.setPosition(sf::Vector2f(window_width_ - scrollbar_size, window_height_ - scrollbar_size));
            corner.setFillColor(sf::Color(220, 220, 220, 255));
            window.draw(corner);
        }
    }

    sf::Vector2f CameraController::scene_to_screen(const geometry::Point& p, const geometry::Scene& scene) const {
        double base_scale_x = static_cast<double>(initial_window_width_) / scene.width;
        double base_scale_y = static_cast<double>(initial_window_height_) / scene.height;
        double base_scale = std::min(base_scale_x, base_scale_y);
        double scale = base_scale * zoom_level_;

        double scene_x = p.x - camera_offset_.x;
        double scene_y = p.y - camera_offset_.y;

        double offset_x = (window_width_ - scene.width * scale) / 2.0;
        double offset_y = (window_height_ - scene.height * scale) / 2.0;

        return sf::Vector2f(
            static_cast<float>(scene_x * scale + offset_x),
            static_cast<float>(window_height_ - (scene_y * scale + offset_y))
        );
    }

    geometry::Point CameraController::screen_to_scene(const sf::Vector2i& screen_pos, const geometry::Scene& scene) const {
        double base_scale_x = static_cast<double>(initial_window_width_) / scene.width;
        double base_scale_y = static_cast<double>(initial_window_height_) / scene.height;
        double base_scale = std::min(base_scale_x, base_scale_y);
        double scale = base_scale * zoom_level_;

        double offset_x = (window_width_ - scene.width * scale) / 2.0;
        double offset_y = (window_height_ - scene.height * scale) / 2.0;

        double screen_x = static_cast<double>(screen_pos.x);
        double screen_y = static_cast<double>(screen_pos.y);

        double scene_x = (screen_x - offset_x) / scale;
        double scene_y = (window_height_ - screen_y - offset_y) / scale;

        scene_x += camera_offset_.x;
        scene_y += camera_offset_.y;

        return geometry::Point{scene_x, scene_y};
    }

    double CameraController::get_scale(const geometry::Scene& scene) const {
        double base_scale_x = static_cast<double>(initial_window_width_) / scene.width;
        double base_scale_y = static_cast<double>(initial_window_height_) / scene.height;
        double base_scale = std::min(base_scale_x, base_scale_y);
        return base_scale * zoom_level_;
    }

    bool CameraController::handle_mouse_press(const sf::Vector2i& mouse_pos, const geometry::Scene& scene) {
        bool is_horizontal = false;
        bool is_on_thumb = false;
        
        if (is_point_on_scrollbar(mouse_pos, scene, is_horizontal, is_on_thumb)) {
            start_scrollbar_drag(mouse_pos, scene, is_horizontal, is_on_thumb);
            return true;
        } else {
            is_dragging_ = true;
            last_mouse_pos_ = mouse_pos;
            return false;
        }
    }

    void CameraController::handle_mouse_release() {
        is_dragging_ = false;
        stop_scrollbar_drag();
    }

    void CameraController::handle_mouse_move(const sf::Vector2i& mouse_pos, const geometry::Scene& scene) {
        if (is_dragging_scrollbar_h_ || is_dragging_scrollbar_v_) {
            update_scrollbar_drag(mouse_pos, scene);
        } else if (is_dragging_) {
            sf::Vector2i delta = mouse_pos - last_mouse_pos_;
            pan(delta, scene);
            last_mouse_pos_ = mouse_pos;
        }
    }

    float CameraController::max_offset_x(const geometry::Scene& scene) const {
        const float margin_ratio = 0.35f;
        return static_cast<float>(scene.width * (0.5f + margin_ratio));
    }

    float CameraController::max_offset_y(const geometry::Scene& scene) const {
        const float margin_ratio = 0.35f;
        return static_cast<float>(scene.height * (0.5f + margin_ratio));
    }

    void CameraController::clamp_camera_offset(const geometry::Scene& scene) {
        float max_offset_x = this->max_offset_x(scene);
        float max_offset_y = this->max_offset_y(scene);
        camera_offset_.x = std::max(-max_offset_x, std::min(camera_offset_.x, max_offset_x));
        camera_offset_.y = std::max(-max_offset_y, std::min(camera_offset_.y, max_offset_y));
    }

}






