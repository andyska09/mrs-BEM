# Hybrid aerodynamic modeling for agile multirotor flight

Reproduction of **NeuroBEM** (Bauersfeld, Kaufmann, Föhn, Sun, Scaramuzza — RSS 2021)
and a clean-break rebuild of its pipeline, aimed at closed-loop evaluation inside the
**Agilicious** simulator.

The whole framework computes one equation:

```
f = f_prop + f_res        τ = τ_prop + τ_res
```

A **base rotor model** (C++, first principles) predicts `f_prop`/`τ_prop`; a **neural
network** (PyTorch) predicts the residual the base model misses.

Flight data and the original framework: **https://download.ifi.uzh.ch/rpg/NeuroBEM/**

## Layout

| path | what |
|---|---|
| [MyBEM/](MyBEM/) | **the active rebuild** — base model (C++) + residual net (PyTorch) |
| [NeuroBEM/](NeuroBEM/) | the frozen paper reference and cross-check; partial copy of the RPG release |
| [research/](research/) | papers, notes, and experiment write-ups |
| `data/` | flight data (gitignored) — `data/processed_data/` holds the 247 `merged_*_seg_*.csv` |

---

## MyBEM

A reimplementation of the pipeline as independent, composable stages with explicit
artifacts. Spec: [MyBEM/DESIGN.md](MyBEM/DESIGN.md).

**C++** ([MyBEM/cpp/](MyBEM/cpp/)) — the base aerodynamic model, assembled from a model
YAML as a list of additive components (`bem`, `quadratic`, `polyfit`, `body_drag`,
`motor_reaction`, `none`). Swapping the base model is a config line, not a rebuild.
Two binaries: `mybem-apply` (predict force/torque over flight segments) and
`mybem-tune` (CMA-ES parameter identification).

**Python** ([MyBEM/mybem/](MyBEM/mybem/)) — the residual network on top of a base
model: `train`, `eval`, `sweep`, `report`, plus `polyfit` for the gray-box
identification. Architectures live in [mybem/nets/](MyBEM/mybem/nets/) (`tcn`, `dtcn`,
`lstm`, `mlp`, `linear`); one file plus one line adds another.

Artifacts land under `MyBEM/store/` (gitignored), named `<name>@<hash>` where the hash
is the digest of the fully-resolved config — so a run always records what produced it.

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
Identify the PolyFit gray-box model (from `MyBEM/`):
```
conda run -n mybem python -m mybem.polyfit --name polyfit_paper --bins 3
```
Train the residual net:
```
conda run -n mybem python -m mybem.train configs/nets/tcn_baseline.yaml
```
Score any set of models in paper Table II format:
```
conda run -n mybem python -m mybem.eval bem_default arch_dtcn_32@707e43 --on test
```
(`ls MyBEM/store/nets` for the `<name>@<hash>` to pass; base models take the bare name.)
Cross-run table over everything trained so far:
```
conda run -n mybem python -m mybem.report --on test
```
Long runs go to the MetaCentrum PBS cluster — see
[MyBEM/scripts/metacentrum/metacentrum.md](MyBEM/scripts/metacentrum/metacentrum.md).

---

## Research

[research/sources/](research/sources/) is inputs only — the papers as PDF plus a
grep-able text transcription ([NeuroBEM](research/sources/papers/RSS21_Bauersfeld.md),
[Neural-Fly](research/sources/papers/scirobotics.abm6597.md),
[the PolyFit gray-box model](research/sources/papers/Quadrotor_Gray_box_Model_Identification_from_High_Speed_Flight_Data.md),
[WaveNet](research/sources/papers/WaveNet_1609.03499.md)) — and
[notes/](research/sources/notes/) with loose notes on the model, its assumptions, and
the data.

[research/reports/](research/reports/) is where findings are written up:

| report | question | status |
|---|---|---|
| [CMAES-identification.md](research/reports/CMAES-identification.md) | Does re-identifying the BEM parameters with CMA-ES beat the paper's published ones? | run |
| [Polyfit-comparison.md](research/reports/Polyfit-comparison.md) | Does PolyFit + NN outperform BEM + NN? Table II never tests it. | run |
| [Feature-ablation.md](research/reports/Feature-ablation.md) | Which of the paper's three input groups actually carries the residual? | run, 7 arms × 3 seeds |
| [Architectures.md](research/reports/Architectures.md) | Does the residual architecture matter, or does any net of the same capacity land in the same place? | run |
| [Generalization.md](research/reports/Generalization.md) | The paper's reduced-training-set experiment. | placeholder |

Cross-run comparison of the CMA-ES fits done on the old stack lives in
[NeuroBEM/CMAES-results-analysis/](NeuroBEM/CMAES-results-analysis/)
(`results_analysis.ipynb` — coefficient table, metrics, convergence plots).
