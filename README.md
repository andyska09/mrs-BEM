## MyBEM

[MyBEM/](MyBEM/) is the active rebuild of the NeuroBEM pipeline — a clean-break
reimplementation as independent, composable stages. Spec: [MyBEM/DESIGN.md](MyBEM/DESIGN.md).

- **C++** ([MyBEM/cpp/](MyBEM/cpp/)) — base aerodynamic model as additive components
  (`bem`, `quadratic`, `body_drag`, `motor_reaction`, `polyfit`, `none`) assembled from a
  model YAML. Two binaries: `mybem-apply` (predict force/torque over
  flight segments) and `mybem-tune` (CMA-ES parameter identification).
- **Python** ([MyBEM/mybem/](MyBEM/mybem/), PyTorch) — residual network on top of the base
  model: `train`, `eval`, `sweep`, `report`, plus `polyfit` for the gray-box identification.

`NeuroBEM/` stays frozen as the paper reference.

### Commands

Build (needs GSL, Eigen, OpenMP):
```
cmake -S MyBEM/cpp -B MyBEM/cpp/build && cmake --build MyBEM/cpp/build
```
Predict force/torque for every segment:
```
MyBEM/cpp/build/mybem-apply MyBEM/configs/models/bem_default.yaml data/processed_data MyBEM/store/preds
```
Fit base-model params with CMA-ES:
```
MyBEM/cpp/build/mybem-tune MODEL.yaml DATA.csv --drone configs/drones/paper_quad.yaml --free all --loss both --out RUNDIR
```
Train and evaluate the residual net (from `MyBEM/`):
```
conda run -n mybem python -m mybem.train configs/experiments/tcn_baseline.yaml
```
```
conda run -n mybem python -m mybem.eval tcn_baseline --on test
```

## CMA-ES analysis

Cross-run comparison of every CMA-ES re-identification fit lives in
[NeuroBEM/CMAES-results-analysis/](NeuroBEM/CMAES-results-analysis/)
(`results_analysis.ipynb` — coefficient table, metrics, convergence plots).
