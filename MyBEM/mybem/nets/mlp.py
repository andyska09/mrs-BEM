import torch
import torch.nn as nn

from .tcn import head


class MLP(nn.Module):
    def __init__(self, cfg, history, feature_dim):
        super().__init__()
        width = history * feature_dim
        self.two_heads = cfg["two_heads"]
        if self.two_heads:
            self.force = head(width, cfg["neurons"], 3)
            self.torque = head(width, cfg["neurons"], 3)
        else:
            self.both = head(width, cfg["neurons"], 6)

    def forward(self, x):
        x = x.flatten(1)
        if self.two_heads:
            return torch.cat([self.force(x), self.torque(x)], dim=1)
        return self.both(x)
