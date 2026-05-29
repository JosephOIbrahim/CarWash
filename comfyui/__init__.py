# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
"""
CarWash ComfyUI Integration

This package provides:
- batch_invariant: Deterministic inference context managers
- custom_nodes: ComfyUI node implementations
- workflows: Pre-built workflow templates
"""

from . import batch_invariant
from . import custom_nodes

__version__ = "1.0.0"
__all__ = ['batch_invariant', 'custom_nodes']
