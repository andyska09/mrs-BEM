"""Network registry. One file plus one line adds an architecture.

Contract: (batch, history, features) -> (batch, 6), residual force then torque.
"""

from .mlp import MLP
from .tcn import TCN

ARCHITECTURES = {"tcn": TCN, "mlp": MLP}


def create_net(cfg, history, feature_dim):
    if cfg["arch"] not in ARCHITECTURES:
        raise SystemExit(f"unknown arch '{cfg['arch']}', have {sorted(ARCHITECTURES)}")
    return ARCHITECTURES[cfg["arch"]](cfg, history, feature_dim)
