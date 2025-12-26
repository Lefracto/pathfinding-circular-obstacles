#ifndef ALGORITHMS_GRAPH_GRAPH_H
#define ALGORITHMS_GRAPH_GRAPH_H

#include <vector>
#include <cstddef>

namespace algorithms::graph {

    struct Graph {
        struct Edge {
            std::size_t to;
            double weight;
        };

        std::vector<std::vector<Edge>> adj;
    };

}

#endif
