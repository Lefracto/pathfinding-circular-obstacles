from __future__ import annotations

import json
import math
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Any

import pandas as pd


EXPERIMENT_FILE_PATTERN = re.compile(r"exp_config_(\d+)_experiment\.json$", re.IGNORECASE)
SCENE_ID_PATTERN = re.compile(r"scene_(\d+)$", re.IGNORECASE)
SCENE_METRIC_PREFIX = "scene_metric_"
DATASET_NAMES = ("runs", "runs_analysis", "scenes", "files")


@dataclass
class ExperimentDatasetBundle:
    run_df: pd.DataFrame
    analysis_run_df: pd.DataFrame
    scene_df: pd.DataFrame
    file_df: pd.DataFrame
    summary: dict[str, Any]


def flatten_mapping(value: Any, prefix: str = "") -> dict[str, Any]:
    items: dict[str, Any] = {}

    if isinstance(value, dict):
        for key, nested_value in value.items():
            nested_prefix = f"{prefix}_{key}" if prefix else str(key)
            items.update(flatten_mapping(nested_value, nested_prefix))
        return items

    if isinstance(value, list):
        items[prefix] = json.dumps(value, ensure_ascii=False, sort_keys=True)
        return items

    items[prefix] = value
    return items


def json_text(value: Any) -> str:
    return json.dumps(value if value is not None else {}, ensure_ascii=False, sort_keys=True)


def parse_config_index(experiment_path: Path) -> int | None:
    match = EXPERIMENT_FILE_PATTERN.search(experiment_path.name)
    if match is None:
        return None
    return int(match.group(1))


def parse_scene_index(instance_id: str) -> int | None:
    match = SCENE_ID_PATTERN.search(instance_id)
    if match is None:
        return None
    return int(match.group(1))


def safe_float(value: Any) -> float | None:
    if value is None:
        return None
    if isinstance(value, bool):
        return float(value)
    if isinstance(value, (int, float)):
        return float(value)
    return None


def direct_distance(start: dict[str, Any], goal: dict[str, Any]) -> float | None:
    start_x = safe_float(start.get("x"))
    start_y = safe_float(start.get("y"))
    goal_x = safe_float(goal.get("x"))
    goal_y = safe_float(goal.get("y"))

    if None in (start_x, start_y, goal_x, goal_y):
        return None

    return math.hypot(goal_x - start_x, goal_y - start_y)


def build_file_summary_record(
    experiment_path: Path,
    experiment_data: dict[str, Any],
    run_records: list[dict[str, Any]],
    scene_records: list[dict[str, Any]],
    theoretical_max_run_rows: int,
) -> dict[str, Any]:
    executed_run_count = sum(1 for record in run_records if bool(record["algorithm_was_executed"]))
    not_run_count = sum(1 for record in run_records if record["algorithm_status"] == "NotRun")
    generated_scene_count = sum(
        1 for record in scene_records if record["scene_generation_status"] == "Generated"
    )
    completed_scene_count = sum(
        1 for record in scene_records if record["scene_run_status"] == "Completed"
    )
    any_path_scene_count = sum(
        1 for record in scene_records if bool(record["any_algorithm_path_found"])
    )
    path_found_run_count = sum(1 for record in run_records if bool(record["path_found"]))
    failed_run_count = sum(1 for record in run_records if record["algorithm_status"] == "Failed")

    return {
        "experiment_file": experiment_path.name,
        "experiment_path": str(experiment_path),
        "config_index": parse_config_index(experiment_path),
        "experiment_name": experiment_data.get("experiment_name"),
        "config_name": experiment_data.get("generator_config_snapshot", {}).get("name"),
        "experiment_status": experiment_data.get("status"),
        "scene_count_declared": experiment_data.get("generator_config_snapshot", {}).get("instance_count"),
        "actual_scene_rows": len(scene_records),
        "generated_scene_count": generated_scene_count,
        "completed_scene_count": completed_scene_count,
        "any_path_scene_count": any_path_scene_count,
        "enabled_algorithm_count": len(experiment_data.get("algorithms", [])),
        "theoretical_max_run_rows": theoretical_max_run_rows,
        "actual_run_rows": len(run_records),
        "executed_run_count": executed_run_count,
        "not_run_count": not_run_count,
        "path_found_run_count": path_found_run_count,
        "failed_run_count": failed_run_count,
    }


def parse_experiment_file(experiment_path: str | Path) -> tuple[list[dict[str, Any]], list[dict[str, Any]], dict[str, Any]]:
    experiment_path = Path(experiment_path)
    with experiment_path.open("r", encoding="utf-8") as fp:
        experiment_data = json.load(fp)

    config = experiment_data.get("generator_config_snapshot", {})
    config_index = parse_config_index(experiment_path)

    top_level_algorithms = experiment_data.get("algorithms", [])
    top_level_algorithm_map = {algorithm.get("id"): algorithm for algorithm in top_level_algorithms}
    enabled_algorithm_count = sum(1 for algorithm in top_level_algorithms if algorithm.get("enabled", True))
    theoretical_max_run_rows = int(config.get("instance_count", 0) or 0) * enabled_algorithm_count

    obstacles_cfg = config.get("obstacles", {}) or {}
    metrics_cfg = config.get("metrics", {}) or {}
    start_goal_cfg = config.get("start_goal", {}) or {}

    config_fields = {
        "config_index": config_index,
        "experiment_file": experiment_path.name,
        "experiment_path": str(experiment_path),
        "experiment_name": experiment_data.get("experiment_name"),
        "experiment_status": experiment_data.get("status"),
        "experiment_created_at": experiment_data.get("created_at"),
        "experiment_last_saved_at": experiment_data.get("last_saved_at"),
        "experiment_global_seed": experiment_data.get("global_seed"),
        "source_config_path": experiment_data.get("source_config_path"),
        "config_name": config.get("name"),
        "config_instance_count": config.get("instance_count"),
        "enabled_algorithm_count": enabled_algorithm_count,
        "config_scene_width": safe_float((config.get("scene") or {}).get("width")),
        "config_scene_height": safe_float((config.get("scene") or {}).get("height")),
        "config_obstacles_allow_overlap": obstacles_cfg.get("allow_overlap"),
        "config_obstacles_require_intersection_with_scene": obstacles_cfg.get("require_intersection_with_scene"),
        "config_obstacles_max_attempts_per_obstacle": obstacles_cfg.get("max_attempts_per_obstacle"),
        "config_start_goal_min_distance": safe_float(start_goal_cfg.get("min_distance")),
        "config_start_goal_max_attempts": start_goal_cfg.get("max_attempts"),
        "config_metrics_auto_compute_on_generation": metrics_cfg.get("auto_compute_on_generation"),
        "config_metrics_execution_mode": metrics_cfg.get("execution_mode"),
        "config_metrics_worker_threads": metrics_cfg.get("worker_threads"),
        "config_scene_json": json_text(config.get("scene")),
        "config_obstacles_json": json_text(obstacles_cfg),
        "config_start_goal_json": json_text(start_goal_cfg),
        "config_metrics_json": json_text(metrics_cfg),
        "config_algorithms_json": json_text(config.get("algorithms", [])),
    }

    flattened_config_fields = {}
    for section_name in ("scene", "obstacles", "start_goal", "metrics", "output"):
        if section_name in config:
            flattened_config_fields.update(
                flatten_mapping(config[section_name], f"config_{section_name}")
            )

    run_records: list[dict[str, Any]] = []
    scene_records: list[dict[str, Any]] = []

    for instance in experiment_data.get("instances", []):
        scene_data = instance.get("scene_data", {}) or {}
        start = scene_data.get("start", {}) or {}
        goal = scene_data.get("goal", {}) or {}
        scene_metrics = instance.get("scene_metrics", {}) or {}

        scene_width = safe_float(scene_data.get("width"))
        scene_height = safe_float(scene_data.get("height"))

        scene_base = {
            **config_fields,
            **flattened_config_fields,
            "scene_id": instance.get("instance_id"),
            "scene_index": parse_scene_index(instance.get("instance_id", "")),
            "scene_seed": instance.get("seed"),
            "scene_generation_status": instance.get("generation_status"),
            "scene_is_generated": instance.get("generation_status") == "Generated",
            "scene_run_status": instance.get("run_status"),
            "scene_is_completed": instance.get("run_status") == "Completed",
            "scene_error_message": instance.get("error_message"),
            "scene_width": scene_width,
            "scene_height": scene_height,
            "scene_area": (
                scene_width * scene_height
                if scene_width is not None and scene_height is not None
                else None
            ),
            "scene_aspect_ratio": (
                scene_width / scene_height
                if scene_width is not None and scene_height not in (None, 0.0)
                else None
            ),
            "scene_obstacle_count": len(scene_data.get("obstacles", []) or []),
            "scene_start_x": safe_float(start.get("x")),
            "scene_start_y": safe_float(start.get("y")),
            "scene_goal_x": safe_float(goal.get("x")),
            "scene_goal_y": safe_float(goal.get("y")),
            "scene_direct_distance": direct_distance(start, goal),
        }

        scene_metric_fields = {
            f"{SCENE_METRIC_PREFIX}{metric_name}": metric_value
            for metric_name, metric_value in scene_metrics.items()
        }

        runs = instance.get("runs", []) or []
        found_by_algorithm_count = 0
        failed_algorithm_count = 0
        no_path_algorithm_count = 0
        found_runtime_values: list[float] = []
        found_path_length_values: list[float] = []

        for run in runs:
            algorithm_id = run.get("algorithm_id")
            algorithm_config = top_level_algorithm_map.get(algorithm_id, {})
            resolved_params = ((run.get("debug_info") or {}).get("resolved_params")) or {}
            algorithm_params = resolved_params or algorithm_config.get("params") or {}

            path_found = bool(run.get("path_found", False))
            algorithm_status = run.get("status")
            runtime_ms_raw = safe_float(run.get("runtime_ms"))
            path_metrics = run.get("path_metrics", {}) or {}
            path_length_raw = safe_float(path_metrics.get("path_length"))
            relative_path_length_raw = safe_float(path_metrics.get("relative_path_length"))

            if path_found:
                found_by_algorithm_count += 1
                if runtime_ms_raw is not None:
                    found_runtime_values.append(runtime_ms_raw)
                if path_length_raw is not None:
                    found_path_length_values.append(path_length_raw)
            elif algorithm_status == "Failed":
                failed_algorithm_count += 1
            elif algorithm_status == "NoPath":
                no_path_algorithm_count += 1

            run_record = {
                **scene_base,
                **scene_metric_fields,
                **flatten_mapping(algorithm_params, "alg_param"),
                "algorithm_id": algorithm_id,
                "algorithm_status": algorithm_status,
                "algorithm_was_executed": algorithm_status in {"Success", "NoPath", "Failed"},
                "algorithm_not_run": algorithm_status == "NotRun",
                "algorithm_no_path": algorithm_status == "NoPath",
                "algorithm_failed": algorithm_status == "Failed",
                "algorithm_success": run.get("success"),
                "algorithm_params_json": json_text(algorithm_params),
                "algorithm_resolved_params_json": json_text(resolved_params),
                "algorithm_debug_info_json": json_text(run.get("debug_info") or {}),
                "path_found": path_found,
                "algorithm_error_message": run.get("error_message"),
                "runtime_ms_raw": runtime_ms_raw,
                "runtime_ms_path_only": runtime_ms_raw if path_found and runtime_ms_raw is not None else -1.0,
                "runtime_ms_no_path_only": runtime_ms_raw if (not path_found and algorithm_status == "NoPath" and runtime_ms_raw is not None) else -1.0,
                "path_length_raw": path_length_raw,
                "path_length_path_only": path_length_raw if path_found and path_length_raw is not None else -1.0,
                "relative_path_length_raw": relative_path_length_raw,
                "relative_path_length_path_only": (
                    relative_path_length_raw
                    if path_found and relative_path_length_raw is not None
                    else -1.0
                ),
                "has_runtime_measurement": runtime_ms_raw is not None,
                "has_path_metrics": path_length_raw is not None,
            }
            run_records.append(run_record)

        scene_record = {
            **scene_base,
            **scene_metric_fields,
            "algorithm_run_count": len(runs),
            "any_algorithm_path_found": found_by_algorithm_count > 0,
            "all_algorithms_path_found": len(runs) > 0 and found_by_algorithm_count == len(runs),
            "found_by_algorithm_count": found_by_algorithm_count,
            "failed_algorithm_count": failed_algorithm_count,
            "no_path_algorithm_count": no_path_algorithm_count,
            "best_found_runtime_ms": min(found_runtime_values) if found_runtime_values else None,
            "best_found_path_length": min(found_path_length_values) if found_path_length_values else None,
        }
        scene_records.append(scene_record)

    file_record = build_file_summary_record(
        experiment_path=experiment_path,
        experiment_data=experiment_data,
        run_records=run_records,
        scene_records=scene_records,
        theoretical_max_run_rows=theoretical_max_run_rows,
    )

    return run_records, scene_records, file_record


def build_analysis_run_dataset(run_df: pd.DataFrame) -> pd.DataFrame:
    if run_df.empty:
        return pd.DataFrame()

    analysis_df = run_df.copy()

    metric_columns = sorted(
        column for column in analysis_df.columns if column.startswith(SCENE_METRIC_PREFIX)
    )

    analysis_df["path_found"] = analysis_df["path_found"].fillna(False).astype(bool)
    analysis_df["algorithm_was_executed"] = analysis_df["algorithm_was_executed"].fillna(False).astype(bool)
    analysis_df["algorithm_not_run"] = analysis_df["algorithm_not_run"].fillna(False).astype(bool)
    analysis_df["algorithm_no_path"] = analysis_df["algorithm_no_path"].fillna(False).astype(bool)
    analysis_df["algorithm_failed"] = analysis_df["algorithm_failed"].fillna(False).astype(bool)
    analysis_df["algorithm_success"] = analysis_df["algorithm_success"].fillna(False).astype(bool)
    analysis_df["scene_is_generated"] = analysis_df["scene_is_generated"].fillna(False).astype(bool)
    analysis_df["scene_is_completed"] = analysis_df["scene_is_completed"].fillna(False).astype(bool)

    float_fill_columns = (
        "runtime_ms_raw",
        "runtime_ms_no_path_only",
        "path_length_raw",
        "relative_path_length_raw",
        "scene_width",
        "scene_height",
        "scene_area",
        "scene_aspect_ratio",
        "scene_start_x",
        "scene_start_y",
        "scene_goal_x",
        "scene_goal_y",
        "scene_direct_distance",
        "config_scene_width",
        "config_scene_height",
        "config_start_goal_min_distance",
    )
    for column in float_fill_columns:
        if column in analysis_df.columns:
            analysis_df[column] = analysis_df[column].fillna(-1.0)

    int_fill_columns = (
        "config_index",
        "config_instance_count",
        "scene_index",
        "scene_seed",
        "scene_obstacle_count",
        "config_obstacles_max_attempts_per_obstacle",
        "config_start_goal_max_attempts",
        "config_metrics_worker_threads",
    )
    for column in int_fill_columns:
        if column in analysis_df.columns:
            analysis_df[column] = analysis_df[column].fillna(-1).astype(int)

    text_fill_columns = (
        "scene_generation_status",
        "scene_run_status",
        "algorithm_status",
        "scene_error_message",
        "algorithm_error_message",
        "config_name",
        "experiment_name",
        "source_config_path",
        "config_scene_json",
        "config_obstacles_json",
        "config_start_goal_json",
        "config_metrics_json",
        "algorithm_params_json",
        "algorithm_resolved_params_json",
    )
    for column in text_fill_columns:
        if column in analysis_df.columns:
            analysis_df[column] = analysis_df[column].fillna("")

    analysis_df["runtime_ms"] = analysis_df["runtime_ms_raw"]
    analysis_df["path_length"] = analysis_df["path_length_path_only"].fillna(-1.0)
    analysis_df["relative_path_length"] = analysis_df["relative_path_length_path_only"].fillna(-1.0)

    for metric_column in metric_columns:
        analysis_df[metric_column] = analysis_df[metric_column].fillna(-1.0)

    dense_columns = [
        "config_index",
        "config_name",
        "experiment_file",
        "experiment_name",
        "source_config_path",
        "scene_id",
        "scene_index",
        "scene_seed",
        "scene_generation_status",
        "scene_run_status",
        "scene_is_generated",
        "scene_is_completed",
        "scene_error_message",
        "scene_width",
        "scene_height",
        "scene_area",
        "scene_aspect_ratio",
        "scene_obstacle_count",
        "scene_start_x",
        "scene_start_y",
        "scene_goal_x",
        "scene_goal_y",
        "scene_direct_distance",
        "algorithm_id",
        "algorithm_status",
        "algorithm_was_executed",
        "algorithm_not_run",
        "algorithm_no_path",
        "algorithm_failed",
        "algorithm_success",
        "path_found",
        "algorithm_error_message",
        "runtime_ms",
        "runtime_ms_raw",
        "runtime_ms_no_path_only",
        "path_length",
        "path_length_raw",
        "relative_path_length",
        "relative_path_length_raw",
        "config_scene_width",
        "config_scene_height",
        "config_obstacles_allow_overlap",
        "config_obstacles_require_intersection_with_scene",
        "config_obstacles_max_attempts_per_obstacle",
        "config_start_goal_min_distance",
        "config_start_goal_max_attempts",
        "config_metrics_auto_compute_on_generation",
        "config_metrics_execution_mode",
        "config_metrics_worker_threads",
        "config_scene_json",
        "config_obstacles_json",
        "config_start_goal_json",
        "config_metrics_json",
        "algorithm_params_json",
        "algorithm_resolved_params_json",
    ]

    selected_columns = [column for column in dense_columns if column in analysis_df.columns]
    selected_columns.extend(metric_columns)

    analysis_df = analysis_df[selected_columns].sort_values(
        ["config_index", "scene_index", "algorithm_id"]
    ).reset_index(drop=True)
    return analysis_df

def build_experiment_dataset(experiments_dir: str | Path) -> ExperimentDatasetBundle:
    experiments_dir = Path(experiments_dir)
    experiment_paths = sorted(experiments_dir.glob("*_experiment.json"))

    if not experiment_paths:
        raise FileNotFoundError(f"No '*_experiment.json' files found in: {experiments_dir}")

    all_run_records: list[dict[str, Any]] = []
    all_scene_records: list[dict[str, Any]] = []
    file_records: list[dict[str, Any]] = []

    for experiment_path in experiment_paths:
        run_records, scene_records, file_record = parse_experiment_file(experiment_path)
        all_run_records.extend(run_records)
        all_scene_records.extend(scene_records)
        file_records.append(file_record)

    run_df = pd.DataFrame(all_run_records)
    scene_df = pd.DataFrame(all_scene_records)
    file_df = pd.DataFrame(file_records).sort_values("config_index").reset_index(drop=True)
    analysis_run_df = build_analysis_run_dataset(run_df)

    summary = {
        "experiment_file_count": len(experiment_paths),
        "run_row_count": int(len(run_df)),
        "analysis_run_row_count": int(len(analysis_run_df)),
        "scene_row_count": int(len(scene_df)),
        "theoretical_max_run_rows": int(file_df["theoretical_max_run_rows"].sum()),
        "actual_run_rows": int(file_df["actual_run_rows"].sum()),
        "executed_run_count": int(file_df["executed_run_count"].sum()),
        "not_run_count": int(file_df["not_run_count"].sum()),
        "generated_scene_count": int(file_df["generated_scene_count"].sum()),
        "completed_scene_count": int(file_df["completed_scene_count"].sum()),
        "path_found_run_count": int(file_df["path_found_run_count"].sum()),
        "failed_run_count": int(file_df["failed_run_count"].sum()),
    }

    return ExperimentDatasetBundle(
        run_df=run_df,
        analysis_run_df=analysis_run_df,
        scene_df=scene_df,
        file_df=file_df,
        summary=summary,
    )


def _dataset_cache_candidates(output_dir: Path, prefix: str, dataset_name: str) -> tuple[Path, Path]:
    parquet_path = output_dir / f"{prefix}_{dataset_name}.parquet"
    csv_path = output_dir / f"{prefix}_{dataset_name}.csv.gz"
    return parquet_path, csv_path


def get_dataset_bundle_paths(
    output_dir: str | Path,
    prefix: str = "experiment_runs",
) -> dict[str, Path]:
    output_dir = Path(output_dir)

    saved_paths: dict[str, Path] = {}
    missing_entries: list[str] = []

    for dataset_name in DATASET_NAMES:
        parquet_path, csv_path = _dataset_cache_candidates(output_dir, prefix, dataset_name)
        if parquet_path.exists():
            saved_paths[dataset_name] = parquet_path
        elif csv_path.exists():
            saved_paths[dataset_name] = csv_path
        else:
            missing_entries.append(dataset_name)

    summary_path = output_dir / f"{prefix}_summary.json"
    if summary_path.exists():
        saved_paths["summary"] = summary_path
    else:
        missing_entries.append("summary")

    if missing_entries:
        missing_text = ", ".join(missing_entries)
        raise FileNotFoundError(
            f"Missing cached dataset files for prefix '{prefix}' in '{output_dir}': {missing_text}"
        )

    return saved_paths


def _read_saved_dataframe(path: Path) -> pd.DataFrame:
    if str(path).endswith(".parquet"):
        return pd.read_parquet(path)
    return pd.read_csv(path)


def load_dataset_bundle(
    output_dir: str | Path,
    prefix: str = "experiment_runs",
) -> ExperimentDatasetBundle:
    saved_paths = get_dataset_bundle_paths(output_dir, prefix=prefix)

    with saved_paths["summary"].open("r", encoding="utf-8") as fp:
        summary = json.load(fp)

    return ExperimentDatasetBundle(
        run_df=_read_saved_dataframe(saved_paths["runs"]),
        analysis_run_df=_read_saved_dataframe(saved_paths["runs_analysis"]),
        scene_df=_read_saved_dataframe(saved_paths["scenes"]),
        file_df=_read_saved_dataframe(saved_paths["files"]),
        summary=summary,
    )


def get_dataset_cache_info(
    experiments_dir: str | Path,
    output_dir: str | Path,
    prefix: str = "experiment_runs",
    dependency_paths: list[str | Path] | None = None,
) -> dict[str, Any]:
    experiments_dir = Path(experiments_dir)
    experiment_paths = sorted(experiments_dir.glob("*_experiment.json"))

    if not experiment_paths:
        raise FileNotFoundError(f"No '*_experiment.json' files found in: {experiments_dir}")

    try:
        saved_paths = get_dataset_bundle_paths(output_dir, prefix=prefix)
    except FileNotFoundError as exc:
        return {
            "cache_available": False,
            "is_fresh": False,
            "reason": str(exc),
            "saved_paths": {},
        }

    freshness_inputs = [*experiment_paths, Path(__file__)]
    if dependency_paths:
        freshness_inputs.extend(Path(path) for path in dependency_paths)
    freshness_inputs = [path for path in freshness_inputs if path.exists()]

    latest_source_mtime = max(path.stat().st_mtime for path in freshness_inputs)
    oldest_cache_mtime = min(path.stat().st_mtime for path in saved_paths.values())
    is_fresh = oldest_cache_mtime >= latest_source_mtime

    return {
        "cache_available": True,
        "is_fresh": is_fresh,
        "reason": "" if is_fresh else "Cache is older than source experiments or parser code.",
        "saved_paths": {name: str(path) for name, path in saved_paths.items()},
        "latest_source_mtime": latest_source_mtime,
        "oldest_cache_mtime": oldest_cache_mtime,
    }


def load_or_build_experiment_dataset(
    experiments_dir: str | Path,
    output_dir: str | Path,
    prefix: str = "experiment_runs",
    force_rebuild: bool = False,
) -> tuple[ExperimentDatasetBundle, dict[str, Any]]:
    cache_info = get_dataset_cache_info(experiments_dir, output_dir, prefix=prefix)

    if cache_info["cache_available"] and cache_info["is_fresh"] and not force_rebuild:
        bundle = load_dataset_bundle(output_dir, prefix=prefix)
        return bundle, {
            **cache_info,
            "mode": "cache",
        }

    bundle = build_experiment_dataset(experiments_dir)
    saved_paths = save_dataset_bundle(bundle, output_dir, prefix=prefix)
    refreshed_cache_info = get_dataset_cache_info(experiments_dir, output_dir, prefix=prefix)

    rebuild_mode = "forced_rebuild" if force_rebuild else "rebuilt"
    return bundle, {
        **refreshed_cache_info,
        "mode": rebuild_mode,
        "saved_paths": saved_paths,
    }


def save_dataset_bundle(
    bundle: ExperimentDatasetBundle,
    output_dir: str | Path,
    prefix: str = "experiment_runs",
) -> dict[str, str]:
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    saved_paths: dict[str, str] = {}
    for dataset_name, dataframe in (
        ("runs", bundle.run_df),
        ("runs_analysis", bundle.analysis_run_df),
        ("scenes", bundle.scene_df),
        ("files", bundle.file_df),
    ):
        parquet_path, csv_path = _dataset_cache_candidates(output_dir, prefix, dataset_name)

        try:
            dataframe.to_parquet(parquet_path, index=False)
            saved_paths[dataset_name] = str(parquet_path)
        except Exception:
            dataframe.to_csv(csv_path, index=False)
            saved_paths[dataset_name] = str(csv_path)

    summary_path = output_dir / f"{prefix}_summary.json"
    with summary_path.open("w", encoding="utf-8") as fp:
        json.dump(bundle.summary, fp, indent=2, ensure_ascii=False)
    saved_paths["summary"] = str(summary_path)

    return saved_paths

