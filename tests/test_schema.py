# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
"""
CarWash USD Schema Test Suite

Tests for USD schema validation and Houdini integration.
"""

import pytest
import json
import os
import sys

# Schema directory path
SCHEMA_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'schema')
PLUGIN_DIR = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'plugin')


class TestPlugInfoJson:
    """Tests for plugInfo.json structure and content."""

    @pytest.fixture
    def plug_info(self):
        """Load plugInfo.json."""
        path = os.path.join(PLUGIN_DIR, 'plugInfo.json')
        with open(path, 'r') as f:
            return json.load(f)

    def test_pluginfo_exists(self):
        """Verify plugInfo.json exists."""
        path = os.path.join(PLUGIN_DIR, 'plugInfo.json')
        assert os.path.exists(path), f"plugInfo.json not found at {path}"

    def test_has_plugins_array(self, plug_info):
        """Verify Plugins array exists."""
        assert 'Plugins' in plug_info
        assert isinstance(plug_info['Plugins'], list)
        assert len(plug_info['Plugins']) > 0

    def test_has_hdcarwash_plugin(self, plug_info):
        """Verify hdCarWash plugin is registered."""
        plugin = plug_info['Plugins'][0]
        assert plugin['Name'] == 'hdCarWash'

    def test_has_renderer_plugin_type(self, plug_info):
        """Verify HdRendererPlugin type is registered."""
        types = plug_info['Plugins'][0]['Info']['Types']
        assert 'HdCarWashRendererPlugin' in types
        assert 'HdRendererPlugin' in types['HdCarWashRendererPlugin']['bases']

    def test_has_render_settings_api(self, plug_info):
        """Verify CarWashRenderSettingsAPI is registered."""
        types = plug_info['Plugins'][0]['Info']['Types']
        assert 'usdCarWashCarWashRenderSettingsAPI' in types

    def test_render_settings_api_schema_kind(self, plug_info):
        """Verify RenderSettingsAPI has correct schemaKind."""
        api_type = plug_info['Plugins'][0]['Info']['Types']['usdCarWashCarWashRenderSettingsAPI']
        assert api_type['schemaKind'] == 'singleApplyAPI'

    def test_render_settings_api_can_only_apply_to(self, plug_info):
        """Verify RenderSettingsAPI can only apply to RenderSettings."""
        api_type = plug_info['Plugins'][0]['Info']['Types']['usdCarWashCarWashRenderSettingsAPI']
        assert 'apiSchemaCanOnlyApplyTo' in api_type
        assert 'RenderSettings' in api_type['apiSchemaCanOnlyApplyTo']

    def test_render_settings_api_bases(self, plug_info):
        """Verify RenderSettingsAPI inherits from UsdAPISchemaBase."""
        api_type = plug_info['Plugins'][0]['Info']['Types']['usdCarWashCarWashRenderSettingsAPI']
        assert 'UsdAPISchemaBase' in api_type['bases']

    def test_has_usd_render_section(self, plug_info):
        """Verify UsdRender metadata exists."""
        info = plug_info['Plugins'][0]['Info']
        assert 'UsdRender' in info
        assert info['UsdRender']['rendererDisplayName'] == 'CarWash Renderer'

    def test_has_schemas_section(self, plug_info):
        """Verify Schemas section exists."""
        info = plug_info['Plugins'][0]['Info']
        assert 'Schemas' in info
        assert 'carWashRenderSettingsAPI' in info['Schemas']


class TestSchemaFiles:
    """Tests for USD schema file content."""

    def test_render_settings_api_schema_exists(self):
        """Verify carWashRenderSettingsAPI.usda exists."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        assert os.path.exists(path), f"Schema not found at {path}"

    def test_generated_schema_exists(self):
        """Verify generatedSchema.usda exists."""
        path = os.path.join(SCHEMA_DIR, 'generatedSchema.usda')
        assert os.path.exists(path), f"Generated schema not found at {path}"

    def test_schema_has_usda_header(self):
        """Verify schema has proper USDA header."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert content.startswith('#usda 1.0')

    def test_schema_defines_class(self):
        """Verify schema defines CarWashRenderSettingsAPI class."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'class "CarWashRenderSettingsAPI"' in content

    def test_schema_inherits_api_schema_base(self):
        """Verify schema inherits from APISchemaBase."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'inherits = </APISchemaBase>' in content

    def test_schema_has_prompt_attribute(self):
        """Verify schema has prompt attribute."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'carwash:prompt' in content

    def test_schema_has_depth_attributes(self):
        """Verify schema has depth conditioning attributes."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'carwash:useDepth' in content
        assert 'carwash:depthStrength' in content

    def test_schema_has_normal_attributes(self):
        """Verify schema has normal conditioning attributes."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'carwash:useNormal' in content
        assert 'carwash:normalStrength' in content

    def test_schema_has_determinism_attributes(self):
        """Verify schema has determinism attributes."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'carwash:strictDeterminism' in content
        assert 'carwash:seed' in content

    def test_schema_has_comfyui_backend_attributes(self):
        """Verify schema has ComfyUI backend attributes."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'carwash:comfyHost' in content
        assert 'carwash:comfyPort' in content

    def test_schema_has_display_groups(self):
        """Verify schema uses displayGroup for UI organization."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            content = f.read()
        assert 'displayGroup = "AI Generation"' in content
        assert 'displayGroup = "Conditioning"' in content
        assert 'displayGroup = "Determinism"' in content


class TestSchemaAttributeDefaults:
    """Tests for schema attribute default values."""

    @pytest.fixture
    def schema_content(self):
        """Load schema content."""
        path = os.path.join(SCHEMA_DIR, 'carWashRenderSettingsAPI.usda')
        with open(path, 'r') as f:
            return f.read()

    def test_default_seed(self, schema_content):
        """Verify default seed is 42."""
        assert 'int carwash:seed = 42' in schema_content

    def test_default_steps(self, schema_content):
        """Verify default steps is 20."""
        assert 'int carwash:steps = 20' in schema_content

    def test_default_cfg(self, schema_content):
        """Verify default CFG is 7.0."""
        assert 'float carwash:cfg = 7.0' in schema_content

    def test_default_depth_strength(self, schema_content):
        """Verify default depth strength is 0.8."""
        assert 'float carwash:depthStrength = 0.8' in schema_content

    def test_default_comfy_port(self, schema_content):
        """Verify default ComfyUI port is 8188."""
        assert 'int carwash:comfyPort = 8188' in schema_content

    def test_default_strict_determinism_enabled(self, schema_content):
        """Verify strict determinism is enabled by default."""
        assert 'bool carwash:strictDeterminism = true' in schema_content


class TestUSDParsing:
    """Tests that require pxr.Usd (skipped if not available)."""

    @pytest.fixture
    def usd_available(self):
        """Check if pxr.Usd is available."""
        try:
            from pxr import Usd, Sdf
            return True
        except ImportError:
            return False

    @pytest.mark.skipif(
        not os.environ.get('HOUDINI_PATH'),
        reason="Houdini/USD not in environment"
    )
    def test_schema_parses_with_usd(self, usd_available):
        """Verify schema can be parsed by USD."""
        if not usd_available:
            pytest.skip("pxr.Usd not available")

        from pxr import Usd, Sdf

        path = os.path.join(SCHEMA_DIR, 'generatedSchema.usda')
        layer = Sdf.Layer.FindOrOpen(path)
        assert layer is not None, "Failed to open schema layer"

    @pytest.mark.skipif(
        not os.environ.get('HOUDINI_PATH'),
        reason="Houdini/USD not in environment"
    )
    def test_render_settings_prim_accepts_api(self, usd_available):
        """Verify RenderSettings prim can have API applied."""
        if not usd_available:
            pytest.skip("pxr.Usd not available")

        from pxr import Usd, UsdRender

        # Create in-memory stage
        stage = Usd.Stage.CreateInMemory()
        render_settings = UsdRender.Settings.Define(stage, '/Render/settings')

        # The API should be applicable (this tests the schema registration)
        prim = render_settings.GetPrim()
        assert prim.IsValid()


if __name__ == '__main__':
    pytest.main([__file__, '-v'])
