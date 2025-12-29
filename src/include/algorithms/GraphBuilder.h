#ifndef ALGORITHMS_GRAPH_GRAPH_BUILDER_H
#define ALGORITHMS_GRAPH_GRAPH_BUILDER_H

#include "../geometry/Scene.h"
#include "Graph.h"
#include <string>

namespace algorithms::graph {

    class GraphBuilder {
    public:
        virtual ~GraphBuilder() = default;

        [[nodiscard]] virtual Graph build(const geometry::Scene& scene) = 0;
        [[nodiscard]] virtual std::string name() const = 0;
    };

}

#endif
