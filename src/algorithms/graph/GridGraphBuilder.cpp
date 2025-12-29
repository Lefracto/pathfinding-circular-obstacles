//
// Implementation of GridGraphBuilder
//

#include "../../include/algorithms/GridGraphBuilder.h"
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace algorithms::graph {

    GridGraphBuilder::GridGraphBuilder(double grid_step, bool allow_diagonal)
        : grid_step_(grid_step), allow_diagonal_(allow_diagonal) {
        if (grid_step <= 0) {
            throw std::invalid_argument("Grid step must be positive");
        }
    }

    Graph GridGraphBuilder::build(const geometry::Scene& scene) {
        // Clear previous mappings
        node_to_point_.clear();
        point_to_node_.clear();

        // Calculate grid dimensions
        int grid_width = static_cast<int>(std::ceil(scene.width / grid_step_));
        int grid_height = static_cast<int>(std::ceil(scene.height / grid_step_));

        // First pass: create nodes for valid grid cells
        // A cell is valid if its center is not inside any obstacle and is within bounds
        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                geometry::Point cell_center = grid_to_point(gx, gy);
                
                // Check if cell center is valid (in bounds and not in obstacle)
                if (is_point_in_bounds(cell_center, scene) && 
                    !is_point_in_obstacle(cell_center, scene)) {
                    
                    std::size_t node_id = node_to_point_.size();
                    node_to_point_.push_back(cell_center);
                    
                    // Create hash key from grid coordinates for fast lookup
                    std::size_t grid_key = static_cast<std::size_t>(gy) * grid_width + static_cast<std::size_t>(gx);
                    point_to_node_[grid_key] = node_id;
                }
            }
        }

        // Initialize graph with empty adjacency lists
        Graph graph;
        graph.adj.resize(node_to_point_.size());

        // Second pass: create edges between adjacent valid nodes
        for (int gy = 0; gy < grid_height; ++gy) {
            for (int gx = 0; gx < grid_width; ++gx) {
                std::size_t grid_key = static_cast<std::size_t>(gy) * grid_width + static_cast<std::size_t>(gx);
                
                // Check if this grid cell has a node
                auto it = point_to_node_.find(grid_key);
                if (it == point_to_node_.end()) {
                    continue; // This cell doesn't have a node (obstacle or out of bounds)
                }

                std::size_t from_node = it->second;
                geometry::Point from_point = node_to_point_[from_node];

                // Check neighbors: 4-connectivity (up, down, left, right)
                int dx[] = {0, 1, 0, -1};
                int dy[] = {-1, 0, 1, 0};
                int num_directions = 4;

                // Add diagonal directions if allowed (8-connectivity)
                if (allow_diagonal_) {
                    int diag_dx[] = {-1, 1, 1, -1};
                    int diag_dy[] = {-1, -1, 1, 1};
                    for (int i = 0; i < 4; ++i) {
                        dx[num_directions] = diag_dx[i];
                        dy[num_directions] = diag_dy[i];
                        ++num_directions;
                    }
                }

                // Check each neighbor direction
                for (int dir = 0; dir < num_directions; ++dir) {
                    int nx = gx + dx[dir];
                    int ny = gy + dy[dir];

                    // Check bounds
                    if (nx < 0 || nx >= grid_width || ny < 0 || ny >= grid_height) {
                        continue;
                    }

                    std::size_t neighbor_key = static_cast<std::size_t>(ny) * grid_width + static_cast<std::size_t>(nx);
                    auto neighbor_it = point_to_node_.find(neighbor_key);
                    
                    if (neighbor_it != point_to_node_.end()) {
                        std::size_t to_node = neighbor_it->second;
                        geometry::Point to_point = node_to_point_[to_node];

                        // Check if edge is valid (line segment doesn't intersect obstacles)
                        // For simplicity, we check if both endpoints are valid
                        // More sophisticated: check if the edge segment intersects any obstacle
                        bool edge_valid = true;
                        
                        // Simple check: verify midpoint is not in obstacle
                        geometry::Point midpoint((from_point.x + to_point.x) / 2.0,
                                                (from_point.y + to_point.y) / 2.0);
                        if (is_point_in_obstacle(midpoint, scene)) {
                            edge_valid = false;
                        }

                        if (edge_valid) {
                            // Calculate edge weight (distance)
                            double weight = calculate_distance(from_point, to_point);
                            
                            // Add edge (undirected graph, so add to both nodes)
                            graph.adj[from_node].push_back({to_node, weight});
                        }
                    }
                }
            }
        }

        return graph;
    }

    std::string GridGraphBuilder::name() const {
        return "GridGraphBuilder";
    }

    geometry::Point GridGraphBuilder::get_node_point(std::size_t node_id) const {
        if (node_id >= node_to_point_.size()) {
            throw std::out_of_range("Node ID out of range");
        }
        return node_to_point_[node_id];
    }

    std::optional<std::size_t> GridGraphBuilder::get_node_id(const geometry::Point& point) const {
        // Find the closest grid cell
        int grid_x = static_cast<int>(std::round(point.x / grid_step_));
        int grid_y = static_cast<int>(std::round(point.y / grid_step_));
        
        // This is a simplified lookup - in a full implementation, you might want
        // a more sophisticated spatial index. For now, we'll search through all nodes.
        double min_distance = std::numeric_limits<double>::max();
        std::size_t closest_node = 0;
        bool found = false;

        for (std::size_t i = 0; i < node_to_point_.size(); ++i) {
            double dist = node_to_point_[i].distance(point);
            if (dist < min_distance) {
                min_distance = dist;
                closest_node = i;
                found = true;
            }
        }

        // Return node if it's close enough (within half a grid step)
        if (found && min_distance < grid_step_ / 2.0) {
            return closest_node;
        }
        return std::nullopt;
    }

    bool GridGraphBuilder::is_point_in_obstacle(const geometry::Point& point, const geometry::Scene& scene) const {
        for (const auto& obstacle : scene.obstacles) {
            if (obstacle.contains(point)) {
                return true;
            }
        }
        return false;
    }

    bool GridGraphBuilder::is_point_in_bounds(const geometry::Point& point, const geometry::Scene& scene) const {
        return point.x >= 0 && point.x <= scene.width &&
               point.y >= 0 && point.y <= scene.height;
    }

    double GridGraphBuilder::calculate_distance(const geometry::Point& a, const geometry::Point& b) const {
        return a.distance(b);
    }

    std::pair<int, int> GridGraphBuilder::point_to_grid(const geometry::Point& point) const {
        int grid_x = static_cast<int>(std::floor(point.x / grid_step_));
        int grid_y = static_cast<int>(std::floor(point.y / grid_step_));
        return {grid_x, grid_y};
    }

    geometry::Point GridGraphBuilder::grid_to_point(int grid_x, int grid_y) const {
        // Center of the grid cell
        double x = (grid_x + 0.5) * grid_step_;
        double y = (grid_y + 0.5) * grid_step_;
        return {x, y};
    }

} // namespace algorithms::graph
