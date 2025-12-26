//
// Created by USER on 26/12/2025.
//

#ifndef GEOMETRY_POINT_H  // Именование: geometry_point_h
#define GEOMETRY_POINT_H

#include <concepts>
#include <cmath>

namespace geometry {

    struct Point {
        double x, y;

        Point() = default;
        Point(double x, double y) : x(x), y(y) {}

        [[nodiscard]] double distance(const Point& other) const {
            return std::hypot(x - other.x, y - other.y);
        }

        Point operator+(const Point& other) const {
            return {x + other.x, y + other.y};
        }

        Point operator*(double scalar) const {
            return {x * scalar, y * scalar};
        }

        bool operator==(const Point& other) const {
            return x == other.x && y == other.y;
        }
    };

}

#endif
