#include <iostream>

#include "include/visualization/SceneVisualizer.h"

int main() {
    std::cout << "=== Experiment Workspace ===\n";

    visualization::SceneVisualizer visualizer(1400, 900);
    visualizer.apply_preset("alternative");

    if (!visualizer.run_experiment_workspace()) {
        std::cerr << "Failed to initialize experiment workspace UI.\n";
        return 1;
    }

    return 0;
}

