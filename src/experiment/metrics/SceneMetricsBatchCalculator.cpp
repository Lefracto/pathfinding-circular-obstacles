#include "experiment/metrics/SceneMetricsBatchCalculator.h"

#include "experiment/metrics/SceneMetricsCalculator.h"
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

namespace experiment {

    namespace {

        std::size_t resolve_worker_count(const std::size_t scene_count, const MetricsComputationConfig& config) {
            if (scene_count == 0 || config.execution_mode == MetricsExecutionMode::Serial) {
                return 1;
            }

            std::size_t workers = config.worker_threads;
            if (workers == 0) {
                workers = static_cast<std::size_t>(std::thread::hardware_concurrency());
            }
            if (workers == 0) {
                workers = 4;
            }

            workers = std::max<std::size_t>(1, workers);
            workers = std::min(workers, scene_count);
            return workers;
        }

    } // namespace

    SceneMetricsBatchResult SceneMetricsBatchCalculator::compute(const std::vector<SceneInstance>& instances,
                                                                 const MetricsComputationConfig& config,
                                                                 std::atomic<std::size_t>* completed_counter) {
        SceneMetricsBatchResult result;
        result.metrics.resize(instances.size());

        if (completed_counter != nullptr) {
            completed_counter->store(0);
        }
        if (instances.empty()) {
            return result;
        }

        const SceneMetricsComputationOptions options = SceneMetricsCalculator::from_config(config);
        const std::size_t worker_count = resolve_worker_count(instances.size(), config);

        if (worker_count == 1) {
            for (std::size_t i = 0; i < instances.size(); ++i) {
                result.metrics[i] = SceneMetricsCalculator::compute(instances[i].scene_data, instances[i].seed, options);
                if (completed_counter != nullptr) {
                    completed_counter->fetch_add(1);
                }
            }
            return result;
        }

        std::atomic<std::size_t> next_index{0};
        std::vector<std::thread> workers;
        workers.reserve(worker_count);

        for (std::size_t worker = 0; worker < worker_count; ++worker) {
            workers.emplace_back([&instances, &result, &options, completed_counter, &next_index]() {
                while (true) {
                    const std::size_t index = next_index.fetch_add(1);
                    if (index >= instances.size()) {
                        return;
                    }

                    result.metrics[index] =
                        SceneMetricsCalculator::compute(instances[index].scene_data, instances[index].seed, options);
                    if (completed_counter != nullptr) {
                        completed_counter->fetch_add(1);
                    }
                }
            });
        }

        for (auto& thread : workers) {
            if (thread.joinable()) {
                thread.join();
            }
        }

        return result;
    }

} // namespace experiment
