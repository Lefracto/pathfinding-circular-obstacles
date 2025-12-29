//
// Created by USER on 26/12/2025.
//

#ifndef GEOMETRY_PATH_H
#define GEOMETRY_PATH_H

#include "point.h"
#include "scene.h"

#include <vector>

namespace geometry {

    struct Path {
        std::vector<Point> points;

        Path() = default;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] double length() const;

        //explicit Path(std::vector<Point> pts) : points(std::move(pts)) {}
        //[[nodiscard]] bool is_valid(const Scene& scene) const;
    }; 

}

#endif
