#ifndef ALGORITHMS_PLANNER_FACTORY_H
#define ALGORITHMS_PLANNER_FACTORY_H

#include "Planner.h"

#include <memory>
#include <string>

#include <nlohmann/json.hpp>

namespace algorithms {

    class PlannerFactory {
    public:
        [[nodiscard]] static std::unique_ptr<Planner> create(
            const std::string& algorithm_id,
            const nlohmann::json& params = nlohmann::json::object());

        [[nodiscard]] static bool supports(const std::string& algorithm_id);
    };

} // namespace algorithms

#endif // ALGORITHMS_PLANNER_FACTORY_H
