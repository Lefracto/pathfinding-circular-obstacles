#include <iostream>

#include "include/visualization/AppInfo.h"
#include "include/visualization/SceneVisualizer.h"

int main() {
    std::cout << "=== " << visualization::app_info::display_name
              << " v" << visualization::app_info::version << " ===\n";

    visualization::SceneVisualizer visualizer(1400, 900);
    visualizer.apply_preset("alternative");

    if (!visualizer.run_experiment_workspace()) {
        std::cerr << "Failed to initialize experiment workspace UI.\n";
        return 1;
    }

    return 0;
}
