//
// Graph renderer for visualizing graph structures
//

#ifndef VISUALIZATION_GRAPH_RENDERER_H
#define VISUALIZATION_GRAPH_RENDERER_H

#include "../algorithms/Graph.h"
#include "../geometry/Point.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>

namespace visualization {

    /**
     * Renders graph structures (nodes and edges) for visualization
     */
    class GraphRenderer {
    public:
        GraphRenderer();

        /**
         * Draw graph nodes and edges using a function to get node coordinates
         * @param window Render window
         * @param graph Graph to draw
         * @param get_node_point Function that returns point for a given node ID
         * @param scene_to_screen Coordinate transformation function
         */
        void draw_graph(sf::RenderWindow& window,
                       const algorithms::graph::Graph& graph,
                       std::function<geometry::Point(std::size_t)> get_node_point,
                       std::function<sf::Vector2f(const geometry::Point&)> scene_to_screen);

        /**
         * Set node color
         */
        void set_node_color(const sf::Color& color) { node_color_ = color; }

        /**
         * Set edge color
         */
        void set_edge_color(const sf::Color& color) { edge_color_ = color; }

        /**
         * Set node radius
         */
        void set_node_radius(double radius) { node_radius_ = radius; }

        /**
         * Set edge thickness
         */
        void set_edge_thickness(float thickness) { edge_thickness_ = thickness; }

        /**
         * Set whether to show nodes
         */
        void set_show_nodes(bool show) { show_nodes_ = show; }

        /**
         * Set whether to show edges
         */
        void set_show_edges(bool show) { show_edges_ = show; }

    private:
        sf::Color node_color_ = sf::Color::Cyan;
        sf::Color edge_color_ = sf::Color(100, 100, 100, 150);  // Semi-transparent gray
        double node_radius_ = 4.0;  // Increased size for better visibility
        float edge_thickness_ = 1.0f;
        bool show_nodes_ = true;
        bool show_edges_ = true;
    };

} // namespace visualization

#endif //VISUALIZATION_GRAPH_RENDERER_H

