import torch
import torch.nn as nn


def head(width, neurons, out):
    layers, d = [], width
    for h in neurons:
        layers += [nn.Linear(d, h), nn.LeakyReLU(0.2)]
        d = h
    return nn.Sequential(*layers, nn.Linear(d, out))


class TimeConv(nn.Module):
    """Valid-padded 1D convolution over time, as unfold + matmul.

    Same arithmetic and parameter count as nn.Conv1d, but at these shapes Conv1d
    costs ~2 ms per call on CPU against ~0.01 ms for the equivalent matmul.
    Input and output stay (batch, time, channels) — no transposes.
    """

    def __init__(self, cin, cout, k):
        super().__init__()
        self.k = k
        self.lin = nn.Linear(cin * k, cout)

    def forward(self, x):
        return self.lin(x.unfold(1, self.k, 1).flatten(2))


class _Branch(nn.Module):
    """Conv stack over time, flattened into an MLP head."""

    def __init__(self, feature_dim, history, kernels, filters, neurons, out):
        super().__init__()
        conv, c, t = [], feature_dim, history
        for k, f in zip(kernels, filters):
            conv += [TimeConv(c, f, k), nn.LeakyReLU(0.2)]
            c, t = f, t - k + 1
        if t < 1:
            raise SystemExit(f"kernels {kernels} consume more than history={history}")
        self.conv = nn.Sequential(*conv)
        self.head = head(c * t, neurons, out)

    def forward(self, x):
        return self.head(self.conv(x).flatten(1))


class TCN(nn.Module):
    """Paper baseline: one independent conv stack per output triple."""

    def __init__(self, cfg, history, feature_dim):
        super().__init__()
        args = (feature_dim, history, cfg["kernels"], cfg["filters"], cfg["neurons"])
        self.two_heads = cfg["two_heads"]
        if self.two_heads:
            self.force = _Branch(*args, 3)
            self.torque = _Branch(*args, 3)
        else:
            self.both = _Branch(*args, 6)

    def forward(self, x):
        if self.two_heads:
            return torch.cat([self.force(x), self.torque(x)], dim=1)
        return self.both(x)
