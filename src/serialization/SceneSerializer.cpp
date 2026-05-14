#include "../include/serialization/SceneSerializer.h"
#include <stdexcept>
#include <iostream>

namespace serialization {

    json SceneSerializer::point_to_json(const geometry::Point& point) {
        return json{
            {"x", point.x},
            {"y", point.y}
        };
    }

    geometry::Point SceneSerializer::point_from_json(const json& j) {
        if (!j.contains("x") || !j.contains("y")) {
            throw std::runtime_error("Invalid Point JSON: missing 'x' or 'y' field");
        }
        if (!j["x"].is_number() || !j["y"].is_number()) {
            throw std::runtime_error("Invalid Point JSON: 'x' and 'y' must be numbers");
        }
        return geometry::Point(j["x"].get<double>(), j["y"].get<double>());
    }

    json SceneSerializer::disk_to_json(const geometry::Disk& disk) {
        return json{
            {"center", point_to_json(disk.center)},
            {"radius", disk.radius},
            {"id", disk.id}
        };
    }

    geometry::Disk SceneSerializer::disk_from_json(const json& j) {
        if (!j.contains("center") || !j.contains("radius")) {
            throw std::runtime_error("Invalid Disk JSON: missing 'center' or 'radius' field");
        }
        if (!j["radius"].is_number()) {
            throw std::runtime_error("Invalid Disk JSON: 'radius' must be a number");
        }
        
        geometry::Disk disk;
        disk.center = point_from_json(j["center"]);
        disk.radius = j["radius"].get<double>();
        
        // ID is optional for backward compatibility
        if (j.contains("id") && j["id"].is_number_unsigned()) {
            disk.id = j["id"].get<size_t>();
        }
        
        return disk;
    }

    json SceneSerializer::to_json(const geometry::Scene& scene) {
        json j;
        j["width"] = scene.width;
        j["height"] = scene.height;
        j["start"] = point_to_json(scene.start);
        j["goal"] = point_to_json(scene.goal);
        
        json obstacles_array = json::array();
        for (const auto& obstacle : scene.obstacles) {
            obstacles_array.push_back(disk_to_json(obstacle));
        }
        j["obstacles"] = obstacles_array;
        
        return j;
    }

    geometry::Scene SceneSerializer::from_json(const json& j) {
        geometry::Scene scene;
        
        // Validate required fields
        if (!j.contains("width") || !j.contains("height")) {
            throw std::runtime_error("Invalid Scene JSON: missing 'width' or 'height' field");
        }
        if (!j["width"].is_number() || !j["height"].is_number()) {
            throw std::runtime_error("Invalid Scene JSON: 'width' and 'height' must be numbers");
        }
        
        scene.width = j["width"].get<double>();
        scene.height = j["height"].get<double>();
        
        if (j.contains("start")) {
            scene.start = point_from_json(j["start"]);
        }
        
        if (j.contains("goal")) {
            scene.goal = point_from_json(j["goal"]);
        }
        
        if (j.contains("obstacles") && j["obstacles"].is_array()) {
            for (const auto& obstacle_json : j["obstacles"]) {
                scene.obstacles.push_back(disk_from_json(obstacle_json));
            }
        }
        
        return scene;
    }

    json SceneSerializer::to_json(const geometry::Path& path) {
        json j;
        json points_array = json::array();
        
        for (const auto& point : path.points) {
            points_array.push_back(point_to_json(point));
        }
        j["points"] = points_array;
        
        return j;
    }

    geometry::Path SceneSerializer::path_from_json(const json& j) {
        geometry::Path path;
        
        if (!j.contains("points")) {
            throw std::runtime_error("Invalid Path JSON: missing 'points' field");
        }
        if (!j["points"].is_array()) {
            throw std::runtime_error("Invalid Path JSON: 'points' must be an array");
        }
        
        for (const auto& point_json : j["points"]) {
            path.points.push_back(point_from_json(point_json));
        }
        
        return path;
    }

    bool SceneSerializer::save_to_file(const geometry::Scene& scene, const std::string& filepath) {
        try {
            json j = to_json(scene);
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for writing: " << filepath << std::endl;
                return false;
            }
            file << j.dump(4);
            file.close();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving Scene to file: " << e.what() << std::endl;
            return false;
        }
    }

    geometry::Scene SceneSerializer::load_from_file(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for reading: " + filepath);
            }
            
            json j;
            file >> j;
            file.close();
            
            return from_json(j);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Error loading Scene from file: " + std::string(e.what()));
        }
    }

    bool SceneSerializer::save_path_to_file(const geometry::Path& path, const std::string& filepath) {
        try {
            json j = to_json(path);
            std::ofstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open file for writing: " << filepath << std::endl;
                return false;
            }
            file << j.dump(4); // Pretty print with 4-space indentation
            file.close();
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error saving Path to file: " << e.what() << std::endl;
            return false;
        }
    }

    geometry::Path SceneSerializer::load_path_from_file(const std::string& filepath) {
        try {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for reading: " + filepath);
            }
            
            json j;
            file >> j;
            file.close();
            
            return path_from_json(j);
        } catch (const json::parse_error& e) {
            throw std::runtime_error("JSON parse error: " + std::string(e.what()));
        } catch (const std::exception& e) {
            throw std::runtime_error("Error loading Path from file: " + std::string(e.what()));
        }
    }
}
