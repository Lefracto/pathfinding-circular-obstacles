# Experiment Core

Этот модуль добавляет базовое ядро экспериментов для ВКР:
- загрузка `generator_config.json`,
- генерация набора сцен по распределениям и `seed`,
- расчёт метрик среды по кнопке в UI или автоматически (по конфигу),
- сборка in-memory модели эксперимента,
- сохранение/загрузка `experiment.json`,
- batch-запуск папки конфигов из workspace UI (`File -> Run Config Folder...`) с автогенерацией, метриками, прогонами алгоритмов и сохранением экспериментов в подпапку `experiment_runs`.

## Файлы

- `include/experiment/config/GeneratorConfig.h`:
  структуры конфига генерации и распределений.
- `include/experiment/config/GeneratorConfigIO.h`, `experiment/config/GeneratorConfigIO.cpp`:
  JSON сериализация/десериализация конфига.
- `include/experiment/generation/SceneBatchGenerator.h`, `experiment/generation/SceneBatchGenerator.cpp`:
  генерация сцен из конфига.
- `include/experiment/core/ExperimentModel.h`:
  модель эксперимента/экземпляра/прогонов.
- `include/experiment/core/ExperimentService.h`, `experiment/core/ExperimentService.cpp`:
  сценарии `create_experiment` и `generate_instances`.
- `include/experiment/core/ExperimentIO.h`, `experiment/core/ExperimentIO.cpp`:
  JSON сериализация/десериализация `experiment.json`.
- `include/experiment/metrics/SceneMetricsCalculator.h`, `experiment/metrics/SceneMetricsCalculator.cpp`:
  расчёт метрик одной сцены (Monte Carlo/регулярная сетка + grid-based метрики свободного пространства).
- `include/experiment/metrics/SceneMetricsBatchCalculator.h`, `experiment/metrics/SceneMetricsBatchCalculator.cpp`:
  пакетный расчёт метрик с параллельной обработкой сцен.


## Generator Config: параметры

### Корень
- `version` (`int`): версия схемы.
- `name` (`string`): имя эксперимента.
- `instance_count` (`uint`): число генерируемых сцен.
- `global_seed` (`uint`): базовый seed; seed сцены = `global_seed + index`.
- `scene`:
  - `width`, `height` (`double > 0`): размеры рабочей области.
- `obstacles`:
  - `count_distribution`: распределение числа препятствий.
  - `radius_distribution`: распределение радиусов.
  - `center_distribution`: распределение центров (`x` и `y`).
  - `allow_overlap` (`bool`): разрешить пересечения препятствий.
  - `require_intersection_with_scene` (`bool`): учитывать только препятствия, пересекающие рабочую область.
  - `max_attempts_per_obstacle` (`uint > 0`): лимит попыток подбора.
- `start_goal`:
  - `start`, `goal`:
    - `mode`: `fixed` или `random_free`.
    - `fixed_point`: для `fixed`.
    - `distribution`: для `random_free` (опционально; если нет, равномерно по области).
  - `min_distance` (`double >= 0`): минимум между стартом и целью.
  - `max_attempts` (`uint > 0`): лимит попыток для random_free.
- `algorithms` (массив): id/флаги/параметры алгоритмов для будущих прогонов.
- `metrics`:
  - `auto_compute_on_generation` (`bool`): считать метрики сразу после генерации.
  - `execution_mode` (`string`): `serial` или `parallel`.
  - `worker_threads` (`uint`): число потоков; `0` = авто.
  - `compute_*` (`bool`): включение/выключение отдельных метрик.
  - `obstacle_area_ratio_method` (`string`): `monte_carlo` или `grid_sampling`.
  - `obstacle_area_samples` (`uint`): число выборок Monte Carlo.
  - `obstacle_area_grid_resolution` (`uint`): разрешение сетки для `grid_sampling`.
  - `boundary_angle_samples` (`uint`): дискретизация границ для boundary density.
  - `cell_grid_x`, `cell_grid_y` (`uint`): сетка для CV по ячейкам.
  - `cell_density_samples` (`uint`): число выборок для локальных плотностей.
  - `clearance_grid_x`, `clearance_grid_y` (`uint`): сетка для clearance в свободном пространстве.
  - `clearance_quantile` (`double` in `[0,1]`): квантиль локального clearance по выбранной свободной области.
  - `clearance_region_mode` (`string`): `largest_free_component`, `start_goal_component` или `start_goal_or_largest`.
  - `connectivity_grid_x`, `connectivity_grid_y` (`uint`): сетка связности.
- `output.save_debug_structures` (`bool`): флаг для будущих debug-структур.

### Поддерживаемые распределения
- Для вещественных:
  - `fixed`: `{ "type": "fixed", "value": ... }`
  - `uniform_real`: `{ "type": "uniform_real", "min": ..., "max": ... }`
  - `normal`: `{ "type": "normal", "mean": ..., "stddev": ..., "clamp_min": ..., "clamp_max": ... }`
- Для целых:
  - `fixed`: `{ "type": "fixed", "value": ... }`
  - `uniform_int`: `{ "type": "uniform_int", "min": ..., "max": ... }`
  - `discrete_int`: `{ "type": "discrete_int", "values": [...], "weights": [...] }`

## Experiment File

`experiment.json` содержит:
- метаданные (`experiment_name`, `created_at`, `status`),
- `generator_config_snapshot` (полная копия использованного конфига),
- список алгоритмов,
- список `instances`.

Каждый `instance` содержит:
- `instance_id`, `seed`,
- `generation_status`, `run_status`,
- `scene_data`,
- `scene_metrics`:
  - `obstacle_area_ratio`
  - `boundary_density`
  - `cell_density_cv`
  - `radii_cv`
  - `clearance` (квантиль расстояния до ближайшего препятствия/границы в выбранной свободной компоненте)
  - `largest_component_ratio`
- `runs` (заготовки/результаты алгоритмов, включая путь и метрики).

Каждый `run` содержит, помимо пути и тайминга:
- `status`: итог прогона (`Success`, `NoPath`, `Failed` ...)
- `success`: булевый флаг успешного построения пути
- `path_found`: явный булевый признак того, что в этой попытке алгоритм вернул путь

Практический смысл такой:
- `path_found = true` или `status = Success`: алгоритм нашёл путь в этой попытке
- `path_found = false` и `status = NoPath`: алгоритм путь не нашёл в этой попытке
- `status = Failed`: алгоритм завершился с ошибкой, это отдельный случай

Важно: `path_found = false` не доказывает, что пути в сцене вообще не существует. Это означает только, что конкретный алгоритм в конкретном прогоне путь не вернул.






