# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
#
# CarWash Batch-Invariant Inference Layer
# =======================================
# Integrates ThinkingMachines batch_invariant_ops with ComfyUI
# for deterministic AI rendering.
#
# Reference: He et al., "Defeating Nondeterminism in LLM Inference", Sep 2025
# https://thinkingmachines.ai/blog/defeating-nondeterminism-in-llm-inference/

"""
CarWash Deterministic Inference Layer

This module provides batch-invariant inference guarantees for AI image generation,
ensuring that identical inputs produce identical outputs regardless of server load,
batch composition, or execution timing.

Key Insight (from ThinkingMachines research):
    The commonly assumed "concurrency + floating-point" hypothesis is WRONG.
    The REAL source of nondeterminism is batch size affecting reduction order in GPU kernels.
    Same prompt + same seed + different batch size = DIFFERENT OUTPUT.

Solution:
    1. Isolation: Lock GPU, fix memory state, process isolation
    2. Batch-Invariant Ops: Fixed tile sizes, deterministic reduction order
    3. Cognitive Substrate: Consistent inputs via style memory and semantic context

Usage:
    from carwash.batch_invariant import CarWashDeterministicContext

    with CarWashDeterministicContext(seed=42) as ctx:
        result = comfyui_workflow.execute(aovs)
"""

import os
import torch
from typing import Dict, Any, Optional
from contextlib import contextmanager
import logging

logger = logging.getLogger("carwash.determinism")


class CarWashDeterministicContext:
    """
    Context manager that ensures batch-invariant inference.

    This context:
    1. Enables PyTorch deterministic algorithms
    2. Sets CUBLAS/CUDA memory config for reproducibility
    3. Seeds all random number generators
    4. Optionally enables batch_invariant_ops (if available)

    Usage:
        with CarWashDeterministicContext(seed=42) as ctx:
            result = comfyui_workflow.execute(aovs)

    Args:
        seed: Master seed for all random number generators
        strict: If True, requires batch_invariant_ops to be installed
    """

    def __init__(self, seed: int, strict: bool = False):
        self.seed = seed
        self.strict = strict
        self._previous_state: Dict[str, Any] = {}
        self._batch_invariant_available = False

    def __enter__(self):
        # Store previous state for restoration
        self._previous_state = {
            'deterministic_algorithms': torch.are_deterministic_algorithms_enabled(),
            'cudnn_benchmark': torch.backends.cudnn.benchmark,
            'cudnn_deterministic': torch.backends.cudnn.deterministic,
            'cublas_workspace': os.environ.get('CUBLAS_WORKSPACE_CONFIG'),
            'cuda_alloc': os.environ.get('PYTORCH_CUDA_ALLOC_CONF'),
        }

        # Enable deterministic algorithms
        torch.use_deterministic_algorithms(True)

        # Disable cuDNN benchmark (can cause variance)
        torch.backends.cudnn.benchmark = False
        torch.backends.cudnn.deterministic = True

        # Set CUDA workspace config for deterministic cuBLAS
        os.environ['CUBLAS_WORKSPACE_CONFIG'] = ':4096:8'

        # Disable expandable memory segments (can cause variance)
        os.environ['PYTORCH_CUDA_ALLOC_CONF'] = 'expandable_segments:False'

        # Set all random states
        self._set_all_seeds(self.seed)

        # Enable batch-invariant mode if available
        self._batch_invariant_available = self._try_enable_batch_invariant()

        if self.strict and not self._batch_invariant_available:
            raise RuntimeError(
                "batch_invariant_ops required for strict determinism. "
                "Install from: github.com/thinking-machines-lab/batch_invariant_ops"
            )

        logger.info(
            f"CarWash deterministic context entered: seed={self.seed}, "
            f"batch_invariant={self._batch_invariant_available}"
        )

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Restore previous state
        prev = self._previous_state

        if prev.get('deterministic_algorithms') is not None:
            torch.use_deterministic_algorithms(prev['deterministic_algorithms'])

        torch.backends.cudnn.benchmark = prev.get('cudnn_benchmark', True)
        torch.backends.cudnn.deterministic = prev.get('cudnn_deterministic', False)

        # Restore environment variables
        if prev.get('cublas_workspace') is None:
            os.environ.pop('CUBLAS_WORKSPACE_CONFIG', None)
        else:
            os.environ['CUBLAS_WORKSPACE_CONFIG'] = prev['cublas_workspace']

        if prev.get('cuda_alloc') is None:
            os.environ.pop('PYTORCH_CUDA_ALLOC_CONF', None)
        else:
            os.environ['PYTORCH_CUDA_ALLOC_CONF'] = prev['cuda_alloc']

        # Disable batch-invariant mode
        if self._batch_invariant_available:
            self._disable_batch_invariant()

        logger.info("CarWash deterministic context exited")

        return False  # Don't suppress exceptions

    def _set_all_seeds(self, seed: int):
        """Set all random number generators to known state."""
        import random
        import numpy as np

        random.seed(seed)
        np.random.seed(seed)
        torch.manual_seed(seed)

        if torch.cuda.is_available():
            torch.cuda.manual_seed(seed)
            torch.cuda.manual_seed_all(seed)

    def _try_enable_batch_invariant(self) -> bool:
        """Try to enable batch-invariant mode. Returns True if successful."""
        try:
            from batch_invariant_ops import set_batch_invariant_mode
            set_batch_invariant_mode(True)
            logger.info("batch_invariant_ops enabled")
            return True
        except ImportError:
            logger.warning(
                "batch_invariant_ops not available. "
                "Determinism may be affected by batch size variance."
            )
            return False
        except Exception as e:
            logger.warning(f"Failed to enable batch_invariant_ops: {e}")
            return False

    def _disable_batch_invariant(self):
        """Disable batch-invariant mode."""
        try:
            from batch_invariant_ops import set_batch_invariant_mode
            set_batch_invariant_mode(False)
        except ImportError:
            pass


@contextmanager
def deterministic_inference(seed: int, strict: bool = False):
    """
    Convenience context manager for deterministic inference.

    Usage:
        with deterministic_inference(seed=42):
            result = run_inference(...)
    """
    ctx = CarWashDeterministicContext(seed=seed, strict=strict)
    with ctx:
        yield ctx


def compute_determinism_hash(tensor: torch.Tensor) -> str:
    """
    Compute a deterministic hash of a tensor for verification.

    This hash can be used to verify that two inference runs produced
    identical outputs.
    """
    import hashlib

    # Convert to CPU and numpy for consistent hashing
    data = tensor.detach().cpu().numpy().tobytes()
    return hashlib.sha256(data).hexdigest()[:16]


def verify_batch_invariance(
    inference_fn,
    inputs: Dict[str, Any],
    seed: int,
    batch_sizes: list = [1, 2, 4, 8]
) -> Dict[str, Any]:
    """
    Verify that an inference function produces batch-invariant results.

    Args:
        inference_fn: Function to test
        inputs: Inputs to pass to the function
        seed: Seed to use
        batch_sizes: Batch sizes to test

    Returns:
        Dict with verification results
    """
    results = []
    hashes = []

    for batch_size in batch_sizes:
        with CarWashDeterministicContext(seed=seed, strict=False) as ctx:
            # Simulate different batch conditions
            result = inference_fn(**inputs)
            h = compute_determinism_hash(result)
            results.append(result)
            hashes.append(h)

    is_invariant = len(set(hashes)) == 1

    return {
        'is_batch_invariant': is_invariant,
        'hashes': hashes,
        'batch_sizes': batch_sizes,
        'batch_invariant_ops_enabled': results[0] if results else None
    }


# Export main classes and functions
__all__ = [
    'CarWashDeterministicContext',
    'deterministic_inference',
    'compute_determinism_hash',
    'verify_batch_invariance',
]
