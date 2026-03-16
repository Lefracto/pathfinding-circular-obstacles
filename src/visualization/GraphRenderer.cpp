//
// Implementation of GraphRenderer
//

#include "../include/visualization/GraphRenderer.h"
#include <cmath>

namespace visualization {

    GraphRenderer::GraphRenderer() = default;

    void GraphRenderer::draw_graph(sf::RenderWindow& window,
                                   const algorithms::graph::Graph& graph,
                                   std::function<geometry::Point(std::size_t)> get_node_point,
                                   std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen) {
        
        // Draw edges first (so nodes appear on top)
        if (show_edges_) {
            for (std::size_t from_node = 0; from_node < graph.adj.size(); ++from_node) {
                try {
                    geometry::Point from_point = get_node_point(from_node);
                    sf::Vector2f from_screen = scene_to_screen(from_point);

                    for (const auto& edge : graph.adj[from_node]) {
                        // Only draw edge once (undirected graph)
                        if (edge.to > from_node) {
                            try {
                                geometry::Point to_point = get_node_point(edge.to);
                                sf::Vector2f to_screen = scene_to_screen(to_point);

                                // Draw edge as a line
                                sf::Vertex line[] = {
                                    sf::Vertex(from_screen, edge_color_),
                                    sf::Vertex(to_screen, edge_color_)
                                };
                                window.draw(line, 2, sf::PrimitiveType::Lines);
                            } catch (...) {
                                // Skip invalid node
                                continue;
                            }
                        }
                    }
                } catch (...) {
                    // Skip invalid node
                    continue;
                }
            }
        }

        // Draw nodes
        if (show_nodes_) {
            for (std::size_t node_id = 0; node_id < graph.adj.size(); ++node_id) {
                try {
                    geometry::Point node_point = get_node_point(node_id);
                    sf::Vector2f screen_pos = scene_to_screen(node_point);

                    // Draw node as a circle
                    sf::CircleShape node_circle(node_radius_);
                    node_circle.setFillColor(node_color_);
                    node_circle.setOutlineColor(sf::Color::Black);
                    node_circle.setOutlineThickness(0.5f);
                    node_circle.setPointCount(16);
                    node_circle.setPosition(screen_pos - sf::Vector2f(node_radius_, node_radius_));
                    window.draw(node_circle);
                } catch (...) {
                    // Skip invalid node
                    continue;
                }
            }
        }
    }

} // namespace visualization

