"""Verify the locked Warp runtime on CPU, CUDA and a captured CUDA graph."""

# Warp kernel annotations construct descriptors consumed by its compiler.
# pyright: reportInvalidTypeForm=false

from pathlib import Path

import numpy as np
import pytest
import warp as wp

ROOT = Path(__file__).resolve().parents[2]


@wp.kernel
def scale_and_shift(values: wp.array(dtype=wp.float32), output: wp.array(dtype=wp.float32)):
    index = wp.tid()
    output[index] = values[index] * 2.0 + 1.0


@pytest.fixture(scope="module", autouse=True)
def runtime():
    wp.config.kernel_cache_dir = str(ROOT / ".cache/warp/acceptance")
    wp.config.mode = "debug"
    wp.config.verify_fp = True
    wp.config.verify_cuda = True
    wp.config.lineinfo = True
    wp.init()
    wp.set_module_options({"enable_backward": False, "fast_math": False, "fuse_fp": False})
    assert wp.__version__ == "1.18.0"


@pytest.mark.parametrize("device", ["cpu", "cuda:0"])
def test_kernel_matches_reference(device: str) -> None:
    values = np.arange(32, dtype=np.float32) * 0.5
    source = wp.array(values, dtype=wp.float32, device=device)
    output = wp.empty(32, dtype=float, device=device)
    wp.launch(scale_and_shift, dim=32, inputs=[source, output], device=device)
    wp.synchronize_device(device)
    np.testing.assert_array_equal(output.numpy(), values * 2.0 + 1.0)


def test_cuda_graph_matches_reference() -> None:
    device = wp.get_device("cuda:0")
    values = np.arange(32, dtype=np.float32) * 0.5
    source = wp.array(values, dtype=wp.float32, device=device)
    output = wp.empty(32, dtype=float, device=device)
    wp.launch(scale_and_shift, dim=32, inputs=[source, output], device=device)
    wp.synchronize_device(device)
    previous = wp.config.verify_cuda
    try:
        wp.config.verify_cuda = False
        with wp.ScopedCapture(device=device, force_module_load=True) as capture:
            wp.launch(scale_and_shift, dim=32, inputs=[source, output], device=device)
        assert capture.graph is not None
        wp.capture_launch(capture.graph)
        wp.synchronize_device(device)
        np.testing.assert_array_equal(output.numpy(), values * 2.0 + 1.0)
    finally:
        wp.config.verify_cuda = previous
