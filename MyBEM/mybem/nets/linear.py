import torch.nn as nn


class Linear(nn.Module):
    """Ridge regression on the flattened window; the ridge term is optim.l2.

    two_heads is meaningless here: two independent affine maps over the same
    input are one affine map.
    """

    def __init__(self, cfg, history, feature_dim):
        super().__init__()
        self.lin = nn.Linear(history * feature_dim, 6)

    def forward(self, x):
        return self.lin(x.flatten(1))
