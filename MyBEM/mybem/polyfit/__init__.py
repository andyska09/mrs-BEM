"""Vehicle-level polynomial gray-box model (Sun, de Visser & Chu, JoA 56(2) 2019).

Selection runs on a cross-product matrix rather than the candidate columns, so
the full dataset is used without ever materialising an N x 895 design matrix.
"""

from .fit import bin_edges, identify, predict, score
from .physics import Geometry
from .terms import AXES

__all__ = ["AXES", "Geometry", "bin_edges", "identify", "predict", "score"]
