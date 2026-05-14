#ifndef VISUALIZATION_UI_MANAGER_H
#define VISUALIZATION_UI_MANAGER_H

#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>

namespace visualization {

    class UIManager {
    public:
        UIManager();
        ~UIManager();

        bool initialize(sf::RenderWindow& window);
        void shutdown();
        void update(sf::RenderWindow& window, sf::Time deltaTime);
        void render(sf::RenderWindow& window);
        bool consume_save_screenshot_request();
        bool consume_create_experiment_request();
        bool consume_open_config_request();
        bool consume_run_config_folder_request();
        bool consume_open_experiment_request();
        bool consume_save_experiment_request();

    private:
        void setup_style();
        void load_fonts();
        void load_app_icon();
        void draw_menu_bar();
        void draw_about_window();

        bool initialized_ = false;
        sf::Texture app_icon_texture_;
        bool app_icon_texture_ready_ = false;
        bool create_experiment_requested_ = false;
        bool open_config_requested_ = false;
        bool run_config_folder_requested_ = false;
        bool open_experiment_requested_ = false;
        bool save_experiment_requested_ = false;
        bool save_screenshot_requested_ = false;
        bool about_window_open_ = false;
    };

}

#endif
