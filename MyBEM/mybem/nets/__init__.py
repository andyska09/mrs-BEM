"""Network registry. One file plus one line adds an architecture.

Contract: (batch, history, features) -> (batch, 6), residual force then torque.
"""

from .dtcn import DTCN
from .linear import Linear
from .lstm import LSTM
from .mlp import MLP
from .tcn import TCN

ARCHITECTURES = {"tcn": TCN, "mlp": MLP, "dtcn": DTCN, "lstm": LSTM, "linear": Linear}


def create_net(cfg, history, feature_dim):
    if cfg["arch"] not in ARCHITECTURES:
        raise SystemExit(f"unknown arch '{cfg['arch']}', have {sorted(ARCHITECTURES)}")
    return ARCHITECTURES[cfg["arch"]](cfg, history, feature_dim)
