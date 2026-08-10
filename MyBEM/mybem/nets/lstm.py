import torch
import torch.nn as nn

from .tcn import head


class _Branch(nn.Module):
    def __init__(self, feature_dim, hidden, layers, neurons, out):
        super().__init__()
        self.rnn = nn.LSTM(feature_dim, hidden, layers, batch_first=True)
        self.head = head(hidden, neurons, out)

    def forward(self, x):
        return self.head(self.rnn(x)[0][:, -1])


class LSTM(nn.Module):
    def __init__(self, cfg, history, feature_dim):
        super().__init__()
        args = (feature_dim, cfg["hidden"], cfg["layers"], cfg["neurons"])
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
