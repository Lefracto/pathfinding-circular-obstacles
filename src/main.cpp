#include <iostream>
#include <cmath>
#include "include/geometry/NaiveObstacleSampler.h"
#include "include/visualization/SceneVisualizer.h"
#include "include/serialization/SceneSerializer.h"
#include "include/algorithms/GridGraphBuilder.h"
#include "include/algorithms/VisibilityGraphBuilder.h"
#include <SFML/Graphics.hpp>
#include <memory>

void test_serialization() {
    std::cout << "\n=== Testing Serialization ===\n";
    
    // Путь к файлу - сохраняем в корне проекта
    // Файл будет создан в директории, где запускается программа
    const std::string test_file = "test_scene.json";
    
    // Создаем генератор препятствий с фиксированным seed для воспроизводимости
    geometry::NaiveObstacleSampler sampler(4);
    
    // Генерируем тестовую сцену
    std::cout << "Generating test scene...\n";
    auto original_scene = sampler.sample(
        5,              // количество препятствий
        3.0,             // минимальный радиус
        8.0,             // максимальный радиус
        100.0, 100.0,    // размеры сцены (ширина, высота)
        50.0, 50.0,      // центр области спавна
        60.0, 60.0,      // размеры области спавна
        std::nullopt,    // start (случайный, если не указан)
        std::nullopt     // goal (случайный, если не указан)
    );
    
    std::cout << "Original scene:\n";
    std::cout << "  Obstacles: " << original_scene.obstacles.size() << "\n";
    std::cout << "  Start: (" << original_scene.start.x << ", " << original_scene.start.y << ")\n";
    std::cout << "  Goal: (" << original_scene.goal.x << ", " << original_scene.goal.y << ")\n";
    std::cout << "  Scene size: " << original_scene.width << "x" << original_scene.height << "\n";
    
    // Сохраняем сцену в файл
    std::cout << "\nSaving scene to file: " << test_file << "\n";
    if (!serialization::SceneSerializer::save_to_file(original_scene, test_file)) {
        std::cerr << "ERROR: Failed to save scene to file!\n";
        return;
    }
    std::cout << "Scene saved successfully!\n";
    
    // Загружаем сцену из файла
    std::cout << "\nLoading scene from file: " << test_file << "\n";
    geometry::Scene loaded_scene;
    try {
        loaded_scene = serialization::SceneSerializer::load_from_file(test_file);
        std::cout << "Scene loaded successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to load scene from file: " << e.what() << "\n";
        return;
    }
    
    // Проверяем, что данные совпадают
    std::cout << "\nLoaded scene:\n";
    std::cout << "  Obstacles: " << loaded_scene.obstacles.size() << "\n";
    std::cout << "  Start: (" << loaded_scene.start.x << ", " << loaded_scene.start.y << ")\n";
    std::cout << "  Goal: (" << loaded_scene.goal.x << ", " << loaded_scene.goal.y << ")\n";
    std::cout << "  Scene size: " << loaded_scene.width << "x" << loaded_scene.height << "\n";
    
    // Валидация данных
    bool is_valid = true;
    if (std::abs(original_scene.width - loaded_scene.width) > 0.001) {
        std::cerr << "ERROR: Width mismatch!\n";
        is_valid = false;
    }
    if (std::abs(original_scene.height - loaded_scene.height) > 0.001) {
        std::cerr << "ERROR: Height mismatch!\n";
        is_valid = false;
    }
    if (std::abs(original_scene.start.x - loaded_scene.start.x) > 0.001 ||
        std::abs(original_scene.start.y - loaded_scene.start.y) > 0.001) {
        std::cerr << "ERROR: Start point mismatch!\n";
        is_valid = false;
    }
    if (std::abs(original_scene.goal.x - loaded_scene.goal.x) > 0.001 ||
        std::abs(original_scene.goal.y - loaded_scene.goal.y) > 0.001) {
        std::cerr << "ERROR: Goal point mismatch!\n";
        is_valid = false;
    }
    if (original_scene.obstacles.size() != loaded_scene.obstacles.size()) {
        std::cerr << "ERROR: Obstacles count mismatch!\n";
        is_valid = false;
    }
    
    if (is_valid) {
        std::cout << "\n✓ Serialization test PASSED! All data matches.\n";
    } else {
        std::cerr << "\n✗ Serialization test FAILED! Data mismatch detected.\n";
        return;
    }
    
    // Визуализируем загруженную сцену для проверки
    std::cout << "\nVisualizing loaded scene to verify correctness...\n";
    visualization::SceneVisualizer visualizer(1200, 800);
    visualizer.apply_preset("alternative");
    
    std::cout << "Opening visualization window...\n";
    std::cout << "Close the window to continue.\n";
    
    // Визуализируем загруженную сцену
    visualizer.visualize(loaded_scene, std::nullopt, true);
    
    std::cout << "\n=== Serialization test completed successfully! ===\n";
}

void test_scene_to_graph_conversion() {
    std::cout << "\n=== Testing Scene to Graph Conversion and Visualization ===\n";
    
    // Генерируем сцену
    std::cout << "Generating test scene...\n";
    geometry::NaiveObstacleSampler sampler(42);
    auto scene = sampler.sample(
        15,              // количество препятствий
        3.0,             // минимальный радиус
        8.0,             // максимальный радиус
        100.0, 100.0,    // размеры сцены
        50.0, 50.0,      // центр области спавна
        60.0, 60.0,      // размеры области спавна
        std::nullopt,    // start
        std::nullopt     // goal
    );
    
    std::cout << "Scene generated:\n";
    std::cout << "  Obstacles: " << scene.obstacles.size() << "\n";
    std::cout << "  Size: " << scene.width << "x" << scene.height << "\n";
    std::cout << "  Start: (" << scene.start.x << ", " << scene.start.y << ")\n";
    std::cout << "  Goal: (" << scene.goal.x << ", " << scene.goal.y << ")\n";
    
    // Создаем построитель графа
    std::cout << "\nCreating grid graph builder...\n";
    auto graph_builder = std::make_shared<algorithms::graph::GridGraphBuilder>(5.0, true);
    std::cout << "  Grid step: " << graph_builder->get_grid_step() << "\n";
    std::cout << "  Diagonal connections: " << (graph_builder->is_diagonal_allowed() ? "enabled" : "disabled") << "\n";
    
    // Конвертируем сцену в граф
    std::cout << "\nConverting scene to graph...\n";
    auto graph = graph_builder->build(scene);
    
    std::cout << "Graph built successfully:\n";
    std::cout << "  Nodes: " << graph.adj.size() << "\n";
    
    // Подсчитываем рёбра
    std::size_t edge_count = 0;
    for (const auto& adj_list : graph.adj) {
        edge_count += adj_list.size();
    }
    edge_count /= 2; // Ненаправленный граф
    std::cout << "  Edges: " << edge_count << "\n";
    
    // Показываем несколько примеров узлов
    if (!graph.adj.empty()) {
        std::cout << "\nSample nodes:\n";
        for (std::size_t i = 0; i < std::min(static_cast<std::size_t>(5), graph.adj.size()); ++i) {
            try {
                auto point = graph_builder->get_node_point(i);
                std::cout << "  Node " << i << ": (" << point.x << ", " << point.y << ") - "
                          << graph.adj[i].size() << " neighbors\n";
            } catch (...) {
                continue;
            }
        }
    }
    
    // Создаем визуализатор и настраиваем
    std::cout << "\nSetting up visualizer...\n";
    visualization::SceneVisualizer visualizer(1200, 800);
    visualizer.apply_preset("alternative");
    
    // Create lambda for getting node point
    auto get_node_point = [graph_builder](std::size_t node_id) -> geometry::Point {
        return graph_builder->get_node_point(node_id);
    };
    
    auto graph_builder_base = std::static_pointer_cast<algorithms::graph::GraphBuilder>(graph_builder);
    visualizer.set_graph_builder(graph_builder_base, get_node_point);
    
    std::cout << "\nOpening visualization window...\n";
    std::cout << "Mode: GRAPH VISUALIZATION\n";
    std::cout << "  - Nodes are shown as cyan circles\n";
    std::cout << "  - Edges are shown as gray lines\n";
    std::cout << "  - Grid is shown in background\n";
    std::cout << "Close the window to continue.\n";
    
    // Визуализируем граф (show_graph = true)
    visualizer.visualize(scene, std::nullopt, true, true);
    
    std::cout << "\n=== Scene to Graph conversion and visualization completed! ===\n";
}

void test_visibility_graph() {
    std::cout << "\n=== Testing Visibility Graph ===\n";
    
    // Генерируем сцену
    std::cout << "Generating test scene...\n";
    geometry::NaiveObstacleSampler sampler(42);
    auto scene = sampler.sample(
        8,               // меньше препятствий для visibility graph
        3.0,
        8.0,
        100.0, 100.0,
        50.0, 50.0,
        60.0, 60.0,
        std::nullopt,
        std::nullopt
    );
    
    std::cout << "Scene generated:\n";
    std::cout << "  Obstacles: " << scene.obstacles.size() << "\n";
    std::cout << "  Size: " << scene.width << "x" << scene.height << "\n";
    std::cout << "  Start: (" << scene.start.x << ", " << scene.start.y << ")\n";
    std::cout << "  Goal: (" << scene.goal.x << ", " << scene.goal.y << ")\n";
    
    // Создаем visibility graph builder
    std::cout << "\nCreating visibility graph builder...\n";
    auto visibility_builder = std::make_shared<algorithms::graph::VisibilityGraphBuilder>(8);
    std::cout << "  Points per obstacle: " << visibility_builder->get_points_per_obstacle() << "\n";
    
    // Строим граф видимости
    std::cout << "\nBuilding visibility graph...\n";
    auto graph = visibility_builder->build(scene);
    
    std::cout << "Visibility graph built:\n";
    std::cout << "  Nodes: " << graph.adj.size() << "\n";
    
    // Подсчитываем рёбра
    std::size_t edge_count = 0;
    for (const auto& adj_list : graph.adj) {
        edge_count += adj_list.size();
    }
    edge_count /= 2;
    std::cout << "  Edges: " << edge_count << "\n";
    
    // Создаем визуализатор
    std::cout << "\nSetting up visualizer...\n";
    visualization::SceneVisualizer visualizer(1200, 800);
    visualizer.apply_preset("alternative");
    
    // Create lambda for getting node point
    auto get_node_point = [visibility_builder](std::size_t node_id) -> geometry::Point {
        return visibility_builder->get_node_point(node_id);
    };
    
    auto graph_builder_base = std::static_pointer_cast<algorithms::graph::GraphBuilder>(visibility_builder);
    visualizer.set_graph_builder(graph_builder_base, get_node_point);
    
    std::cout << "\nOpening visualization window...\n";
    std::cout << "Mode: VISIBILITY GRAPH\n";
    std::cout << "  - Nodes are on obstacle boundaries and at start/goal\n";
    std::cout << "  - Edges connect nodes with direct line-of-sight\n";
    std::cout << "Close the window to continue.\n";
    
    // Визуализируем граф видимости
    visualizer.visualize(scene, std::nullopt, false, true);
    
    std::cout << "\n=== Visibility graph test completed! ===\n";
}

int main() {
    std::cout << "=== Diploma Project - Scene Visualization ===\n";
    
    // Запускаем тест сериализации
    test_serialization();
    
    // Тест конвертации сцены в граф и визуализации графа
    test_scene_to_graph_conversion();
    
    // Тест графа видимости
    test_visibility_graph();
    
    std::cout << "\nProgram finished.\n";
    return 0;
}
