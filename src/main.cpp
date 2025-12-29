#include <iostream>
#include "include/geometry/NaiveObstacleSampler.h"
#include "include/visualization/SceneVisualizer.h"
#include <SFML/Graphics.hpp>

int main() {
    std::cout << "Generating test scene with obstacles...\n";

    // Создаем генератор препятствий с фиксированным seed для воспроизводимости
    geometry::NaiveObstacleSampler sampler(54);

    // Генерируем сцену:
    // - 15 препятствий
    // - радиусы от 3.0 до 8.0
    // - размер сцены 100x100
    // - область спавна в центре (50, 50) размером 60x60
    // - start и goal генерируются случайно
    auto scene = sampler.sample(
        15,              // количество препятствий
        3.0,             // минимальный радиус
        8.0,             // максимальный радиус
        100.0, 100.0,    // размеры сцены (ширина, высота)
        50.0, 50.0,      // центр области спавна
        60.0, 60.0,      // размеры области спавна
        std::nullopt,    // start (случайный, если не указан)
        std::nullopt     // goal (случайный, если не указан)
    );

    std::cout << "Scene generated successfully!\n";
    std::cout << "  Obstacles: " << scene.obstacles.size() << "\n";
    std::cout << "  Start: (" << scene.start.x << ", " << scene.start.y << ")\n";
    std::cout << "  Goal: (" << scene.goal.x << ", " << scene.goal.y << ")\n";
    std::cout << "  Scene size: " << scene.width << "x" << scene.height << "\n";

    // С  /*оздаем визуализатор
    visualization::SceneVisualizer visualizer(1200, 800);

    // Настраиваем параметры визуализации (опционально)
    visualizer.set_grid_step(10.0);
    visualizer.set_obstacle_color(sf::Color::Red);
    visualizer.set_start_color(sf::Color::Green);
    visualizer.set_goal_color(sf::Color::Blue);

    std::cout << "\nOpening visualization window...\n";
    std::cout << "Close the window to exit.\n";

    // Визуализируем сцену (показываем сетку)
    visualizer.visualize(scene, std::nullopt, true);

    return 0;
}
