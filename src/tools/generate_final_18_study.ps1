$ErrorActionPreference = 'Stop'

function New-UniformReal([double]$min, [double]$max) {
    return [ordered]@{
        type = 'uniform_real'
        min = $min
        max = $max
    }
}

function New-NormalReal([double]$mean, [double]$stddev, [double]$clampMin, [double]$clampMax) {
    return [ordered]@{
        type = 'normal'
        mean = $mean
        stddev = $stddev
        clamp_min = $clampMin
        clamp_max = $clampMax
    }
}

function New-UniformInt([int]$min, [int]$max) {
    return [ordered]@{
        type = 'uniform_int'
        min = $min
        max = $max
    }
}

function New-DiscreteInt([int[]]$values, [double[]]$weights) {
    return [ordered]@{
        type = 'discrete_int'
        values = $values
        weights = $weights
    }
}

function New-PointDistribution($xDist, $yDist) {
    return [ordered]@{
        x = $xDist
        y = $yDist
    }
}

function New-RandomEndpoint([double]$xmin, [double]$xmax, [double]$ymin, [double]$ymax) {
    return [ordered]@{
        mode = 'random_free'
        distribution = [ordered]@{
            x = New-UniformReal $xmin $xmax
            y = New-UniformReal $ymin $ymax
        }
    }
}

function New-Algorithms() {
    return @(
        [ordered]@{
            id = 'rrt'
            enabled = $true
            params = [ordered]@{
                max_iterations = 10000
                step_size = 2.5
                goal_sample_rate = 0.08
                goal_tolerance = 2.5
                enable_path_smoothing = $true
                smoothing_iterations = 120
            }
        },
        [ordered]@{
            id = 'grid_dijkstra'
            enabled = $true
            params = [ordered]@{
                grid_step = 3.5
                allow_diagonal = $true
            }
        },
        [ordered]@{
            id = 'grid_astar'
            enabled = $true
            params = [ordered]@{
                grid_step = 3.5
                allow_diagonal = $true
            }
        },
        [ordered]@{
            id = 'visibility_dijkstra'
            enabled = $true
            params = [ordered]@{
                points_per_obstacle = 16
            }
        },
        [ordered]@{
            id = 'visibility_astar'
            enabled = $true
            params = [ordered]@{
                points_per_obstacle = 16
            }
        }
    )
}

function New-Metrics() {
    return [ordered]@{
        auto_compute_on_generation = $false
        execution_mode = 'parallel'
        worker_threads = 0
        compute_obstacle_area_ratio = $true
        compute_boundary_density = $true
        compute_cell_density_cv = $true
        compute_radii_cv = $true
        compute_clearance = $true
        compute_largest_component_ratio = $true
        obstacle_area_ratio_method = 'monte_carlo'
        obstacle_area_samples = 10000
        obstacle_area_grid_resolution = 180
        boundary_angle_samples = 240
        cell_grid_x = 12
        cell_grid_y = 12
        cell_density_samples = 7000
        clearance_grid_x = 140
        clearance_grid_y = 140
        clearance_quantile = 0.10
        clearance_region_mode = 'start_goal_or_largest'
        connectivity_grid_x = 140
        connectivity_grid_y = 140
    }
}

function New-Output() {
    return [ordered]@{
        save_debug_structures = $false
    }
}

$root = Split-Path -Parent $PSScriptRoot
$configDir = Join-Path $root 'configs/studies/final_18'
$experimentDir = Join-Path $root 'experiments/final_18_runs'
New-Item -ItemType Directory -Force -Path $configDir | Out-Null
New-Item -ItemType Directory -Force -Path $experimentDir | Out-Null

$specs = @(
    [ordered]@{
        file = '01_uniform_compact_light.json'
        config_name = 'final18_uniform_compact_light'
        family = 'uniform_compact'
        difficulty = 'light'
        summary = 'Compact square scenes with a few small uniformly distributed obstacles and wide free-space corridors.'
        segment = 'Small-workspace, low-density isotropic random clutter.'
        workspace = [ordered]@{ width = 100.0; height = 100.0 }
        count_distribution = New-UniformInt 10 18
        radius_distribution = New-UniformReal 1.8 3.4
        center_distribution = New-PointDistribution (New-UniformReal 0.0 100.0) (New-UniformReal 0.0 100.0)
        start = New-RandomEndpoint 8.0 28.0 8.0 28.0
        goal = New-RandomEndpoint 72.0 92.0 72.0 92.0
        min_distance = 55.0
        obstacle_attempts = 350
        start_goal_attempts = 4000
        global_seed = 81001
        scene_notes = 'Mostly open compact layouts; direct paths often break but broad alternate corridors usually remain.'
        metric_signature = 'Low obstacle area ratio, high clearance, high largest free component ratio.'
    },
    [ordered]@{
        file = '02_uniform_compact_medium.json'
        config_name = 'final18_uniform_compact_medium'
        family = 'uniform_compact'
        difficulty = 'medium'
        summary = 'Compact square scenes with balanced small-to-medium random clutter and frequent local detours.'
        segment = 'Small-workspace, medium-density isotropic clutter.'
        workspace = [ordered]@{ width = 100.0; height = 100.0 }
        count_distribution = New-UniformInt 18 28
        radius_distribution = New-UniformReal 2.1 3.9
        center_distribution = New-PointDistribution (New-UniformReal 0.0 100.0) (New-UniformReal 0.0 100.0)
        start = New-RandomEndpoint 8.0 28.0 8.0 28.0
        goal = New-RandomEndpoint 72.0 92.0 72.0 92.0
        min_distance = 60.0
        obstacle_attempts = 450
        start_goal_attempts = 5000
        global_seed = 81002
        scene_notes = 'Dense local interactions in a compact workspace; straight-line solutions are less frequent.'
        metric_signature = 'Moderate obstacle area ratio, reduced clearance, still mostly connected free space.'
    },
    [ordered]@{
        file = '03_uniform_compact_dense.json'
        config_name = 'final18_uniform_compact_dense'
        family = 'uniform_compact'
        difficulty = 'high'
        summary = 'Compact square scenes with dense uniform clutter where many routes are narrow and highly local.'
        segment = 'Small-workspace, high-density isotropic clutter.'
        workspace = [ordered]@{ width = 100.0; height = 100.0 }
        count_distribution = New-UniformInt 28 40
        radius_distribution = New-UniformReal 2.3 4.3
        center_distribution = New-PointDistribution (New-UniformReal 0.0 100.0) (New-UniformReal 0.0 100.0)
        start = New-RandomEndpoint 8.0 28.0 8.0 28.0
        goal = New-RandomEndpoint 72.0 92.0 72.0 92.0
        min_distance = 65.0
        obstacle_attempts = 650
        start_goal_attempts = 6000
        global_seed = 81003
        scene_notes = 'Tight compact scenes with many short detours and frequent narrow micro-passages.'
        metric_signature = 'Higher obstacle area ratio, lower clearance, stronger spread in local cell densities.'
    },
    [ordered]@{
        file = '04_uniform_large_disks_light.json'
        config_name = 'final18_uniform_large_disks_light'
        family = 'uniform_large_disks'
        difficulty = 'light'
        summary = 'Medium square scenes with a few large dominant disks creating macro detours and broad pockets of free space.'
        segment = 'Large-obstacle random fields, light density.'
        workspace = [ordered]@{ width = 150.0; height = 150.0 }
        count_distribution = New-DiscreteInt @(10, 14, 18) @(0.30, 0.45, 0.25)
        radius_distribution = New-NormalReal 4.8 0.9 2.8 6.6
        center_distribution = New-PointDistribution (New-UniformReal 0.0 150.0) (New-UniformReal 0.0 150.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 110.0 140.0 110.0 140.0
        min_distance = 80.0
        obstacle_attempts = 400
        start_goal_attempts = 4500
        global_seed = 81004
        scene_notes = 'Fewer but visually dominant obstacles; path quality depends on choosing the correct side around large disks.'
        metric_signature = 'Moderate obstacle area ratio with comparatively larger clearance variance.'
    },
    [ordered]@{
        file = '05_uniform_large_disks_medium.json'
        config_name = 'final18_uniform_large_disks_medium'
        family = 'uniform_large_disks'
        difficulty = 'medium'
        summary = 'Medium square scenes with more large disks and stronger route-deflecting geometry.'
        segment = 'Large-obstacle random fields, medium density.'
        workspace = [ordered]@{ width = 150.0; height = 150.0 }
        count_distribution = New-DiscreteInt @(16, 22, 28) @(0.25, 0.50, 0.25)
        radius_distribution = New-NormalReal 5.6 1.0 3.0 7.6
        center_distribution = New-PointDistribution (New-UniformReal 0.0 150.0) (New-UniformReal 0.0 150.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 110.0 140.0 110.0 140.0
        min_distance = 88.0
        obstacle_attempts = 550
        start_goal_attempts = 5000
        global_seed = 81005
        scene_notes = 'Routes are shaped by a smaller number of strong blockers rather than by many tiny collisions.'
        metric_signature = 'Higher area ratio than light variant, still moderate cell-density CV compared with clustered families.'
    },
    [ordered]@{
        file = '06_uniform_large_disks_dense.json'
        config_name = 'final18_uniform_large_disks_dense'
        family = 'uniform_large_disks'
        difficulty = 'high'
        summary = 'Medium square scenes with many large disks, producing long detours and frequent line-of-sight disruption.'
        segment = 'Large-obstacle random fields, high density.'
        workspace = [ordered]@{ width = 150.0; height = 150.0 }
        count_distribution = New-DiscreteInt @(22, 30, 38) @(0.25, 0.45, 0.30)
        radius_distribution = New-NormalReal 6.4 1.1 3.2 8.6
        center_distribution = New-PointDistribution (New-UniformReal 0.0 150.0) (New-UniformReal 0.0 150.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 110.0 140.0 110.0 140.0
        min_distance = 95.0
        obstacle_attempts = 800
        start_goal_attempts = 6500
        global_seed = 81006
        scene_notes = 'Macro-detour scenes where one bad side choice can lengthen the route substantially.'
        metric_signature = 'High obstacle area ratio, noticeably lower visibility and lower effective clearance.'
    },
    [ordered]@{
        file = '07_uniform_mixed_scale_light.json'
        config_name = 'final18_uniform_mixed_scale_light'
        family = 'uniform_mixed_scale_large'
        difficulty = 'light'
        summary = 'Large square scenes with heterogeneous obstacle sizes and long-range routes through mostly open space.'
        segment = 'Large-workspace, mixed-scale random obstacles, light density.'
        workspace = [ordered]@{ width = 260.0; height = 260.0 }
        count_distribution = New-UniformInt 20 34
        radius_distribution = New-NormalReal 4.0 2.0 1.2 8.5
        center_distribution = New-PointDistribution (New-UniformReal 0.0 260.0) (New-UniformReal 0.0 260.0)
        start = New-RandomEndpoint 15.0 60.0 15.0 60.0
        goal = New-RandomEndpoint 200.0 245.0 200.0 245.0
        min_distance = 140.0
        obstacle_attempts = 350
        start_goal_attempts = 5000
        global_seed = 81007
        scene_notes = 'Long planning horizons with both tiny and large obstacles; path smoothness starts to matter.'
        metric_signature = 'Low-to-moderate area ratio, large component ratio near 1, broad radii CV.'
    },
    [ordered]@{
        file = '08_uniform_mixed_scale_medium.json'
        config_name = 'final18_uniform_mixed_scale_medium'
        family = 'uniform_mixed_scale_large'
        difficulty = 'medium'
        summary = 'Large square scenes with more heterogeneous obstacles and frequent medium-range rerouting.'
        segment = 'Large-workspace, mixed-scale random obstacles, medium density.'
        workspace = [ordered]@{ width = 260.0; height = 260.0 }
        count_distribution = New-UniformInt 36 56
        radius_distribution = New-NormalReal 4.6 2.2 1.2 9.0
        center_distribution = New-PointDistribution (New-UniformReal 0.0 260.0) (New-UniformReal 0.0 260.0)
        start = New-RandomEndpoint 15.0 60.0 15.0 60.0
        goal = New-RandomEndpoint 200.0 245.0 200.0 245.0
        min_distance = 155.0
        obstacle_attempts = 500
        start_goal_attempts = 6000
        global_seed = 81008
        scene_notes = 'Large scenes where obstacle heterogeneity creates both global and local difficulty at once.'
        metric_signature = 'Moderate area ratio, high radii CV, moderate cell-density CV.'
    },
    [ordered]@{
        file = '09_uniform_mixed_scale_dense.json'
        config_name = 'final18_uniform_mixed_scale_dense'
        family = 'uniform_mixed_scale_large'
        difficulty = 'high'
        summary = 'Large square scenes with dense multi-scale clutter and many competing route structures.'
        segment = 'Large-workspace, mixed-scale random obstacles, high density.'
        workspace = [ordered]@{ width = 260.0; height = 260.0 }
        count_distribution = New-UniformInt 58 84
        radius_distribution = New-NormalReal 5.0 2.3 1.4 9.8
        center_distribution = New-PointDistribution (New-UniformReal 0.0 260.0) (New-UniformReal 0.0 260.0)
        start = New-RandomEndpoint 15.0 60.0 15.0 60.0
        goal = New-RandomEndpoint 200.0 245.0 200.0 245.0
        min_distance = 170.0
        obstacle_attempts = 700
        start_goal_attempts = 7500
        global_seed = 81009
        scene_notes = 'Long routes in large workspaces with many choices, dead-ends and detour scales mixed together.'
        metric_signature = 'Moderate-to-high area ratio, strong radii CV, visible drop in clearance and connectivity margins.'
    },
    [ordered]@{
        file = '10_center_cluster_light.json'
        config_name = 'final18_center_cluster_light'
        family = 'center_cluster'
        difficulty = 'light'
        summary = 'Square scenes with a soft central concentration that encourages peripheral bypasses.'
        segment = 'Center-biased clutter, light concentration.'
        workspace = [ordered]@{ width = 180.0; height = 180.0 }
        count_distribution = New-DiscreteInt @(20, 28, 36) @(0.25, 0.50, 0.25)
        radius_distribution = New-NormalReal 3.2 0.8 1.6 5.2
        center_distribution = New-PointDistribution (New-NormalReal 90.0 55.0 -10.0 190.0) (New-NormalReal 90.0 55.0 -10.0 190.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 140.0 170.0 140.0 170.0
        min_distance = 95.0
        obstacle_attempts = 450
        start_goal_attempts = 5500
        global_seed = 81010
        scene_notes = 'Central congestion is visible but still porous; many scenes admit both center and edge routes.'
        metric_signature = 'Moderate cell-density CV, reduced central clearance, overall connectivity still robust.'
    },
    [ordered]@{
        file = '11_center_cluster_medium.json'
        config_name = 'final18_center_cluster_medium'
        family = 'center_cluster'
        difficulty = 'medium'
        summary = 'Square scenes with a stronger central obstacle cloud and more pronounced ring-like free regions.'
        segment = 'Center-biased clutter, medium concentration.'
        workspace = [ordered]@{ width = 180.0; height = 180.0 }
        count_distribution = New-DiscreteInt @(32, 42, 52) @(0.25, 0.45, 0.30)
        radius_distribution = New-NormalReal 3.8 0.9 1.8 5.8
        center_distribution = New-PointDistribution (New-NormalReal 90.0 40.0 -8.0 188.0) (New-NormalReal 90.0 40.0 -8.0 188.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 140.0 170.0 140.0 170.0
        min_distance = 105.0
        obstacle_attempts = 650
        start_goal_attempts = 6500
        global_seed = 81011
        scene_notes = 'A denser core pushes good routes toward the outskirts and amplifies path-length differences.'
        metric_signature = 'Higher cell-density CV, lower central clearance, strong boundary-vs-center contrast.'
    },
    [ordered]@{
        file = '12_center_cluster_dense.json'
        config_name = 'final18_center_cluster_dense'
        family = 'center_cluster'
        difficulty = 'high'
        summary = 'Square scenes with a dense central mass that often creates ring bypasses and selective narrow crossings.'
        segment = 'Center-biased clutter, high concentration.'
        workspace = [ordered]@{ width = 180.0; height = 180.0 }
        count_distribution = New-DiscreteInt @(44, 54, 66) @(0.25, 0.45, 0.30)
        radius_distribution = New-NormalReal 4.2 1.0 2.0 6.2
        center_distribution = New-PointDistribution (New-NormalReal 90.0 30.0 -5.0 185.0) (New-NormalReal 90.0 30.0 -5.0 185.0)
        start = New-RandomEndpoint 10.0 40.0 10.0 40.0
        goal = New-RandomEndpoint 140.0 170.0 140.0 170.0
        min_distance = 115.0
        obstacle_attempts = 900
        start_goal_attempts = 8000
        global_seed = 81012
        scene_notes = 'The center becomes the dominant structural feature, often forcing wide arc-like bypasses.'
        metric_signature = 'High cell-density CV, low center clearance, reduced straight-line feasibility.'
    },
    [ordered]@{
        file = '13_vertical_band_light.json'
        config_name = 'final18_vertical_band_light'
        family = 'vertical_band'
        difficulty = 'light'
        summary = 'Wide rectangular scenes with a loose vertical clutter belt between left and right free regions.'
        segment = 'Wide-workspace cross-barrier scenes, light band density.'
        workspace = [ordered]@{ width = 280.0; height = 180.0 }
        count_distribution = New-UniformInt 20 32
        radius_distribution = New-UniformReal 2.4 4.6
        center_distribution = New-PointDistribution (New-NormalReal 140.0 40.0 -15.0 295.0) (New-UniformReal 0.0 180.0)
        start = New-RandomEndpoint 10.0 45.0 25.0 155.0
        goal = New-RandomEndpoint 235.0 270.0 25.0 155.0
        min_distance = 165.0
        obstacle_attempts = 500
        start_goal_attempts = 6500
        global_seed = 81013
        scene_notes = 'Crossing from left to right is usually possible through several openings inside a central belt.'
        metric_signature = 'Moderate boundary density, elevated cell-density CV along the x-axis, clearance drops near the center strip.'
    },
    [ordered]@{
        file = '14_vertical_band_medium.json'
        config_name = 'final18_vertical_band_medium'
        family = 'vertical_band'
        difficulty = 'medium'
        summary = 'Wide rectangular scenes with a denser vertical belt that starts to act like a soft barrier.'
        segment = 'Wide-workspace cross-barrier scenes, medium band density.'
        workspace = [ordered]@{ width = 280.0; height = 180.0 }
        count_distribution = New-UniformInt 30 44
        radius_distribution = New-UniformReal 2.8 5.0
        center_distribution = New-PointDistribution (New-NormalReal 140.0 30.0 -12.0 292.0) (New-UniformReal 0.0 180.0)
        start = New-RandomEndpoint 10.0 45.0 25.0 155.0
        goal = New-RandomEndpoint 235.0 270.0 25.0 155.0
        min_distance = 180.0
        obstacle_attempts = 750
        start_goal_attempts = 8000
        global_seed = 81014
        scene_notes = 'The central strip often leaves a few good crossings whose quality varies strongly from scene to scene.'
        metric_signature = 'Higher local density anisotropy, lower central clearance, stronger path-length spread.'
    },
    [ordered]@{
        file = '15_vertical_band_dense.json'
        config_name = 'final18_vertical_band_dense'
        family = 'vertical_band'
        difficulty = 'high'
        summary = 'Wide rectangular scenes with a dense vertical belt that frequently creates bottleneck-like crossings.'
        segment = 'Wide-workspace cross-barrier scenes, high band density.'
        workspace = [ordered]@{ width = 280.0; height = 180.0 }
        count_distribution = New-UniformInt 42 58
        radius_distribution = New-UniformReal 3.0 5.4
        center_distribution = New-PointDistribution (New-NormalReal 140.0 24.0 -10.0 290.0) (New-UniformReal 0.0 180.0)
        start = New-RandomEndpoint 10.0 45.0 25.0 155.0
        goal = New-RandomEndpoint 235.0 270.0 25.0 155.0
        min_distance = 195.0
        obstacle_attempts = 950
        start_goal_attempts = 9500
        global_seed = 81015
        scene_notes = 'These are the most barrier-like scenes in the set; a small number of passages may dominate route quality.'
        metric_signature = 'Low clearance in the belt, high anisotropy, larger variance in success and runtime across algorithms.'
    },
    [ordered]@{
        file = '16_corner_bias_light.json'
        config_name = 'final18_corner_bias_light'
        family = 'corner_bias_tall'
        difficulty = 'light'
        summary = 'Tall rectangular scenes with one obstacle-rich corner and a long diagonal approach from open space.'
        segment = 'Tall-workspace asymmetric clutter, light corner concentration.'
        workspace = [ordered]@{ width = 180.0; height = 280.0 }
        count_distribution = New-DiscreteInt @(18, 26, 34) @(0.25, 0.50, 0.25)
        radius_distribution = New-NormalReal 3.8 1.0 1.8 5.8
        center_distribution = New-PointDistribution (New-NormalReal 45.0 55.0 -15.0 195.0) (New-NormalReal 60.0 70.0 -15.0 295.0)
        start = New-RandomEndpoint 125.0 170.0 210.0 260.0
        goal = New-RandomEndpoint 10.0 55.0 20.0 70.0
        min_distance = 165.0
        obstacle_attempts = 450
        start_goal_attempts = 6500
        global_seed = 81016
        scene_notes = 'One side of the workspace remains fairly open while the destination region is structurally cluttered.'
        metric_signature = 'Asymmetric cell-density distribution, moderate area ratio, corridor quality depends on diagonal entry.'
    },
    [ordered]@{
        file = '17_corner_bias_medium.json'
        config_name = 'final18_corner_bias_medium'
        family = 'corner_bias_tall'
        difficulty = 'medium'
        summary = 'Tall rectangular scenes with a stronger corner cluster and more pronounced free-space gradient.'
        segment = 'Tall-workspace asymmetric clutter, medium corner concentration.'
        workspace = [ordered]@{ width = 180.0; height = 280.0 }
        count_distribution = New-DiscreteInt @(30, 40, 50) @(0.25, 0.45, 0.30)
        radius_distribution = New-NormalReal 4.3 1.1 2.0 6.4
        center_distribution = New-PointDistribution (New-NormalReal 45.0 40.0 -12.0 192.0) (New-NormalReal 60.0 52.0 -12.0 292.0)
        start = New-RandomEndpoint 125.0 170.0 210.0 260.0
        goal = New-RandomEndpoint 10.0 55.0 20.0 70.0
        min_distance = 185.0
        obstacle_attempts = 700
        start_goal_attempts = 8500
        global_seed = 81017
        scene_notes = 'The target-side cluster grows denser and makes final approach quality much more scene-dependent.'
        metric_signature = 'Higher cell-density CV, lower clearance near the clustered corner, larger route asymmetry.'
    },
    [ordered]@{
        file = '18_corner_bias_dense.json'
        config_name = 'final18_corner_bias_dense'
        family = 'corner_bias_tall'
        difficulty = 'high'
        summary = 'Tall rectangular scenes with a dense corner cluster that can turn the target region into a selective maze.'
        segment = 'Tall-workspace asymmetric clutter, high corner concentration.'
        workspace = [ordered]@{ width = 180.0; height = 280.0 }
        count_distribution = New-DiscreteInt @(42, 54, 68) @(0.25, 0.45, 0.30)
        radius_distribution = New-NormalReal 4.8 1.2 2.2 7.0
        center_distribution = New-PointDistribution (New-NormalReal 45.0 30.0 -10.0 190.0) (New-NormalReal 60.0 40.0 -10.0 290.0)
        start = New-RandomEndpoint 125.0 170.0 210.0 260.0
        goal = New-RandomEndpoint 10.0 55.0 20.0 70.0
        min_distance = 205.0
        obstacle_attempts = 950
        start_goal_attempts = 10000
        global_seed = 81018
        scene_notes = 'The diagonal approach remains long and the final region is often the dominant source of difficulty.'
        metric_signature = 'Strong asymmetry, reduced clearance near the goal-side cluster, higher runtime variance for detour-sensitive planners.'
    }
)

$manifestEntries = @()
foreach ($spec in $specs) {
    $config = [ordered]@{
        version = 1
        name = $spec.config_name
        instance_count = 1200
        global_seed = $spec.global_seed
        scene = [ordered]@{
            width = $spec.workspace.width
            height = $spec.workspace.height
        }
        obstacles = [ordered]@{
            count_distribution = $spec.count_distribution
            radius_distribution = $spec.radius_distribution
            center_distribution = $spec.center_distribution
            allow_overlap = $false
            require_intersection_with_scene = $true
            max_attempts_per_obstacle = $spec.obstacle_attempts
        }
        start_goal = [ordered]@{
            start = $spec.start
            goal = $spec.goal
            min_distance = $spec.min_distance
            max_attempts = $spec.start_goal_attempts
        }
        algorithms = New-Algorithms
        metrics = New-Metrics
        output = New-Output
    }

    $configPath = Join-Path $configDir $spec.file
    $config | ConvertTo-Json -Depth 30 | Set-Content -Path $configPath -Encoding utf8NoBOM

    $manifestEntries += [ordered]@{
        file = $spec.file
        config_name = $spec.config_name
        family = $spec.family
        difficulty = $spec.difficulty
        instance_count = 1200
        workspace = [ordered]@{
            width = $spec.workspace.width
            height = $spec.workspace.height
        }
        summary = $spec.summary
        segment = $spec.segment
        scene_notes = $spec.scene_notes
        metric_signature = $spec.metric_signature
        recommended_experiment_file = ("experiments/final_18_runs/" + ($spec.file -replace '\.json$', '_experiment.json'))
    }
}

$manifest = [ordered]@{
    study_id = 'final_18'
    study_title = 'Final 18-config diploma study'
    config_directory = 'configs/studies/final_18'
    experiment_output_directory = 'experiments/final_18_runs'
    config_count = $specs.Count
    instance_count_per_config = 1200
    total_scene_count = 1200 * $specs.Count
    notes = @(
        'All configs use the same algorithm and metrics settings so that scene generation parameters remain the main varying factor.',
        'Obstacle overlap is disabled in every config to keep visibility-graph and geometric metrics meaningful.',
        'The set spans compact, medium, large, wide, and tall workspaces together with uniform, central, band-shaped, and corner-biased obstacle layouts.'
    )
    configs = $manifestEntries
}
$manifest | ConvertTo-Json -Depth 30 | Set-Content -Path (Join-Path $configDir 'study_manifest.json') -Encoding utf8NoBOM

$readme = @()
$readme += '# Final 18-Config Study'
$readme += ''
$readme += 'This directory contains the final generator config set for the diploma-scale experiment campaign.'
$readme += ''
$readme += '- Config count: 18'
$readme += '- Scenes per config: 1200'
$readme += '- Total generated scenes: 21600'
$readme += '- Recommended experiment output folder: `experiments/final_18_runs/`'
$readme += ''
$readme += 'The whole set keeps algorithm parameters fixed and varies only scene-generation regimes so that downstream analysis can attribute changes in performance to scene structure rather than to tuning changes.'
$readme += ''
$readme += '## Coverage summary'
$readme += ''
$readme += '| File | Family | Difficulty | Typical scene character | Covered segment |'
$readme += '| --- | --- | --- | --- | --- |'
foreach ($spec in $specs) {
    $readme += "| ``$($spec.file)`` | ``$($spec.family)`` | ``$($spec.difficulty)`` | $($spec.summary) | $($spec.segment) |"
}
$readme += ''
$readme += '## Per-config notes'
$readme += ''
foreach ($spec in $specs) {
    $readme += "### ``$($spec.file)``"
    $readme += "- Family: ``$($spec.family)``"
    $readme += "- Difficulty: ``$($spec.difficulty)``"
    $readme += "- Workspace: ``$($spec.workspace.width)x$($spec.workspace.height)``"
    $readme += "- Typical scenes: $($spec.scene_notes)"
    $readme += "- Covered segment: $($spec.segment)"
    $readme += "- Expected metric tendency: $($spec.metric_signature)"
    $readme += "- Recommended saved experiment file: ``experiments/final_18_runs/$($spec.file -replace '\.json$', '_experiment.json')``"
    $readme += ''
}
$readme -join "`r`n" | Set-Content -Path (Join-Path $configDir 'README.md') -Encoding utf8NoBOM

$experimentReadme = @'
# Final 18 Study Experiments

Save generated `experiment.json` files for the final 18-config study here.

Recommended naming pattern:
- `01_uniform_compact_light_experiment.json`
- `02_uniform_compact_medium_experiment.json`
- ...
- `18_corner_bias_dense_experiment.json`

This keeps saved experiment files aligned with `configs/studies/final_18/study_manifest.json` for later aggregation in Python.
'@
$experimentReadme | Set-Content -Path (Join-Path $experimentDir 'README.md') -Encoding utf8NoBOM
