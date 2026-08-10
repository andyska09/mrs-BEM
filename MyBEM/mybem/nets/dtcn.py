import torch
import torch.nn as nn
import torch.nn.functional as F

from .tcn import head


class DilatedConv(nn.Module):
    """Causal dilated 1D convolution over time, as pad + unfold + matmul."""

    def __init__(self, cin, cout, k, d):
        super().__init__()
        self.pad, self.span, self.d = (k - 1) * d, (k - 1) * d + 1, d
        self.lin = nn.Linear(cin * k, cout)

    def forward(self, x):
        x = F.pad(x, (0, 0, self.pad, 0))
        return self.lin(x.unfold(1, self.span, 1)[..., ::self.d].flatten(2))


class _Block(nn.Module):
    def __init__(self, cin, cout, k, d):
        super().__init__()
        self.conv = nn.Sequential(DilatedConv(cin, cout, k, d), nn.LeakyReLU(0.2),
                                  DilatedConv(cout, cout, k, d), nn.LeakyReLU(0.2))
        self.res = nn.Linear(cin, cout) if cin != cout else nn.Identity()

    def forward(self, x):
        return self.conv(x) + self.res(x)


class _Branch(nn.Module):
    def __init__(self, feature_dim, channels, dilations, kernel, neurons, out):
        super().__init__()
        c, stack = feature_dim, []
        for d in dilations:
            stack.append(_Block(c, channels, kernel, d))
            c = channels
        self.stack = nn.Sequential(*stack)
        self.head = head(c, neurons, out)

    def forward(self, x):
        return self.head(self.stack(x)[:, -1])


class DTCN(nn.Module):
    """Dilated residual conv stack; the last timestep feeds the head."""

    def __init__(self, cfg, history, feature_dim):
        super().__init__()
        dilations, kernel = cfg["dilations"], cfg["kernel"]
        span = 1 + 2 * (kernel - 1) * sum(dilations)
        if span < history:
            raise SystemExit(f"receptive field {span} < history {history}")
        args = (feature_dim, cfg["channels"], dilations, kernel, cfg["neurons"])
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
