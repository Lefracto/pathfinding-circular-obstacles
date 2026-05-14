#ifndef GEOMETRY_DISK_H
#define GEOMETRY_DISK_H

#include "point.h"

namespace geometry {

    struct Disk {
        Point center{};
        double radius{};
        size_t id = 0;

        Disk() = default;
        Disk(Point c, double r, size_t id = 0)
            : center(c), radius(r), id(id) {}

        [[nodiscard]] bool contains(const Point& p) const {
            return center.distance(p) <= radius;
        }
    };

}

#endif
