# Final 18-Config Study

> Superseded by the broader curated study in configs/studies/final_25/. Use that set for the main final experiment campaign when you need stronger scene diversity.

This directory contains the final generator config set for the diploma-scale experiment campaign.

- Config count: 18
- Scenes per config: 1200
- Total generated scenes: 21600
- Recommended experiment output folder: `experiments/final_18_runs/`

The whole set keeps algorithm parameters fixed and varies only scene-generation regimes so that downstream analysis can attribute changes in performance to scene structure rather than to tuning changes.

## Coverage summary

| File | Family | Difficulty | Typical scene character | Covered segment |
| --- | --- | --- | --- | --- |
| `01_uniform_compact_light.json` | `uniform_compact` | `light` | Compact square scenes with a few small uniformly distributed obstacles and wide free-space corridors. | Small-workspace, low-density isotropic random clutter. |
| `02_uniform_compact_medium.json` | `uniform_compact` | `medium` | Compact square scenes with balanced small-to-medium random clutter and frequent local detours. | Small-workspace, medium-density isotropic clutter. |
| `03_uniform_compact_dense.json` | `uniform_compact` | `high` | Compact square scenes with dense uniform clutter where many routes are narrow and highly local. | Small-workspace, high-density isotropic clutter. |
| `04_uniform_large_disks_light.json` | `uniform_large_disks` | `light` | Medium square scenes with a few large dominant disks creating macro detours and broad pockets of free space. | Large-obstacle random fields, light density. |
| `05_uniform_large_disks_medium.json` | `uniform_large_disks` | `medium` | Medium square scenes with more large disks and stronger route-deflecting geometry. | Large-obstacle random fields, medium density. |
| `06_uniform_large_disks_dense.json` | `uniform_large_disks` | `high` | Medium square scenes with many large disks, producing long detours and frequent line-of-sight disruption. | Large-obstacle random fields, high density. |
| `07_uniform_mixed_scale_light.json` | `uniform_mixed_scale_large` | `light` | Large square scenes with heterogeneous obstacle sizes and long-range routes through mostly open space. | Large-workspace, mixed-scale random obstacles, light density. |
| `08_uniform_mixed_scale_medium.json` | `uniform_mixed_scale_large` | `medium` | Large square scenes with more heterogeneous obstacles and frequent medium-range rerouting. | Large-workspace, mixed-scale random obstacles, medium density. |
| `09_uniform_mixed_scale_dense.json` | `uniform_mixed_scale_large` | `high` | Large square scenes with dense multi-scale clutter and many competing route structures. | Large-workspace, mixed-scale random obstacles, high density. |
| `10_center_cluster_light.json` | `center_cluster` | `light` | Square scenes with a soft central concentration that encourages peripheral bypasses. | Center-biased clutter, light concentration. |
| `11_center_cluster_medium.json` | `center_cluster` | `medium` | Square scenes with a stronger central obstacle cloud and more pronounced ring-like free regions. | Center-biased clutter, medium concentration. |
| `12_center_cluster_dense.json` | `center_cluster` | `high` | Square scenes with a dense central mass that often creates ring bypasses and selective narrow crossings. | Center-biased clutter, high concentration. |
| `13_vertical_band_light.json` | `vertical_band` | `light` | Wide rectangular scenes with a loose vertical clutter belt between left and right free regions. | Wide-workspace cross-barrier scenes, light band density. |
| `14_vertical_band_medium.json` | `vertical_band` | `medium` | Wide rectangular scenes with a denser vertical belt that starts to act like a soft barrier. | Wide-workspace cross-barrier scenes, medium band density. |
| `15_vertical_band_dense.json` | `vertical_band` | `high` | Wide rectangular scenes with a dense vertical belt that frequently creates bottleneck-like crossings. | Wide-workspace cross-barrier scenes, high band density. |
| `16_corner_bias_light.json` | `corner_bias_tall` | `light` | Tall rectangular scenes with one obstacle-rich corner and a long diagonal approach from open space. | Tall-workspace asymmetric clutter, light corner concentration. |
| `17_corner_bias_medium.json` | `corner_bias_tall` | `medium` | Tall rectangular scenes with a stronger corner cluster and more pronounced free-space gradient. | Tall-workspace asymmetric clutter, medium corner concentration. |
| `18_corner_bias_dense.json` | `corner_bias_tall` | `high` | Tall rectangular scenes with a dense corner cluster that can turn the target region into a selective maze. | Tall-workspace asymmetric clutter, high corner concentration. |

## Per-config notes

### `01_uniform_compact_light.json`
- Family: `uniform_compact`
- Difficulty: `light`
- Workspace: `100x100`
- Typical scenes: Mostly open compact layouts; direct paths often break but broad alternate corridors usually remain.
- Covered segment: Small-workspace, low-density isotropic random clutter.
- Expected metric tendency: Low obstacle area ratio, high clearance, high largest free component ratio.
- Recommended saved experiment file: `experiments/final_18_runs/01_uniform_compact_light_experiment.json`

### `02_uniform_compact_medium.json`
- Family: `uniform_compact`
- Difficulty: `medium`
- Workspace: `100x100`
- Typical scenes: Dense local interactions in a compact workspace; straight-line solutions are less frequent.
- Covered segment: Small-workspace, medium-density isotropic clutter.
- Expected metric tendency: Moderate obstacle area ratio, reduced clearance, still mostly connected free space.
- Recommended saved experiment file: `experiments/final_18_runs/02_uniform_compact_medium_experiment.json`

### `03_uniform_compact_dense.json`
- Family: `uniform_compact`
- Difficulty: `high`
- Workspace: `100x100`
- Typical scenes: Tight compact scenes with many short detours and frequent narrow micro-passages.
- Covered segment: Small-workspace, high-density isotropic clutter.
- Expected metric tendency: Higher obstacle area ratio, lower clearance, stronger spread in local cell densities.
- Recommended saved experiment file: `experiments/final_18_runs/03_uniform_compact_dense_experiment.json`

### `04_uniform_large_disks_light.json`
- Family: `uniform_large_disks`
- Difficulty: `light`
- Workspace: `150x150`
- Typical scenes: Fewer but visually dominant obstacles; path quality depends on choosing the correct side around large disks.
- Covered segment: Large-obstacle random fields, light density.
- Expected metric tendency: Moderate obstacle area ratio with comparatively larger clearance variance.
- Recommended saved experiment file: `experiments/final_18_runs/04_uniform_large_disks_light_experiment.json`

### `05_uniform_large_disks_medium.json`
- Family: `uniform_large_disks`
- Difficulty: `medium`
- Workspace: `150x150`
- Typical scenes: Routes are shaped by a smaller number of strong blockers rather than by many tiny collisions.
- Covered segment: Large-obstacle random fields, medium density.
- Expected metric tendency: Higher area ratio than light variant, still moderate cell-density CV compared with clustered families.
- Recommended saved experiment file: `experiments/final_18_runs/05_uniform_large_disks_medium_experiment.json`

### `06_uniform_large_disks_dense.json`
- Family: `uniform_large_disks`
- Difficulty: `high`
- Workspace: `150x150`
- Typical scenes: Macro-detour scenes where one bad side choice can lengthen the route substantially.
- Covered segment: Large-obstacle random fields, high density.
- Expected metric tendency: High obstacle area ratio, noticeably lower visibility and lower effective clearance.
- Recommended saved experiment file: `experiments/final_18_runs/06_uniform_large_disks_dense_experiment.json`

### `07_uniform_mixed_scale_light.json`
- Family: `uniform_mixed_scale_large`
- Difficulty: `light`
- Workspace: `260x260`
- Typical scenes: Long planning horizons with both tiny and large obstacles; path smoothness starts to matter.
- Covered segment: Large-workspace, mixed-scale random obstacles, light density.
- Expected metric tendency: Low-to-moderate area ratio, large component ratio near 1, broad radii CV.
- Recommended saved experiment file: `experiments/final_18_runs/07_uniform_mixed_scale_light_experiment.json`

### `08_uniform_mixed_scale_medium.json`
- Family: `uniform_mixed_scale_large`
- Difficulty: `medium`
- Workspace: `260x260`
- Typical scenes: Large scenes where obstacle heterogeneity creates both global and local difficulty at once.
- Covered segment: Large-workspace, mixed-scale random obstacles, medium density.
- Expected metric tendency: Moderate area ratio, high radii CV, moderate cell-density CV.
- Recommended saved experiment file: `experiments/final_18_runs/08_uniform_mixed_scale_medium_experiment.json`

### `09_uniform_mixed_scale_dense.json`
- Family: `uniform_mixed_scale_large`
- Difficulty: `high`
- Workspace: `260x260`
- Typical scenes: Long routes in large workspaces with many choices, dead-ends and detour scales mixed together.
- Covered segment: Large-workspace, mixed-scale random obstacles, high density.
- Expected metric tendency: Moderate-to-high area ratio, strong radii CV, visible drop in clearance and connectivity margins.
- Recommended saved experiment file: `experiments/final_18_runs/09_uniform_mixed_scale_dense_experiment.json`

### `10_center_cluster_light.json`
- Family: `center_cluster`
- Difficulty: `light`
- Workspace: `180x180`
- Typical scenes: Central congestion is visible but still porous; many scenes admit both center and edge routes.
- Covered segment: Center-biased clutter, light concentration.
- Expected metric tendency: Moderate cell-density CV, reduced central clearance, overall connectivity still robust.
- Recommended saved experiment file: `experiments/final_18_runs/10_center_cluster_light_experiment.json`

### `11_center_cluster_medium.json`
- Family: `center_cluster`
- Difficulty: `medium`
- Workspace: `180x180`
- Typical scenes: A denser core pushes good routes toward the outskirts and amplifies path-length differences.
- Covered segment: Center-biased clutter, medium concentration.
- Expected metric tendency: Higher cell-density CV, lower central clearance, strong boundary-vs-center contrast.
- Recommended saved experiment file: `experiments/final_18_runs/11_center_cluster_medium_experiment.json`

### `12_center_cluster_dense.json`
- Family: `center_cluster`
- Difficulty: `high`
- Workspace: `180x180`
- Typical scenes: The center becomes the dominant structural feature, often forcing wide arc-like bypasses.
- Covered segment: Center-biased clutter, high concentration.
- Expected metric tendency: High cell-density CV, low center clearance, reduced straight-line feasibility.
- Recommended saved experiment file: `experiments/final_18_runs/12_center_cluster_dense_experiment.json`

### `13_vertical_band_light.json`
- Family: `vertical_band`
- Difficulty: `light`
- Workspace: `280x180`
- Typical scenes: Crossing from left to right is usually possible through several openings inside a central belt.
- Covered segment: Wide-workspace cross-barrier scenes, light band density.
- Expected metric tendency: Moderate boundary density, elevated cell-density CV along the x-axis, clearance drops near the center strip.
- Recommended saved experiment file: `experiments/final_18_runs/13_vertical_band_light_experiment.json`

### `14_vertical_band_medium.json`
- Family: `vertical_band`
- Difficulty: `medium`
- Workspace: `280x180`
- Typical scenes: The central strip often leaves a few good crossings whose quality varies strongly from scene to scene.
- Covered segment: Wide-workspace cross-barrier scenes, medium band density.
- Expected metric tendency: Higher local density anisotropy, lower central clearance, stronger path-length spread.
- Recommended saved experiment file: `experiments/final_18_runs/14_vertical_band_medium_experiment.json`

### `15_vertical_band_dense.json`
- Family: `vertical_band`
- Difficulty: `high`
- Workspace: `280x180`
- Typical scenes: These are the most barrier-like scenes in the set; a small number of passages may dominate route quality.
- Covered segment: Wide-workspace cross-barrier scenes, high band density.
- Expected metric tendency: Low clearance in the belt, high anisotropy, larger variance in success and runtime across algorithms.
- Recommended saved experiment file: `experiments/final_18_runs/15_vertical_band_dense_experiment.json`

### `16_corner_bias_light.json`
- Family: `corner_bias_tall`
- Difficulty: `light`
- Workspace: `180x280`
- Typical scenes: One side of the workspace remains fairly open while the destination region is structurally cluttered.
- Covered segment: Tall-workspace asymmetric clutter, light corner concentration.
- Expected metric tendency: Asymmetric cell-density distribution, moderate area ratio, corridor quality depends on diagonal entry.
- Recommended saved experiment file: `experiments/final_18_runs/16_corner_bias_light_experiment.json`

### `17_corner_bias_medium.json`
- Family: `corner_bias_tall`
- Difficulty: `medium`
- Workspace: `180x280`
- Typical scenes: The target-side cluster grows denser and makes final approach quality much more scene-dependent.
- Covered segment: Tall-workspace asymmetric clutter, medium corner concentration.
- Expected metric tendency: Higher cell-density CV, lower clearance near the clustered corner, larger route asymmetry.
- Recommended saved experiment file: `experiments/final_18_runs/17_corner_bias_medium_experiment.json`

### `18_corner_bias_dense.json`
- Family: `corner_bias_tall`
- Difficulty: `high`
- Workspace: `180x280`
- Typical scenes: The diagonal approach remains long and the final region is often the dominant source of difficulty.
- Covered segment: Tall-workspace asymmetric clutter, high corner concentration.
- Expected metric tendency: Strong asymmetry, reduced clearance near the goal-side cluster, higher runtime variance for detour-sensitive planners.
- Recommended saved experiment file: `experiments/final_18_runs/18_corner_bias_dense_experiment.json`


