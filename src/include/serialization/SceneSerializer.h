#ifndef SERIALIZATION_SCENE_SERIALIZER_H
#define SERIALIZATION_SCENE_SERIALIZER_H

#include "../geometry/Scene.h"
#include "../geometry/Path.h"
#include <string>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace serialization {
    class SceneSerializer {
    public:
        static json to_json(const geometry::Scene& scene);
        static geometry::Scene from_json(const json& j);
        static json to_json(const geometry::Path& path);
        static geometry::Path path_from_json(const json& j);
        static bool save_to_file(const geometry::Scene& scene, const std::string& filepath);
        static geometry::Scene load_from_file(const std::string& filepath);
        static bool save_path_to_file(const geometry::Path& path, const std::string& filepath);
        static geometry::Path load_path_from_file(const std::string& filepath);

    private:
        static json point_to_json(const geometry::Point& point);
        static geometry::Point point_from_json(const json& j);
        
        static json disk_to_json(const geometry::Disk& disk);
        static geometry::Disk disk_from_json(const json& j);
    };

}

#endif
