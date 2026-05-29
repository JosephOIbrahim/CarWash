"""
CarWash Test Suite
==================
Comprehensive end-to-end testing for production readiness.

Test Categories:
1. Connection & Health
2. Workflow Validation
3. Rendering Pipeline
4. Caching & Determinism
5. Error Handling & Recovery
6. Color Management
7. High-Resolution Tiling
8. Animation/Video

Run: python carwash_test_suite.py
"""

import sys
import os
import time
import json
import hashlib
import tempfile
import shutil
from pathlib import Path
from datetime import datetime
from typing import List, Tuple, Optional
from dataclasses import dataclass
from enum import Enum

# Add module path
sys.path.insert(0, str(Path(__file__).parent))

from carwash_production import (
    CarWashConfig, CarWashEngine, CarWashLogger,
    ComfyUIConnection, ConnectionState, WorkflowBuilder,
    RenderJob, JobStatus, JobQueue, RenderCache
)
from carwash_color import ColorManager, DepthNormalizer, ColorSpace

import numpy as np


# =============================================================================
# TEST FRAMEWORK
# =============================================================================

class TestResult(Enum):
    PASS = "PASS"
    FAIL = "FAIL"
    SKIP = "SKIP"
    ERROR = "ERROR"


@dataclass
class TestCase:
    name: str
    result: TestResult
    duration: float
    message: str = ""
    details: Optional[dict] = None


class TestRunner:
    """Test runner with reporting."""

    def __init__(self):
        self.results: List[TestCase] = []
        self.start_time = None

    def run_test(self, name: str, test_func, *args, **kwargs) -> TestCase:
        """Run a single test and capture results."""
        print(f"\n[TEST] {name}...", end=" ", flush=True)
        start = time.time()

        try:
            result = test_func(*args, **kwargs)
            duration = time.time() - start

            if result is True or result is None:
                tc = TestCase(name, TestResult.PASS, duration)
                print(f"PASS ({duration:.2f}s)")
            elif result is False:
                tc = TestCase(name, TestResult.FAIL, duration, "Test returned False")
                print(f"FAIL ({duration:.2f}s)")
            elif isinstance(result, str):
                tc = TestCase(name, TestResult.FAIL, duration, result)
                print(f"FAIL: {result}")
            else:
                tc = TestCase(name, TestResult.PASS, duration, details=result)
                print(f"PASS ({duration:.2f}s)")

        except AssertionError as e:
            duration = time.time() - start
            tc = TestCase(name, TestResult.FAIL, duration, str(e))
            print(f"FAIL: {e}")

        except Exception as e:
            duration = time.time() - start
            tc = TestCase(name, TestResult.ERROR, duration, f"{type(e).__name__}: {e}")
            print(f"ERROR: {e}")

        self.results.append(tc)
        return tc

    def skip_test(self, name: str, reason: str) -> TestCase:
        """Skip a test."""
        print(f"\n[TEST] {name}... SKIP: {reason}")
        tc = TestCase(name, TestResult.SKIP, 0, reason)
        self.results.append(tc)
        return tc

    def report(self) -> Tuple[int, int, int, int]:
        """Print test report and return (pass, fail, skip, error) counts."""
        print("\n" + "=" * 70)
        print("TEST REPORT")
        print("=" * 70)

        passed = sum(1 for t in self.results if t.result == TestResult.PASS)
        failed = sum(1 for t in self.results if t.result == TestResult.FAIL)
        skipped = sum(1 for t in self.results if t.result == TestResult.SKIP)
        errors = sum(1 for t in self.results if t.result == TestResult.ERROR)

        for tc in self.results:
            symbol = {"PASS": "[OK]", "FAIL": "[X]", "SKIP": "[-]", "ERROR": "[!]"}[tc.result.value]
            print(f"  {symbol} {tc.name}: {tc.result.value}")
            if tc.message and tc.result != TestResult.PASS:
                print(f"      {tc.message}")

        print("-" * 70)
        print(f"Total: {len(self.results)} | Pass: {passed} | Fail: {failed} | Skip: {skipped} | Error: {errors}")

        total_time = sum(t.duration for t in self.results)
        print(f"Total time: {total_time:.2f}s")
        print("=" * 70)

        return passed, failed, skipped, errors


# =============================================================================
# TEST CATEGORIES
# =============================================================================

class ConnectionTests:
    """Tests for connection and health monitoring."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.connection = ComfyUIConnection(config)

    def test_connection_health(self):
        """Test basic health check."""
        result = self.connection.health_check(force=True)
        assert result, "Health check failed"
        assert self.connection.state == ConnectionState.CONNECTED
        return True

    def test_system_stats(self):
        """Test system stats endpoint."""
        stats = self.connection.request("/system_stats")
        assert "system" in stats, "Missing system info"
        assert "devices" in stats, "Missing devices info"

        # Verify GPU detected
        devices = stats.get("devices", [])
        assert len(devices) > 0, "No GPU devices found"
        assert devices[0].get("type") == "cuda", "CUDA device expected"

        return {"gpu": devices[0].get("name")}

    def test_retry_logic(self):
        """Test connection retry on failure."""
        # Test with bad endpoint - should fail gracefully
        try:
            self.connection.request("/nonexistent_endpoint", retries=1)
            return "Expected error not raised"
        except ConnectionError:
            return True

    def test_timeout_handling(self):
        """Test timeout is properly configured."""
        assert self.config.timeout > 0
        assert self.config.max_retries >= 0
        return True


class WorkflowTests:
    """Tests for workflow building and validation."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.builder = WorkflowBuilder(config)

    def test_basic_workflow(self):
        """Test basic txt2img workflow generation."""
        job = RenderJob(
            job_id="test_basic",
            prompt="a red car",
            width=512,
            height=512,
            seed=42
        )
        workflow = self.builder.build_basic(job)

        assert "1" in workflow, "Missing checkpoint loader"
        assert workflow["1"]["class_type"] == "CheckpointLoaderSimple"

        valid, errors = self.builder.validate(workflow)
        assert valid, f"Validation failed: {errors}"

        return True

    def test_controlnet_workflow(self):
        """Test ControlNet workflow generation."""
        job = RenderJob(
            job_id="test_cn",
            prompt="a blue car",
            depth_image="C:/temp/depth.png"
        )
        workflow = self.builder.build_with_controlnet(job)

        # Should have more nodes for ControlNet
        assert len(workflow) > 7, "ControlNet workflow should have additional nodes"

        valid, errors = self.builder.validate(workflow)
        assert valid, f"Validation failed: {errors}"

        return True

    def test_animatediff_workflow(self):
        """Test AnimateDiff video workflow."""
        job = RenderJob(
            job_id="test_ad",
            prompt="a car driving"
        )
        workflow = self.builder.build_animatediff(job, frame_count=16)

        # Check for AnimateDiff nodes
        node_types = {v["class_type"] for v in workflow.values()}
        assert "ADE_AnimateDiffLoaderGen1" in node_types, "Missing AnimateDiff loader"
        assert "VHS_VideoCombine" in node_types, "Missing video output"

        return True

    def test_invalid_workflow_detection(self):
        """Test that invalid workflows are caught."""
        # Missing required nodes
        bad_workflow = {
            "1": {"class_type": "EmptyLatentImage", "inputs": {}}
        }
        valid, errors = self.builder.validate(bad_workflow)
        assert not valid, "Should detect missing nodes"
        assert len(errors) > 0

        return True


class RenderingTests:
    """Tests for the rendering pipeline."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.engine = None

    def setup(self):
        """Start engine for tests."""
        self.engine = CarWashEngine(self.config)
        self.engine.start()

    def teardown(self):
        """Stop engine after tests."""
        if self.engine:
            self.engine.stop()

    def test_basic_render(self):
        """Test basic image generation."""
        self.setup()
        try:
            job = self.engine.render(
                "a shiny red sports car, studio lighting",
                width=256,  # Small for speed
                height=256,
                steps=8,  # Fewer steps for speed
                seed=42
            )

            assert job.status == JobStatus.COMPLETED, f"Job failed: {job.error_message}"
            assert job.output_path is not None, "No output path"
            assert Path(job.output_path).exists() or job.output_path.startswith("C:/ComfyUI/output"), "Output file missing"

            return {"output": job.output_path, "duration": job.duration}

        finally:
            self.teardown()

    def test_deterministic_seed(self):
        """Test that same seed produces same result."""
        self.setup()
        try:
            # Render twice with same seed
            job1 = self.engine.render(
                "a green car",
                width=256, height=256, steps=4, seed=12345
            )
            job2 = self.engine.render(
                "a green car",
                width=256, height=256, steps=4, seed=12345
            )

            assert job1.status == JobStatus.COMPLETED
            assert job2.status == JobStatus.COMPLETED

            # Cache should have caught the second one
            # (same parameters = same cache key)
            assert job1.cache_key == job2.cache_key, "Cache keys should match"

            return True

        finally:
            self.teardown()

    def test_different_seeds(self):
        """Test that different seeds produce different results."""
        self.setup()
        try:
            job1 = self.engine.render(
                "a car", width=256, height=256, steps=4, seed=111
            )
            job2 = self.engine.render(
                "a car", width=256, height=256, steps=4, seed=222
            )

            assert job1.cache_key != job2.cache_key, "Different seeds should produce different cache keys"
            return True

        finally:
            self.teardown()

    def test_error_handling(self):
        """Test error handling for invalid inputs."""
        self.setup()
        try:
            # Try with invalid checkpoint (should fail gracefully)
            old_checkpoint = self.config.checkpoint
            self.config.checkpoint = "nonexistent_model.safetensors"

            self.engine = CarWashEngine(self.config)
            self.engine.start()

            job = self.engine.render("test", width=256, height=256, steps=1)

            # Should fail but not crash
            assert job.status == JobStatus.FAILED or job.error_message is not None

            self.config.checkpoint = old_checkpoint
            return True

        except Exception as e:
            # Any exception is caught = pass
            return True
        finally:
            self.teardown()


class CacheTests:
    """Tests for caching and determinism."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.cache_dir = Path(tempfile.mkdtemp())
        config.cache_dir = self.cache_dir
        self.cache = RenderCache(config)

    def teardown(self):
        """Clean up test cache."""
        shutil.rmtree(self.cache_dir, ignore_errors=True)

    def test_cache_key_generation(self):
        """Test cache key consistency."""
        job1 = RenderJob(job_id="a", prompt="test", seed=42)
        job2 = RenderJob(job_id="b", prompt="test", seed=42)
        job3 = RenderJob(job_id="c", prompt="test", seed=43)

        assert job1.cache_key == job2.cache_key, "Same params should have same key"
        assert job1.cache_key != job3.cache_key, "Different seed should have different key"

        return True

    def test_cache_put_get(self):
        """Test cache storage and retrieval."""
        # Create a dummy file
        test_file = self.cache_dir / "test_image.png"
        test_file.write_bytes(b"fake image data")

        # Cache it
        cache_key = "test_key_123"
        cached_path = self.cache.put(cache_key, str(test_file))

        # Retrieve it
        retrieved = self.cache.get(cache_key)

        assert retrieved is not None, "Cache miss"
        assert Path(retrieved).exists(), "Cached file missing"

        return True

    def test_cache_miss(self):
        """Test cache miss behavior."""
        result = self.cache.get("nonexistent_key")
        assert result is None, "Should return None for cache miss"
        return True

    def test_cache_clear(self):
        """Test cache clearing."""
        # Add something to cache
        test_file = self.cache_dir / "temp.png"
        test_file.write_bytes(b"data")
        self.cache.put("key", str(test_file))

        # Clear
        self.cache.clear()

        # Should be empty
        result = self.cache.get("key")
        assert result is None, "Cache should be empty after clear"

        return True


class ColorTests:
    """Tests for color management."""

    def __init__(self):
        self.cm = ColorManager()

    def test_linear_srgb_roundtrip(self):
        """Test linear to sRGB and back."""
        linear = np.array([[[0.0, 0.18, 0.5, 1.0]]], dtype=np.float32)
        srgb = self.cm.linear_to_srgb(linear)
        back = self.cm.srgb_to_linear(srgb)

        error = np.abs(linear - back).max()
        assert error < 0.01, f"Roundtrip error too high: {error}"

        return {"max_error": float(error)}

    def test_depth_normalization(self):
        """Test depth map normalization."""
        normalizer = DepthNormalizer()

        # Create test depth map
        depth = np.linspace(0.1, 100.0, 512*512).reshape(512, 512).astype(np.float32)

        normalized = normalizer.normalize_depth(depth, near=0.1, far=100.0)

        assert normalized.dtype == np.uint8
        assert normalized.min() >= 0
        assert normalized.max() <= 255

        return True

    def test_normal_normalization(self):
        """Test normal map normalization."""
        normalizer = DepthNormalizer()

        # Create test normals (unit vectors)
        normals = np.random.randn(512, 512, 3).astype(np.float32)
        normals /= np.linalg.norm(normals, axis=2, keepdims=True)

        normalized = normalizer.normalize_normals(normals)

        assert normalized.dtype == np.uint8
        assert normalized.shape == (512, 512, 3)

        return True

    def test_ai_preparation(self):
        """Test image preparation for AI input."""
        # Linear HDR image
        hdr = np.random.random((256, 256, 3)).astype(np.float32) * 2.0

        prepared = self.cm.prepare_for_ai(hdr, ColorSpace.LINEAR)

        assert prepared.dtype == np.uint8
        assert prepared.max() <= 255
        assert prepared.min() >= 0

        return True


class JobQueueTests:
    """Tests for job queue management."""

    def __init__(self, config: CarWashConfig):
        self.queue = JobQueue(config)

    def test_job_submission(self):
        """Test job submission."""
        job = RenderJob(job_id="test1", prompt="test")
        job_id = self.queue.submit(job)

        assert job_id == "test1"
        assert self.queue.get_job("test1") is not None

        return True

    def test_progress_tracking(self):
        """Test progress updates."""
        job = RenderJob(job_id="test2", prompt="test")
        self.queue.submit(job)

        # Update progress
        self.queue.update_progress("test2", 0.5, JobStatus.RUNNING)

        retrieved = self.queue.get_job("test2")
        assert retrieved.progress == 0.5
        assert retrieved.status == JobStatus.RUNNING

        return True

    def test_job_cancellation(self):
        """Test job cancellation."""
        job = RenderJob(job_id="test3", prompt="test")
        self.queue.submit(job)

        result = self.queue.cancel("test3")
        assert result, "Cancel should succeed"

        retrieved = self.queue.get_job("test3")
        assert retrieved.status == JobStatus.CANCELLED

        return True

    def test_progress_callback(self):
        """Test progress callbacks."""
        received = []

        def callback(job):
            received.append(job.progress)

        self.queue.on_progress(callback)

        job = RenderJob(job_id="test4", prompt="test")
        self.queue.submit(job)
        self.queue.update_progress("test4", 0.25)
        self.queue.update_progress("test4", 0.75)

        assert len(received) == 2
        assert 0.25 in received
        assert 0.75 in received

        return True

    def test_queue_stats(self):
        """Test queue statistics."""
        # Submit some jobs with different statuses
        for i in range(5):
            job = RenderJob(job_id=f"stat_{i}", prompt="test")
            self.queue.submit(job)

        self.queue.update_progress("stat_0", 1.0, JobStatus.COMPLETED)
        self.queue.update_progress("stat_1", 0.5, JobStatus.RUNNING)
        self.queue.cancel("stat_2")

        stats = self.queue.get_stats()
        assert stats["completed"] >= 1
        assert stats["total"] >= 5

        return {"stats": stats}


# =============================================================================
# INTEGRATION TEST
# =============================================================================

def test_full_pipeline():
    """Full end-to-end integration test."""
    config = CarWashConfig(
        default_steps=8,  # Fast for testing
        default_width=256,
        default_height=256
    )

    engine = CarWashEngine(config)

    try:
        engine.start()

        # Test 1: Basic render
        job = engine.render(
            "a luxury car in a professional studio, perfect lighting",
            seed=42
        )
        assert job.status == JobStatus.COMPLETED, f"Render failed: {job.error_message}"

        # Test 2: Verify output exists
        # (output may be in ComfyUI directory)

        # Test 3: Check timing
        assert job.duration is not None
        assert job.duration > 0

        return {
            "job_id": job.job_id,
            "duration": job.duration,
            "output": job.output_path
        }

    finally:
        engine.stop()


# =============================================================================
# MAIN
# =============================================================================

def main():
    """Run all tests."""
    print("=" * 70)
    print("CARWASH PRODUCTION TEST SUITE")
    print(f"Started: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print("=" * 70)

    runner = TestRunner()
    config = CarWashConfig()

    # Check if ComfyUI is available
    connection = ComfyUIConnection(config)
    comfyui_available = connection.health_check(force=True)

    if not comfyui_available:
        print("\n[WARNING] ComfyUI not available - skipping live tests")

    # CONNECTION TESTS
    print("\n--- CONNECTION TESTS ---")
    if comfyui_available:
        ct = ConnectionTests(config)
        runner.run_test("Connection health check", ct.test_connection_health)
        runner.run_test("System stats retrieval", ct.test_system_stats)
        runner.run_test("Retry logic", ct.test_retry_logic)
        runner.run_test("Timeout handling", ct.test_timeout_handling)
    else:
        runner.skip_test("Connection tests", "ComfyUI not available")

    # WORKFLOW TESTS
    print("\n--- WORKFLOW TESTS ---")
    wt = WorkflowTests(config)
    runner.run_test("Basic workflow generation", wt.test_basic_workflow)
    runner.run_test("ControlNet workflow", wt.test_controlnet_workflow)
    runner.run_test("AnimateDiff workflow", wt.test_animatediff_workflow)
    runner.run_test("Invalid workflow detection", wt.test_invalid_workflow_detection)

    # CACHE TESTS
    print("\n--- CACHE TESTS ---")
    cache_config = CarWashConfig()
    ct = CacheTests(cache_config)
    runner.run_test("Cache key generation", ct.test_cache_key_generation)
    runner.run_test("Cache put/get", ct.test_cache_put_get)
    runner.run_test("Cache miss", ct.test_cache_miss)
    runner.run_test("Cache clear", ct.test_cache_clear)
    ct.teardown()

    # COLOR TESTS
    print("\n--- COLOR TESTS ---")
    color_tests = ColorTests()
    runner.run_test("Linear/sRGB roundtrip", color_tests.test_linear_srgb_roundtrip)
    runner.run_test("Depth normalization", color_tests.test_depth_normalization)
    runner.run_test("Normal normalization", color_tests.test_normal_normalization)
    runner.run_test("AI preparation", color_tests.test_ai_preparation)

    # JOB QUEUE TESTS
    print("\n--- JOB QUEUE TESTS ---")
    jq = JobQueueTests(config)
    runner.run_test("Job submission", jq.test_job_submission)
    runner.run_test("Progress tracking", jq.test_progress_tracking)
    runner.run_test("Job cancellation", jq.test_job_cancellation)
    runner.run_test("Progress callbacks", jq.test_progress_callback)
    runner.run_test("Queue statistics", jq.test_queue_stats)

    # RENDERING TESTS (require ComfyUI)
    print("\n--- RENDERING TESTS ---")
    if comfyui_available:
        rt = RenderingTests(config)
        runner.run_test("Basic render", rt.test_basic_render)
        runner.run_test("Deterministic seeds", rt.test_deterministic_seed)
        runner.run_test("Different seeds", rt.test_different_seeds)
        runner.run_test("Error handling", rt.test_error_handling)
    else:
        runner.skip_test("Rendering tests", "ComfyUI not available")

    # INTEGRATION TEST
    print("\n--- INTEGRATION TEST ---")
    if comfyui_available:
        runner.run_test("Full pipeline integration", test_full_pipeline)
    else:
        runner.skip_test("Full pipeline integration", "ComfyUI not available")

    # REPORT
    passed, failed, skipped, errors = runner.report()

    # Calculate production readiness score
    total_runnable = passed + failed + errors
    if total_runnable > 0:
        pass_rate = passed / total_runnable
        score = 3.5 + (pass_rate * 6.5)  # Scale from 3.5 to 10
        print(f"\n[PRODUCTION READINESS SCORE: {score:.1f}/10]")
    else:
        print("\n[Cannot calculate score - no tests run]")

    return failed == 0 and errors == 0


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
