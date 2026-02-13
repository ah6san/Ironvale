"""
IRONVALE PBR Materials & Texture Baking — Blender 4.2+
=======================================================
Creates and assigns PBR shader networks:
  - Muddy clay (terrain base)
  - Weathered granite (rocks)
  - Oak bark (trees)
  - Aged timber + thatch (buildings)
  - Realistic river water
Bakes to 4096px textures (heightmap EXR 16-bit, normals, AO).

Prerequisites: Run terrain, scatter, and structures scripts first.
"""

import bpy
import math
from mathutils import Vector

CONFIG = {
    "bake_resolution": 4096,
    "bake_margin": 16,
    "bake_samples": 64,
    "heightmap_format": "OPEN_EXR",  # 16-bit EXR
    "texture_format": "PNG",
}


# ============================================================================
# MATERIAL BUILDERS
# ============================================================================

def create_pbr_material(name):
    """Create a new material with Principled BSDF node tree."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    # Clear defaults
    for n in nodes:
        nodes.remove(n)

    # Output
    n_out = nodes.new('ShaderNodeOutputMaterial')
    n_out.location = (600, 0)

    # Principled BSDF
    n_bsdf = nodes.new('ShaderNodeBsdfPrincipled')
    n_bsdf.location = (200, 0)
    links.new(n_bsdf.outputs['BSDF'], n_out.inputs['Surface'])

    return mat, nodes, links, n_bsdf


def create_muddy_clay_material():
    """Terrain base: muddy clay with procedural dirt variation."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_MuddyClay")

    # Base color: brown clay mix
    bsdf.inputs['Base Color'].default_value = (0.22, 0.15, 0.08, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.85
    bsdf.inputs['Specular IOR Level'].default_value = 0.2

    # Noise for color variation
    n_noise = nodes.new('ShaderNodeTexNoise')
    n_noise.location = (-400, 200)
    n_noise.inputs['Scale'].default_value = 80.0
    n_noise.inputs['Detail'].default_value = 8.0
    n_noise.inputs['Roughness'].default_value = 0.7

    # Color ramp: dark mud to lighter clay
    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-200, 200)
    n_ramp.color_ramp.elements[0].position = 0.3
    n_ramp.color_ramp.elements[0].color = (0.12, 0.08, 0.04, 1.0)
    n_ramp.color_ramp.elements[1].position = 0.7
    n_ramp.color_ramp.elements[1].color = (0.30, 0.22, 0.12, 1.0)

    links.new(n_noise.outputs['Fac'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    # Bump from noise
    n_bump_noise = nodes.new('ShaderNodeTexNoise')
    n_bump_noise.location = (-400, -200)
    n_bump_noise.inputs['Scale'].default_value = 200.0
    n_bump_noise.inputs['Detail'].default_value = 6.0

    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.3

    links.new(n_bump_noise.outputs['Fac'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_MuddyClay")
    return mat


def create_granite_material():
    """Weathered granite for boulders."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_Granite")

    bsdf.inputs['Roughness'].default_value = 0.75
    bsdf.inputs['Specular IOR Level'].default_value = 0.35

    # Voronoi for granite crystal pattern
    n_voronoi = nodes.new('ShaderNodeTexVoronoi')
    n_voronoi.location = (-400, 200)
    n_voronoi.inputs['Scale'].default_value = 50.0
    n_voronoi.voronoi_dimensions = '3D'

    # Noise overlay
    n_noise = nodes.new('ShaderNodeTexNoise')
    n_noise.location = (-400, 0)
    n_noise.inputs['Scale'].default_value = 120.0
    n_noise.inputs['Detail'].default_value = 5.0

    # Mix
    n_mix = nodes.new('ShaderNodeMix')
    n_mix.location = (-200, 100)
    n_mix.data_type = 'RGBA'
    n_mix.inputs['Factor'].default_value = 0.4
    n_mix.inputs[6].default_value = (0.45, 0.43, 0.40, 1.0)  # light granite
    n_mix.inputs[7].default_value = (0.25, 0.24, 0.22, 1.0)  # dark spots

    links.new(n_voronoi.outputs['Distance'], n_mix.inputs['Factor'])
    links.new(n_mix.outputs[2], bsdf.inputs['Base Color'])

    # Bump
    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.5

    links.new(n_noise.outputs['Fac'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_Granite")
    return mat


def create_oak_bark_material():
    """Oak bark for tree trunks."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_OakBark")

    bsdf.inputs['Roughness'].default_value = 0.9
    bsdf.inputs['Specular IOR Level'].default_value = 0.15

    # Wave texture for bark ridges
    n_wave = nodes.new('ShaderNodeTexWave')
    n_wave.location = (-400, 200)
    n_wave.inputs['Scale'].default_value = 3.0
    n_wave.inputs['Distortion'].default_value = 8.0
    n_wave.inputs['Detail'].default_value = 4.0
    n_wave.wave_type = 'BANDS'
    n_wave.bands_direction = 'Z'

    # Color ramp
    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-200, 200)
    n_ramp.color_ramp.elements[0].position = 0.35
    n_ramp.color_ramp.elements[0].color = (0.10, 0.06, 0.03, 1.0)
    n_ramp.color_ramp.elements[1].position = 0.65
    n_ramp.color_ramp.elements[1].color = (0.20, 0.13, 0.07, 1.0)

    links.new(n_wave.outputs['Fac'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    # Bump from wave
    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.6

    links.new(n_wave.outputs['Fac'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_OakBark")
    return mat


def create_oak_leaf_material():
    """Oak leaf canopy material."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_OakLeaf")

    bsdf.inputs['Base Color'].default_value = (0.15, 0.28, 0.08, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.7
    bsdf.inputs['Specular IOR Level'].default_value = 0.3

    # Subsurface for leaf translucency
    bsdf.inputs['Subsurface Weight'].default_value = 0.15
    bsdf.inputs['Subsurface Radius'].default_value = (0.3, 0.5, 0.1)

    # Noise variation
    n_noise = nodes.new('ShaderNodeTexNoise')
    n_noise.location = (-400, 200)
    n_noise.inputs['Scale'].default_value = 30.0
    n_noise.inputs['Detail'].default_value = 4.0

    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-200, 200)
    n_ramp.color_ramp.elements[0].position = 0.4
    n_ramp.color_ramp.elements[0].color = (0.08, 0.18, 0.04, 1.0)
    n_ramp.color_ramp.elements[1].position = 0.6
    n_ramp.color_ramp.elements[1].color = (0.22, 0.35, 0.10, 1.0)

    links.new(n_noise.outputs['Fac'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    print("[IRONVALE] Material created: Ironvale_OakLeaf")
    return mat


def create_timber_material():
    """Aged timber for building frames."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_AgedTimber")

    bsdf.inputs['Roughness'].default_value = 0.85
    bsdf.inputs['Specular IOR Level'].default_value = 0.2

    # Wood grain via wave texture
    n_wave = nodes.new('ShaderNodeTexWave')
    n_wave.location = (-400, 200)
    n_wave.inputs['Scale'].default_value = 5.0
    n_wave.inputs['Distortion'].default_value = 4.0
    n_wave.inputs['Detail'].default_value = 3.0

    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-200, 200)
    n_ramp.color_ramp.elements[0].position = 0.3
    n_ramp.color_ramp.elements[0].color = (0.18, 0.12, 0.06, 1.0)
    n_ramp.color_ramp.elements[1].position = 0.7
    n_ramp.color_ramp.elements[1].color = (0.30, 0.22, 0.12, 1.0)

    links.new(n_wave.outputs['Fac'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    # Bump
    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.4

    links.new(n_wave.outputs['Fac'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_AgedTimber")
    return mat


def create_thatch_material():
    """Thatch roof material."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_Thatch")

    bsdf.inputs['Roughness'].default_value = 0.95
    bsdf.inputs['Specular IOR Level'].default_value = 0.1

    # Anisotropic straw pattern
    n_noise = nodes.new('ShaderNodeTexNoise')
    n_noise.location = (-600, 200)
    n_noise.inputs['Scale'].default_value = 200.0
    n_noise.inputs['Detail'].default_value = 8.0
    n_noise.inputs['Roughness'].default_value = 0.9

    # Stretch along one axis for straw look
    n_mapping = nodes.new('ShaderNodeMapping')
    n_mapping.location = (-800, 200)
    n_mapping.inputs['Scale'].default_value = (1.0, 5.0, 1.0)

    n_tc = nodes.new('ShaderNodeTexCoord')
    n_tc.location = (-1000, 200)

    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-400, 200)
    n_ramp.color_ramp.elements[0].position = 0.3
    n_ramp.color_ramp.elements[0].color = (0.35, 0.28, 0.12, 1.0)
    n_ramp.color_ramp.elements[1].position = 0.7
    n_ramp.color_ramp.elements[1].color = (0.50, 0.40, 0.20, 1.0)

    links.new(n_tc.outputs['Object'], n_mapping.inputs['Vector'])
    links.new(n_mapping.outputs['Vector'], n_noise.inputs['Vector'])
    links.new(n_noise.outputs['Fac'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    # Strong bump for straw texture
    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.7

    links.new(n_noise.outputs['Fac'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_Thatch")
    return mat


def create_water_material():
    """Realistic river water with refraction and wave normals."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_Water")

    mat.blend_method = 'OPAQUE'  # Eevee setting

    bsdf.inputs['Base Color'].default_value = (0.02, 0.06, 0.08, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.05
    bsdf.inputs['Specular IOR Level'].default_value = 0.8
    bsdf.inputs['IOR'].default_value = 1.333
    bsdf.inputs['Transmission Weight'].default_value = 0.6
    bsdf.inputs['Alpha'].default_value = 0.85

    # Wave normals — two scales
    n_wave1 = nodes.new('ShaderNodeTexWave')
    n_wave1.location = (-600, -200)
    n_wave1.inputs['Scale'].default_value = 8.0
    n_wave1.inputs['Distortion'].default_value = 2.0
    n_wave1.inputs['Detail'].default_value = 3.0

    n_wave2 = nodes.new('ShaderNodeTexWave')
    n_wave2.location = (-600, -400)
    n_wave2.inputs['Scale'].default_value = 25.0
    n_wave2.inputs['Distortion'].default_value = 1.0
    n_wave2.inputs['Detail'].default_value = 2.0

    n_mix_wave = nodes.new('ShaderNodeMix')
    n_mix_wave.location = (-400, -300)
    n_mix_wave.data_type = 'FLOAT'
    n_mix_wave.inputs['Factor'].default_value = 0.5

    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -300)
    n_bump.inputs['Strength'].default_value = 0.15

    links.new(n_wave1.outputs['Fac'], n_mix_wave.inputs[2])
    links.new(n_wave2.outputs['Fac'], n_mix_wave.inputs[3])
    links.new(n_mix_wave.outputs[0], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_Water")
    return mat


def create_stone_material():
    """Stone for bridge and boundary walls."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_Stone")

    bsdf.inputs['Roughness'].default_value = 0.8
    bsdf.inputs['Specular IOR Level'].default_value = 0.3

    # Voronoi for stone block pattern
    n_voronoi = nodes.new('ShaderNodeTexVoronoi')
    n_voronoi.location = (-400, 200)
    n_voronoi.inputs['Scale'].default_value = 8.0
    n_voronoi.voronoi_dimensions = '3D'
    n_voronoi.feature = 'F1'

    n_ramp = nodes.new('ShaderNodeValToRGB')
    n_ramp.location = (-200, 200)
    n_ramp.color_ramp.elements[0].position = 0.0
    n_ramp.color_ramp.elements[0].color = (0.35, 0.33, 0.30, 1.0)
    n_ramp.color_ramp.elements[1].position = 1.0
    n_ramp.color_ramp.elements[1].color = (0.55, 0.52, 0.48, 1.0)

    links.new(n_voronoi.outputs['Distance'], n_ramp.inputs['Fac'])
    links.new(n_ramp.outputs['Color'], bsdf.inputs['Base Color'])

    # Bump from voronoi edges
    n_bump = nodes.new('ShaderNodeBump')
    n_bump.location = (0, -200)
    n_bump.inputs['Strength'].default_value = 0.5

    links.new(n_voronoi.outputs['Distance'], n_bump.inputs['Height'])
    links.new(n_bump.outputs['Normal'], bsdf.inputs['Normal'])

    print("[IRONVALE] Material created: Ironvale_Stone")
    return mat


def create_grass_material():
    """Wildgrass blade material."""
    mat, nodes, links, bsdf = create_pbr_material("Ironvale_Grass")

    bsdf.inputs['Base Color'].default_value = (0.12, 0.22, 0.05, 1.0)
    bsdf.inputs['Roughness'].default_value = 0.65
    bsdf.inputs['Subsurface Weight'].default_value = 0.1
    bsdf.inputs['Subsurface Radius'].default_value = (0.2, 0.4, 0.05)

    print("[IRONVALE] Material created: Ironvale_Grass")
    return mat


# ============================================================================
# MATERIAL ASSIGNMENT
# ============================================================================

def assign_material(obj, mat):
    """Assign material to object, adding slot if needed."""
    if obj.data is None:
        return
    if len(obj.data.materials) == 0:
        obj.data.materials.append(mat)
    else:
        obj.data.materials[0] = mat


def assign_materials_to_scene():
    """Walk scene objects and assign materials by naming convention."""
    mat_clay = create_muddy_clay_material()
    mat_granite = create_granite_material()
    mat_oak_bark = create_oak_bark_material()
    mat_oak_leaf = create_oak_leaf_material()
    mat_timber = create_timber_material()
    mat_thatch = create_thatch_material()
    mat_water = create_water_material()
    mat_stone = create_stone_material()
    mat_grass = create_grass_material()

    for obj in bpy.data.objects:
        name = obj.name.lower()

        if "terrain" in name:
            assign_material(obj, mat_clay)
        elif "waterplane" in name or "water_plane" in name:
            assign_material(obj, mat_water)
        elif "boulder" in name:
            assign_material(obj, mat_granite)
        elif "oak" in name:
            assign_material(obj, mat_oak_bark)
            # Add leaf material as second slot
            if obj.data and len(obj.data.materials) < 2:
                obj.data.materials.append(mat_oak_leaf)
        elif "birch" in name:
            assign_material(obj, mat_oak_bark)  # Reuse bark, retint later
            if obj.data and len(obj.data.materials) < 2:
                obj.data.materials.append(mat_oak_leaf)
        elif "house" in name or "mill" in name:
            assign_material(obj, mat_timber)
            if obj.data and len(obj.data.materials) < 2:
                obj.data.materials.append(mat_thatch)
        elif "palisade" in name:
            assign_material(obj, mat_timber)
        elif "bridge" in name or "farmwall" in name or "boundary" in name:
            assign_material(obj, mat_stone)
        elif "farm_" in name and "wall" not in name:
            assign_material(obj, mat_clay)
        elif "grass" in name:
            assign_material(obj, mat_grass)

    print("[IRONVALE] Materials assigned to all scene objects.")


# ============================================================================
# TEXTURE BAKING
# ============================================================================

def setup_bake_images(cfg):
    """Create target images for baking."""
    res = cfg["bake_resolution"]
    images = {}

    # Heightmap (EXR 16-bit)
    img_h = bpy.data.images.new("Ironvale_Heightmap", res, res, float_buffer=True)
    img_h.colorspace_settings.name = 'Non-Color'
    images["heightmap"] = img_h

    # Normal map
    img_n = bpy.data.images.new("Ironvale_NormalMap", res, res)
    img_n.colorspace_settings.name = 'Non-Color'
    images["normal"] = img_n

    # AO map
    img_ao = bpy.data.images.new("Ironvale_AO", res, res)
    img_ao.colorspace_settings.name = 'Non-Color'
    images["ao"] = img_ao

    # Diffuse/Albedo
    img_diff = bpy.data.images.new("Ironvale_Diffuse", res, res)
    images["diffuse"] = img_diff

    # Roughness
    img_rough = bpy.data.images.new("Ironvale_Roughness", res, res)
    img_rough.colorspace_settings.name = 'Non-Color'
    images["roughness"] = img_rough

    print(f"[IRONVALE] Bake images created at {res}x{res}.")
    return images


def bake_terrain_textures(terrain_obj, images, cfg):
    """
    Bake terrain textures using Cycles.
    NOTE: This can take several minutes at 4096px.
    """
    print("[IRONVALE] Starting texture bake (this may take a while)...")

    # Switch to Cycles for baking
    scene = bpy.context.scene
    original_engine = scene.render.engine
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = cfg["bake_samples"]
    scene.cycles.device = 'GPU' if bpy.context.preferences.addons.get('cycles') else 'CPU'

    # Select terrain
    bpy.ops.object.select_all(action='DESELECT')
    terrain_obj.select_set(True)
    bpy.context.view_layer.objects.active = terrain_obj

    margin = cfg["bake_margin"]

    # Add image texture node to material for bake target
    mat = terrain_obj.data.materials[0] if terrain_obj.data.materials else None
    if mat is None:
        print("[WARNING] No material on terrain, skipping bake.")
        return

    node_tree = mat.node_tree
    bake_node = node_tree.nodes.new('ShaderNodeTexImage')
    bake_node.location = (-800, -600)

    # Bake diffuse
    print("[IRONVALE]   Baking diffuse...")
    bake_node.image = images["diffuse"]
    bake_node.select = True
    node_tree.nodes.active = bake_node
    bpy.ops.object.bake(type='DIFFUSE', margin=margin,
                        pass_filter={'COLOR'})

    # Bake normals
    print("[IRONVALE]   Baking normals...")
    bake_node.image = images["normal"]
    bpy.ops.object.bake(type='NORMAL', margin=margin)

    # Bake AO
    print("[IRONVALE]   Baking AO...")
    bake_node.image = images["ao"]
    bpy.ops.object.bake(type='AO', margin=margin)

    # Bake roughness
    print("[IRONVALE]   Baking roughness...")
    bake_node.image = images["roughness"]
    bpy.ops.object.bake(type='ROUGHNESS', margin=margin)

    # Clean up
    node_tree.nodes.remove(bake_node)
    scene.render.engine = original_engine

    print("[IRONVALE] Texture baking complete.")


# ============================================================================
# MAIN
# ============================================================================

def main():
    cfg = CONFIG

    print("=" * 60)
    print("  IRONVALE MATERIALS & BAKING v1.0")
    print("=" * 60)

    # Create and assign all materials
    assign_materials_to_scene()

    # Setup bake images
    images = setup_bake_images(cfg)

    # Bake terrain (optional — comment out if you just want materials)
    terrain = bpy.data.objects.get("Ironvale_Terrain")
    if terrain:
        print("")
        print("[IRONVALE] To bake textures, uncomment the bake call below.")
        print("[IRONVALE] Baking at 4096px can take 5-15 minutes.")
        print("")
        # Uncomment to bake:
        # bake_terrain_textures(terrain, images, cfg)
    else:
        print("[WARNING] Terrain not found — materials created but not assigned to terrain.")

    print("")
    print("=" * 60)
    print("  IRONVALE MATERIALS COMPLETE")
    print("=" * 60)
    print("")
    print("  Materials created:")
    print("    • Ironvale_MuddyClay — Terrain base")
    print("    • Ironvale_Granite — Boulders/rocks")
    print("    • Ironvale_OakBark — Tree trunks")
    print("    • Ironvale_OakLeaf — Tree canopy")
    print("    • Ironvale_AgedTimber — Building frames")
    print("    • Ironvale_Thatch — Roof thatch")
    print("    • Ironvale_Water — River surface")
    print("    • Ironvale_Stone — Bridge/walls")
    print("    • Ironvale_Grass — Wildgrass blades")
    print("")
    print("  Bake images (in Image Editor):")
    print("    • Ironvale_Heightmap (EXR 16-bit float)")
    print("    • Ironvale_NormalMap, Ironvale_AO")
    print("    • Ironvale_Diffuse, Ironvale_Roughness")
    print("")
    print("  To bake: edit this script and uncomment bake_terrain_textures() call.")
    print("  NEXT: Run ironvale_export.py for FBX/texture export.")
    print("=" * 60)


if __name__ == "__main__":
    main()
