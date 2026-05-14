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

    }; 

}

#endif
