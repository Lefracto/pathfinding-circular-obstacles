#ifndef ALGORITHMS_RRT_PLANNER_H
#define ALGORITHMS_RRT_PLANNER_H

#include "Planner.h"

#include <cstddef>
#include <cstdint>

namespace algorithms {

    class RRTPlanner final : public Planner {
    public:
        struct Config {
            std::size_t max_iterations = 6000;
            double step_size = 2.5;
            double goal_sample_rate = 0.10;
            double goal_tolerance = 2.0;
            std::size_t smoothing_iterations = 100;
            bool enable_path_smoothing = true;
            std::uint32_t seed = 42;
        };

        RRTPlanner();
        explicit RRTPlanner(const Config& config);

        void set_seed(std::uint32_t seed);
        [[nodiscard]] std::uint32_t seed() const;
        [[nodiscard]] const Config& config() const;

        [[nodiscard]] PathResult find_path(const geometry::Scene& scene) override;
        [[nodiscard]] BenchmarkResult plan(const geometry::Scene& scene) override;
        [[nodiscard]] std::string name() const override;

    private:
        Config config_;
    };

} // namespace algorithms

#endif // ALGORITHMS_RRT_PLANNER_H
