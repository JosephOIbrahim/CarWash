# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
"""
CarWash ComfyUI Nodes Test Suite

Tests for custom ComfyUI nodes functionality.
"""

import pytest
import torch
import numpy as np
import sys
import os

# Add parent directory to path for imports
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class TestNodeRegistration:
    """Tests for ComfyUI node registration."""

    def test_node_class_mappings_exists(self):
        """Verify NODE_CLASS_MAPPINGS is defined."""
        from comfyui.custom_nodes.carwash import NODE_CLASS_MAPPINGS
        assert isinstance(NODE_CLASS_MAPPINGS, dict)

    def test_node_display_name_mappings_exists(self):
        """Verify NODE_DISPLAY_NAME_MAPPINGS is defined."""
        from comfyui.custom_nodes.carwash import NODE_DISPLAY_NAME_MAPPINGS
        assert isinstance(NODE_DISPLAY_NAME_MAPPINGS, dict)

    def test_all_nodes_registered(self):
        """Verify all expected nodes are registered."""
        from comfyui.custom_nodes.carwash import NODE_CLASS_MAPPINGS

        expected_nodes = [
            'CarWashDeterministicSampler',
            'CarWashContextLoader',
            'CarWashStyleConditioner',
            'CarWashTemporalBlend',
        ]

        for node in expected_nodes:
            assert node in NODE_CLASS_MAPPINGS, f"Node {node} not registered"

    def test_all_nodes_have_display_names(self):
        """Verify all nodes have display names."""
        from comfyui.custom_nodes.carwash import (
            NODE_CLASS_MAPPINGS,
            NODE_DISPLAY_NAME_MAPPINGS
        )

        for node_name in NODE_CLASS_MAPPINGS:
            assert node_name in NODE_DISPLAY_NAME_MAPPINGS, \
                f"Node {node_name} missing display name"


class TestCarWashDeterministicSampler:
    """Tests for CarWashDeterministicSampler node."""

    @pytest.fixture
    def sampler_class(self):
        """Get the sampler class."""
        from comfyui.custom_nodes.carwash import CarWashDeterministicSampler
        return CarWashDeterministicSampler

    def test_has_required_attributes(self, sampler_class):
        """Verify node has required ComfyUI attributes."""
        assert hasattr(sampler_class, 'CATEGORY')
        assert hasattr(sampler_class, 'RETURN_TYPES')
        assert hasattr(sampler_class, 'FUNCTION')
        assert hasattr(sampler_class, 'INPUT_TYPES')

    def test_category_is_carwash(self, sampler_class):
        """Verify node category."""
        assert 'CarWash' in sampler_class.CATEGORY

    def test_returns_latent(self, sampler_class):
        """Verify node returns LATENT type."""
        assert 'LATENT' in sampler_class.RETURN_TYPES

    def test_input_types_has_required_fields(self, sampler_class):
        """Verify required input fields exist."""
        input_types = sampler_class.INPUT_TYPES()
        required = input_types['required']

        expected_fields = ['model', 'positive', 'negative', 'latent_image',
                          'seed', 'steps', 'cfg', 'sampler_name', 'scheduler']

        for field in expected_fields:
            assert field in required, f"Missing required field: {field}"

    def test_has_strict_determinism_option(self, sampler_class):
        """Verify strict_determinism option exists."""
        input_types = sampler_class.INPUT_TYPES()
        optional = input_types.get('optional', {})
        assert 'strict_determinism' in optional

    def test_function_method_exists(self, sampler_class):
        """Verify the function method exists."""
        func_name = sampler_class.FUNCTION
        assert hasattr(sampler_class, func_name)


class TestCarWashContextLoader:
    """Tests for CarWashContextLoader node."""

    @pytest.fixture
    def loader_class(self):
        """Get the loader class."""
        from comfyui.custom_nodes.carwash import CarWashContextLoader
        return CarWashContextLoader

    def test_has_required_attributes(self, loader_class):
        """Verify node has required ComfyUI attributes."""
        assert hasattr(loader_class, 'CATEGORY')
        assert hasattr(loader_class, 'RETURN_TYPES')
        assert hasattr(loader_class, 'FUNCTION')

    def test_returns_multiple_outputs(self, loader_class):
        """Verify node returns depth, normal, beauty, mask."""
        returns = loader_class.RETURN_TYPES
        assert len(returns) == 4
        assert 'IMAGE' in returns
        assert 'MASK' in returns

    def test_return_names_are_descriptive(self, loader_class):
        """Verify return names describe the outputs."""
        names = loader_class.RETURN_NAMES
        assert 'depth' in names
        assert 'normal' in names
        assert 'beauty' in names
        assert 'mask' in names

    def test_input_has_context_path(self, loader_class):
        """Verify context_path input exists."""
        input_types = loader_class.INPUT_TYPES()
        required = input_types['required']
        assert 'context_path' in required
        assert 'frame' in required


class TestCarWashStyleConditioner:
    """Tests for CarWashStyleConditioner node."""

    @pytest.fixture
    def conditioner_class(self):
        """Get the conditioner class."""
        from comfyui.custom_nodes.carwash import CarWashStyleConditioner
        return CarWashStyleConditioner

    def test_has_required_attributes(self, conditioner_class):
        """Verify node has required ComfyUI attributes."""
        assert hasattr(conditioner_class, 'CATEGORY')
        assert hasattr(conditioner_class, 'RETURN_TYPES')
        assert hasattr(conditioner_class, 'FUNCTION')

    def test_returns_conditioning(self, conditioner_class):
        """Verify node returns CONDITIONING type."""
        assert 'CONDITIONING' in conditioner_class.RETURN_TYPES

    def test_input_has_style_settings(self, conditioner_class):
        """Verify style-related inputs exist."""
        input_types = conditioner_class.INPUT_TYPES()
        required = input_types['required']

        assert 'clip' in required
        assert 'prompt' in required
        assert 'style_id' in required

    def test_has_style_cache(self, conditioner_class):
        """Verify class has style cache."""
        assert hasattr(conditioner_class, '_style_cache')

    def test_clear_cache_method_exists(self, conditioner_class):
        """Verify clear_cache method exists."""
        assert hasattr(conditioner_class, 'clear_cache')
        assert callable(conditioner_class.clear_cache)


class TestCarWashTemporalBlend:
    """Tests for CarWashTemporalBlend node."""

    @pytest.fixture
    def blend_class(self):
        """Get the blend class."""
        from comfyui.custom_nodes.carwash import CarWashTemporalBlend
        return CarWashTemporalBlend

    def test_has_required_attributes(self, blend_class):
        """Verify node has required ComfyUI attributes."""
        assert hasattr(blend_class, 'CATEGORY')
        assert hasattr(blend_class, 'RETURN_TYPES')
        assert hasattr(blend_class, 'FUNCTION')

    def test_returns_image(self, blend_class):
        """Verify node returns IMAGE type."""
        assert 'IMAGE' in blend_class.RETURN_TYPES

    def test_input_has_blend_settings(self, blend_class):
        """Verify blend-related inputs exist."""
        input_types = blend_class.INPUT_TYPES()
        required = input_types['required']

        assert 'current_frame' in required
        assert 'blend_strength' in required

    def test_has_frame_buffer(self, blend_class):
        """Verify class has frame buffer."""
        assert hasattr(blend_class, '_frame_buffer')

    def test_clear_buffer_method_exists(self, blend_class):
        """Verify clear_buffer method exists."""
        assert hasattr(blend_class, 'clear_buffer')
        assert callable(blend_class.clear_buffer)

    def test_has_optional_motion_vectors(self, blend_class):
        """Verify motion vectors is optional input."""
        input_types = blend_class.INPUT_TYPES()
        optional = input_types.get('optional', {})
        assert 'motion_vectors' in optional

    def test_has_reset_buffer_option(self, blend_class):
        """Verify reset_buffer option exists."""
        input_types = blend_class.INPUT_TYPES()
        optional = input_types.get('optional', {})
        assert 'reset_buffer' in optional


class TestNodeCategories:
    """Tests for node category organization."""

    def test_all_nodes_in_carwash_category(self):
        """Verify all nodes are in CarWash category tree."""
        from comfyui.custom_nodes.carwash import NODE_CLASS_MAPPINGS

        for name, cls in NODE_CLASS_MAPPINGS.items():
            assert cls.CATEGORY.startswith('CarWash'), \
                f"Node {name} not in CarWash category"

    def test_categories_are_organized(self):
        """Verify nodes have subcategories."""
        from comfyui.custom_nodes.carwash import (
            CarWashDeterministicSampler,
            CarWashContextLoader,
            CarWashStyleConditioner,
            CarWashTemporalBlend,
        )

        # Check subcategories exist
        categories = [
            CarWashDeterministicSampler.CATEGORY,
            CarWashContextLoader.CATEGORY,
            CarWashStyleConditioner.CATEGORY,
            CarWashTemporalBlend.CATEGORY,
        ]

        # Should have at least 2 different subcategories
        unique_categories = set(categories)
        assert len(unique_categories) >= 2, "Nodes should be organized into subcategories"


class TestModuleMetadata:
    """Tests for module-level metadata."""

    def test_version_defined(self):
        """Verify __version__ is defined."""
        from comfyui.custom_nodes import carwash
        assert hasattr(carwash, '__version__')

    def test_author_defined(self):
        """Verify __author__ is defined."""
        from comfyui.custom_nodes import carwash
        assert hasattr(carwash, '__author__')


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
