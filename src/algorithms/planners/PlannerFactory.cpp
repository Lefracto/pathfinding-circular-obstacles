#include "algorithms/PlannerFactory.h"

#include "algorithms/GraphBasedPlanners.h"
#include "algorithms/RRTPlanner.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

namespace algorithms {

    namespace {

        double read_double(const nlohmann::json& params, const char* key, const double default_value) {
            if (!params.contains(key)) {
                return default_value;
            }
            if (!params[key].is_number()) {
                throw std::runtime_error(std::string("Algorithm param '") + key + "' must be a number");
            }
            return params[key].get<double>();
        }

        bool read_bool(const nlohmann::json& params, const char* key, const bool default_value) {
            if (!params.contains(key)) {
                return default_value;
            }
            if (!params[key].is_boolean()) {
                throw std::runtime_error(std::string("Algorithm param '") + key + "' must be a boolean");
            }
            return params[key].get<bool>();
        }

        std::size_t read_size_t(const nlohmann::json& params,
                                const char* key,
                                const std::size_t default_value) {
            if (!params.contains(key)) {
                return default_value;
            }
            if (!params[key].is_number_unsigned()) {
                throw std::runtime_error(std::string("Algorithm param '") + key + "' must be an unsigned integer");
            }
            return params[key].get<std::size_t>();
        }

        std::uint32_t read_uint32(const nlohmann::json& params,
                                  const char* key,
                                  const std::uint32_t default_value) {
            if (!params.contains(key)) {
                return default_value;
            }
            if (!params[key].is_number_unsigned()) {
                throw std::runtime_error(std::string("Algorithm param '") + key + "' must be an unsigned integer");
            }
            return params[key].get<std::uint32_t>();
        }

        int read_int(const nlohmann::json& params, const char* key, const int default_value) {
            if (!params.contains(key)) {
                return default_value;
            }
            if (!params[key].is_number_integer()) {
                throw std::runtime_error(std::string("Algorithm param '") + key + "' must be an integer");
            }
            return params[key].get<int>();
        }

    } // namespace

    std::unique_ptr<Planner> PlannerFactory::create(const std::string& algorithm_id,
                                                    const nlohmann::json& params) {
        if (algorithm_id == "grid_dijkstra") {
            const double grid_step = read_double(params, "grid_step", 5.0);
            const bool allow_diagonal = read_bool(params, "allow_diagonal", true);
            return std::make_unique<GridDijkstraPlanner>(grid_step, allow_diagonal);
        }

        if (algorithm_id == "grid_astar") {
            const double grid_step = read_double(params, "grid_step", 5.0);
            const bool allow_diagonal = read_bool(params, "allow_diagonal", true);
            return std::make_unique<GridAStarPlanner>(grid_step, allow_diagonal);
        }

        if (algorithm_id == "visibility_dijkstra") {
            const int points_per_obstacle = read_int(params, "points_per_obstacle", 12);
            return std::make_unique<VisibilityDijkstraPlanner>(points_per_obstacle);
        }

        if (algorithm_id == "visibility_astar") {
            const int points_per_obstacle = read_int(params, "points_per_obstacle", 12);
            return std::make_unique<VisibilityAStarPlanner>(points_per_obstacle);
        }

        if (algorithm_id == "rrt") {
            RRTPlanner::Config config;
            config.max_iterations = read_size_t(params, "max_iterations", config.max_iterations);
            config.step_size = read_double(params, "step_size", config.step_size);
            config.goal_sample_rate = read_double(params, "goal_sample_rate", config.goal_sample_rate);
            config.goal_tolerance = read_double(params, "goal_tolerance", config.goal_tolerance);
            config.smoothing_iterations =
                read_size_t(params, "smoothing_iterations", config.smoothing_iterations);
            config.enable_path_smoothing =
                read_bool(params, "enable_path_smoothing", config.enable_path_smoothing);
            config.seed = read_uint32(params, "seed", config.seed);
            return std::make_unique<RRTPlanner>(config);
        }

        throw std::runtime_error("Unsupported algorithm id: " + algorithm_id);
    }

    bool PlannerFactory::supports(const std::string& algorithm_id) {
        return algorithm_id == "grid_dijkstra" ||
               algorithm_id == "grid_astar" ||
               algorithm_id == "visibility_dijkstra" ||
               algorithm_id == "visibility_astar" ||
               algorithm_id == "rrt";
    }

} // namespace algorithms
