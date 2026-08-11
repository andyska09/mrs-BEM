"""Every filesystem location the package uses."""

from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

CONFIGS = ROOT / "configs"
MODELS = CONFIGS / "models"
DRONES = CONFIGS / "drones"
SPLITS = CONFIGS / "splits"
NET_CONFIGS = CONFIGS / "nets"
SWEEPS = CONFIGS / "sweeps"

DATA = ROOT.parent / "data" / "processed_data"

STORE = ROOT / "store"
PREDS = STORE / "preds"
NETS = STORE / "nets"
POLYFIT = STORE / "polyfit"