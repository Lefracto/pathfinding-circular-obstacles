#include "../include/visualization/UIManager.h"
#include "../include/visualization/AppInfo.h"
#include "../include/visualization/AppAssets.h"

namespace {
    constexpr const char* kProjectTitle = visualization::app_info::display_name;
    constexpr const char* kProjectVersion = visualization::app_info::version;
    constexpr const char* kProjectAuthor = "Anton Pauliuchyk";
    constexpr const char* kProjectUpdatedAt = "2026-05-14";
    constexpr const char* kProjectGithub = "https://github.com/Lefracto/pathfinding-circular-obstacles";
    constexpr const char* kProjectEmail = "ampavlyuchik@edu.hse.ru";
    constexpr const char* kProjectAnnotation =
        "Interactive experiment workspace for generating 2D scenes with circular obstacles, "
        "visualizing planning structures, and benchmarking pathfinding algorithms.";
}

namespace visualization {

    UIManager::UIManager() = default;

    UIManager::~UIManager() {
        if (initialized_) {
            shutdown();
        }
    }

    bool UIManager::initialize(sf::RenderWindow& window) {
        if (!ImGui::SFML::Init(window)) {
            return false;
        }

        load_fonts();
        load_app_icon();
        setup_style();
        initialized_ = true;
        return true;
    }

    void UIManager::shutdown() {
        if (initialized_) {
            ImGui::SFML::Shutdown();
            initialized_ = false;
        }
    }

    void UIManager::update(sf::RenderWindow& window, sf::Time deltaTime) {
        if (initialized_) {
            ImGui::SFML::Update(window, deltaTime);
        }
    }

    void UIManager::render(sf::RenderWindow& window) {
        if (initialized_) {
            draw_menu_bar();
            draw_about_window();
            ImGui::SFML::Render(window);
        }
    }

    bool UIManager::consume_save_screenshot_request() {
        const bool requested = save_screenshot_requested_;
        save_screenshot_requested_ = false;
        return requested;
    }

    bool UIManager::consume_create_experiment_request() {
        const bool requested = create_experiment_requested_;
        create_experiment_requested_ = false;
        return requested;
    }

    bool UIManager::consume_open_config_request() {
        const bool requested = open_config_requested_;
        open_config_requested_ = false;
        return requested;
    }

    bool UIManager::consume_run_config_folder_request() {
        const bool requested = run_config_folder_requested_;
        run_config_folder_requested_ = false;
        return requested;
    }

    bool UIManager::consume_open_experiment_request() {
        const bool requested = open_experiment_requested_;
        open_experiment_requested_ = false;
        return requested;
    }

    bool UIManager::consume_save_experiment_request() {
        const bool requested = save_experiment_requested_;
        save_experiment_requested_ = false;
        return requested;
    }

    void UIManager::load_fonts() {
        ImGuiIO& io = ImGui::GetIO();

        ImFontConfig fontConfig;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        fontConfig.PixelSnapH = true;

        ImFont* font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeui.ttf", 18.0f, &fontConfig);
        if (font == nullptr) {
            font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/segoeuib.ttf", 18.0f, &fontConfig);
        }
        if (font == nullptr) {
            font = io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/tahoma.ttf", 18.0f, &fontConfig);
        }
        if (font == nullptr) {
            io.FontGlobalScale = 1.5f;
        } else {
            io.FontDefault = font;
        }

        if (!ImGui::SFML::UpdateFontTexture()) {
            // Font texture update failed, but continue anyway.
        }
    }

    void UIManager::load_app_icon() {
        const auto icon_image = app_assets::load_app_icon();
        app_icon_texture_ready_ = icon_image.has_value() && app_icon_texture_.loadFromImage(*icon_image);
        if (app_icon_texture_ready_) {
            app_icon_texture_.setSmooth(true);
        }
    }

    void UIManager::setup_style() {
        ImGuiStyle& style = ImGui::GetStyle();

        style.ScaleAllSizes(1.4f);

        style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 0.35f, 0.65f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.47f, 0.84f, 0.20f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.47f, 0.84f, 0.40f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.47f, 0.84f, 0.60f);
        style.Colors[ImGuiCol_Separator] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 0.35f, 0.65f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 0.35f, 0.65f, 1.00f);
        style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
        style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
        style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        style.Colors[ImGuiCol_TableRowBg] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.47f, 0.84f, 0.35f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.00f, 0.47f, 0.84f, 0.90f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.00f, 0.47f, 0.84f, 1.00f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.35f);

        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.PopupRounding = 0.0f;
        style.ScrollbarRounding = 0.0f;
        style.GrabRounding = 0.0f;
        style.TabRounding = 0.0f;
        style.WindowBorderSize = 1.0f;
        style.FrameBorderSize = 1.0f;
        style.PopupBorderSize = 1.0f;

        style.WindowPadding = ImVec2(8.0f, 8.0f);
        style.FramePadding = ImVec2(10.0f, 6.0f);
        style.CellPadding = ImVec2(4.0f, 2.0f);
        style.ItemSpacing = ImVec2(8.0f, 6.0f);
        style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
        style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
        style.IndentSpacing = 21.0f;
        style.ScrollbarSize = 20.0f;
        style.GrabMinSize = 15.0f;
    }

    void UIManager::draw_menu_bar() {
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Create Experiment...")) {
                    create_experiment_requested_ = true;
                }
                if (ImGui::MenuItem("Open Config...")) {
                    open_config_requested_ = true;
                }
                if (ImGui::MenuItem("Run Config Folder...")) {
                    run_config_folder_requested_ = true;
                }
                if (ImGui::MenuItem("Open Experiment...")) {
                    open_experiment_requested_ = true;
                }
                if (ImGui::MenuItem("Save Experiment...")) {
                    save_experiment_requested_ = true;
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save Screenshot...")) {
                    save_screenshot_requested_ = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("About")) {
                    about_window_open_ = true;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void UIManager::draw_about_window() {
        if (!about_window_open_) {
            return;
        }

        ImGui::SetNextWindowSize(ImVec2(700.0f, 360.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(120.0f, 90.0f), ImGuiCond_FirstUseEver);

        if (!ImGui::Begin("About Project", &about_window_open_,
                          ImGuiWindowFlags_NoCollapse)) {
            ImGui::End();
            return;
        }

        if (app_icon_texture_ready_) {
            ImGui::Image(app_icon_texture_, sf::Vector2f(96.0f, 96.0f));
            ImGui::SameLine();
        }

        ImGui::BeginGroup();
        ImGui::Text("%s", kProjectTitle);
        ImGui::Separator();
        ImGui::Text("Version: %s", kProjectVersion);
        ImGui::Text("Author: %s", kProjectAuthor);
        ImGui::Text("Last Updated: %s", kProjectUpdatedAt);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::TextWrapped("%s", kProjectAnnotation);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("Repository");
        ImGui::TextWrapped("%s", kProjectGithub);
        if (ImGui::Button("Copy GitHub Link")) {
            ImGui::SetClipboardText(kProjectGithub);
        }

        ImGui::Spacing();
        ImGui::Text("Contact");
        ImGui::TextWrapped("%s", kProjectEmail);
        if (ImGui::Button("Copy Email")) {
            ImGui::SetClipboardText(kProjectEmail);
        }

        ImGui::Spacing();
        if (ImGui::Button("Close")) {
            about_window_open_ = false;
        }

        ImGui::End();
    }

}
