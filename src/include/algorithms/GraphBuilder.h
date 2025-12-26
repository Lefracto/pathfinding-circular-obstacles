#ifndef ALGORITHMS_GRAPH_GRAPH_BUILDER_H
#define ALGORITHMS_GRAPH_GRAPH_BUILDER_H

#include "../geometry/Scene.h"
#include "Graph.h"

namespace algorithms::graph {

    class GraphBuilder {
    public:
        virtual ~GraphBuilder() = default;

        [[nodiscard]] virtual Graph build(const geometry::Scene& scene) const = 0;
        [[nodiscard]] virtual std::string name() const;
    };

}

#endif
