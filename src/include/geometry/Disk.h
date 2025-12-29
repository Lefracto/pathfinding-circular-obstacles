//
// Created by USER on 26/12/2025.
//

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

        /**
         * Check if a point is inside or on the boundary of this disk
         */
        [[nodiscard]] bool contains(const Point& p) const {
            return center.distance(p) <= radius;
        }

        /*
        [[nodiscard]] bool intersects(const Disk& other) const {
            double dist = center.distance(other.center);
            return dist <= (radius + other.radius);
        }

        [[nodiscard]] double circumference() const {
            constexpr double PI = 3.14159265358979323846;
            return 2.0 * PI * radius;
        }

        bool intersects_segment(const Point& a, const Point& b) const;

        std::pair<Point, Point> tangents_from(const Point& external) const;
        */
    };

}

#endif
