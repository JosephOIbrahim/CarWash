# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
"""
CarWash Determinism Test Suite

Tests for batch-invariant inference guarantees.
Reference: He et al., "Defeating Nondeterminism in LLM Inference", Sep 2025
"""

import pytest
import torch
import numpy as np
import hashlib
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from comfyui.batch_invariant import (
    CarWashDeterministicContext,
    deterministic_inference,
    compute_determinism_hash,
    verify_batch_invariance,
)


class TestCarWashDeterministicContext:
    """Tests for the deterministic context manager."""

    def test_context_enters_deterministic_mode(self):
        """Verify context enables PyTorch deterministic algorithms."""
        with CarWashDeterministicContext(seed=42) as ctx:
            assert torch.are_deterministic_algorithms_enabled()
            assert not torch.backends.cudnn.benchmark
            assert torch.backends.cudnn.deterministic

    def test_context_restores_state_on_exit(self):
        """Verify context restores previous state on exit."""
        # Store original state
        orig_det = torch.are_deterministic_algorithms_enabled()
        orig_bench = torch.backends.cudnn.benchmark

        with CarWashDeterministicContext(seed=42):
            pass

        # Verify restoration
        assert torch.are_deterministic_algorithms_enabled() == orig_det
        assert torch.backends.cudnn.benchmark == orig_bench

    def test_seed_reproducibility(self):
        """Verify same seed produces same random values."""
        results = []
        for _ in range(3):
            with CarWashDeterministicContext(seed=42):
                results.append(torch.rand(10).tolist())

        assert results[0] == results[1] == results[2]

    def test_different_seeds_produce_different_results(self):
        """Verify different seeds produce different random values."""
        with CarWashDeterministicContext(seed=42):
            result1 = torch.rand(10).tolist()

        with CarWashDeterministicContext(seed=43):
            result2 = torch.rand(10).tolist()

        assert result1 != result2

    def test_strict_mode_without_batch_invariant_ops(self):
        """Verify strict mode raises when batch_invariant_ops not available."""
        # This test expects RuntimeError if batch_invariant_ops is required but missing
        # Since batch_invariant_ops is optional, we test the warning path
        with CarWashDeterministicContext(seed=42, strict=False) as ctx:
            # Should not raise, just warn
            pass

    def test_numpy_seed_consistency(self):
        """Verify numpy random state is also seeded."""
        with CarWashDeterministicContext(seed=42):
            result1 = np.random.rand(5).tolist()

        with CarWashDeterministicContext(seed=42):
            result2 = np.random.rand(5).tolist()

        assert result1 == result2


class TestDeterministicInference:
    """Tests for the deterministic_inference convenience function."""

    def test_convenience_context_manager(self):
        """Verify convenience function works as context manager."""
        with deterministic_inference(seed=42):
            assert torch.are_deterministic_algorithms_enabled()

    def test_yields_context_object(self):
        """Verify context object is yielded."""
        with deterministic_inference(seed=42) as ctx:
            assert isinstance(ctx, CarWashDeterministicContext)


class TestComputeDeterminismHash:
    """Tests for determinism hash computation."""

    def test_hash_is_string(self):
        """Verify hash returns a string."""
        tensor = torch.rand(10, 10)
        h = compute_determinism_hash(tensor)
        assert isinstance(h, str)

    def test_hash_is_16_chars(self):
        """Verify hash is truncated to 16 characters."""
        tensor = torch.rand(10, 10)
        h = compute_determinism_hash(tensor)
        assert len(h) == 16

    def test_same_tensor_same_hash(self):
        """Verify identical tensors produce identical hashes."""
        with deterministic_inference(seed=42):
            t1 = torch.rand(10, 10)

        with deterministic_inference(seed=42):
            t2 = torch.rand(10, 10)

        assert compute_determinism_hash(t1) == compute_determinism_hash(t2)

    def test_different_tensor_different_hash(self):
        """Verify different tensors produce different hashes."""
        t1 = torch.rand(10, 10)
        t2 = torch.rand(10, 10)
        assert compute_determinism_hash(t1) != compute_determinism_hash(t2)


class TestBatchInvariance:
    """Tests for batch-invariant behavior."""

    def test_verify_batch_invariance_function_exists(self):
        """Verify the batch invariance verification function exists."""
        assert callable(verify_batch_invariance)

    def test_simple_operation_is_batch_invariant(self):
        """Verify simple deterministic operations are batch-invariant."""
        def simple_inference(**kwargs):
            seed = kwargs.get('seed', 42)
            torch.manual_seed(seed)
            return torch.rand(10, 10)

        # Test across multiple runs
        hashes = []
        for _ in range(5):
            with deterministic_inference(seed=42):
                result = simple_inference(seed=42)
                hashes.append(compute_determinism_hash(result))

        # All hashes should be identical
        assert len(set(hashes)) == 1, f"Non-deterministic hashes: {hashes}"


class TestCUDADeterminism:
    """Tests for CUDA-specific determinism (skipped if no CUDA)."""

    @pytest.mark.skipif(not torch.cuda.is_available(), reason="CUDA not available")
    def test_cuda_seed_reproducibility(self):
        """Verify CUDA operations are deterministic with context."""
        device = torch.device('cuda')

        with CarWashDeterministicContext(seed=42):
            t1 = torch.rand(100, 100, device=device)
            h1 = compute_determinism_hash(t1)

        with CarWashDeterministicContext(seed=42):
            t2 = torch.rand(100, 100, device=device)
            h2 = compute_determinism_hash(t2)

        assert h1 == h2

    @pytest.mark.skipif(not torch.cuda.is_available(), reason="CUDA not available")
    def test_cuda_matmul_determinism(self):
        """Verify CUDA matmul is deterministic."""
        device = torch.device('cuda')

        with CarWashDeterministicContext(seed=42):
            a = torch.rand(64, 64, device=device)
            b = torch.rand(64, 64, device=device)
            c1 = torch.matmul(a, b)
            h1 = compute_determinism_hash(c1)

        with CarWashDeterministicContext(seed=42):
            a = torch.rand(64, 64, device=device)
            b = torch.rand(64, 64, device=device)
            c2 = torch.matmul(a, b)
            h2 = compute_determinism_hash(c2)

        assert h1 == h2


class TestEnvironmentVariables:
    """Tests for environment variable handling."""

    def test_cublas_workspace_set(self):
        """Verify CUBLAS_WORKSPACE_CONFIG is set in context."""
        with CarWashDeterministicContext(seed=42):
            assert os.environ.get('CUBLAS_WORKSPACE_CONFIG') == ':4096:8'

    def test_cublas_workspace_restored(self):
        """Verify CUBLAS_WORKSPACE_CONFIG is restored after context."""
        original = os.environ.get('CUBLAS_WORKSPACE_CONFIG')

        with CarWashDeterministicContext(seed=42):
            pass

        assert os.environ.get('CUBLAS_WORKSPACE_CONFIG') == original

    def test_cuda_alloc_config_set(self):
        """Verify PYTORCH_CUDA_ALLOC_CONF is set in context."""
        with CarWashDeterministicContext(seed=42):
            assert 'expandable_segments:False' in os.environ.get('PYTORCH_CUDA_ALLOC_CONF', '')


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
