#ifndef VISUALIZATION_APP_INFO_H
#define VISUALIZATION_APP_INFO_H

#ifndef DIPLOMA_APP_DISPLAY_NAME
#define DIPLOMA_APP_DISPLAY_NAME "Circular Obstacle Pathfinding Lab"
#endif

#ifndef DIPLOMA_PROJECT_VERSION
#define DIPLOMA_PROJECT_VERSION "1.2.15"
#endif

#ifndef DIPLOMA_WORKSPACE_WINDOW_TITLE
#define DIPLOMA_WORKSPACE_WINDOW_TITLE "Circular Obstacle Pathfinding Lab"
#endif

#ifndef DIPLOMA_SCENE_WINDOW_TITLE
#define DIPLOMA_SCENE_WINDOW_TITLE "Circular Obstacle Pathfinding Lab - Scene View"
#endif

namespace visualization::app_info {

    inline constexpr const char* display_name = DIPLOMA_APP_DISPLAY_NAME;
    inline constexpr const char* version = DIPLOMA_PROJECT_VERSION;
    inline constexpr const char* workspace_window_title = DIPLOMA_WORKSPACE_WINDOW_TITLE;
    inline constexpr const char* scene_window_title = DIPLOMA_SCENE_WINDOW_TITLE;

}

#endif
