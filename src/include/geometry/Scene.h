//
// Created by USER on 26/12/2025.
//

#ifndef GEOMETRY_SCENE_H
#define GEOMETRY_SCENE_H

#include "point.h"
#include "disk.h"
#include <vector>

namespace geometry {

    struct Scene {
        std::vector<Disk> obstacles;
        Point start {0, 0};
        Point goal {0, 0};
        double width = 100.0;
        double height = 100.0;

        Scene() = default;
        Scene(const Point s, const Point g, const double w = 100.0, const double h = 100.0)
            : start(s), goal(g), width(w), height(h) {}

        void add_obstacle(const Disk& disk) {
            obstacles.push_back(disk);
        }
    };

}

#endif
