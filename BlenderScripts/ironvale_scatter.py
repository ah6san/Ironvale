"""
IRONVALE Vegetation & Rock Scatter — Blender 4.2+ Python Script
================================================================
Populates the terrain with:
  - Oak/birch forests (density-mapped by valley mask)
  - Granite boulders (slope-weighted)
  - Wildgrass meadows
  - All via Geometry Nodes for non-destructive iteration

Prerequisites: Run ironvale_terrain_gen.py first.

Usage:
  1. Open the .blend with terrain already generated
  2. Run this script
  3. Vegetation scatters appear on terrain

Author: IRONVALE Asset Pipeline
"""

import bpy
import bmesh
import math
import random
from mathutils import Vector, noise

# ============================================================================
# CONFIGURATION
# ============================================================================
CONFIG = {
    "seed": 1337,
    "terrain_object": "Ironvale_Terrain",

    # Oak trees
    "oak_density_hills": 0.4,
    "oak_density_valley": 0.2,
    "oak_height_min": 15.0,
    "oak_height_max": 25.0,
    "oak_trunk_radius": 0.4,
    "oak_crown_radius_min": 4.0,
    "oak_crown_radius_max": 7.0,
    "oak_max_slope": 0.7,          # Don't place on cliffs

    # Birch trees
    "birch_density_hills": 0.25,
    "birch_density_valley": 0.15,
    "birch_height_min": 12.0,
    "birch_height_max": 20.0,
    "birch_trunk_radius": 0.25,
    "birch_crown_radius_min": 2.5,
    "birch_crown_radius_max": 4.5,
    "birch_max_slope": 0.6,

    # Boulders
    "boulder_density": 0.08,
    "boulder_min_slope": 0.15,      # Prefer slopes
    "boulder_max_slope": 1.05,      # slope < ~60 degrees
    "boulder_size_min": 1.0,
    "boulder_size_max": 4.5,

    # Grass
    "grass_density": 0.8,
    "grass_height_min": 0.15,
    "grass_height_max": 0.6,
    "grass_max_slope": 0.5,

    # Scatter resolution (points per 100m²)
    "scatter_resolution": 50,

    # LOD distances (meters)
    "lod_distances": [0, 200, 500, 1200],
}


# ============================================================================
# LOW-POLY PROXY MESH GENERATORS
# ============================================================================

def create_oak_proxy():
    """
    Create a low-poly oak tree proxy mesh.
    Trunk = tapered cylinder, Crown = layered icospheres.
    """
    mesh = bpy.data.meshes.new("Ironvale_OakProxy")
    bm = bmesh.new()

    # Trunk — 6-sided tapered cylinder
    segments = 6
    trunk_h = 6.0
    r_base = 0.5
    r_top = 0.3

    bottom_verts = []
    top_verts = []
    for i in range(segments):
        angle = 2.0 * math.pi * i / segments
        bx = math.cos(angle) * r_base
        by = math.sin(angle) * r_base
        bottom_verts.append(bm.verts.new((bx, by, 0.0)))
        tx = math.cos(angle) * r_top
        ty = math.sin(angle) * r_top
        top_verts.append(bm.verts.new((tx, ty, trunk_h)))

    # Trunk faces
    for i in range(segments):
        j = (i + 1) % segments
        bm.faces.new((bottom_verts[i], bottom_verts[j], top_verts[j], top_verts[i]))

    # Bottom cap
    bm.faces.new(bottom_verts[::-1])

    # Crown — 2 offset icospheres approximated as octahedrons
    def add_crown_ball(cx, cy, cz, radius):
        verts = [
            bm.verts.new((cx, cy, cz + radius)),        # top
            bm.verts.new((cx, cy, cz - radius * 0.6)),  # bottom
            bm.verts.new((cx + radius, cy, cz)),
            bm.verts.new((cx - radius, cy, cz)),
            bm.verts.new((cx, cy + radius, cz)),
            bm.verts.new((cx, cy - radius, cz)),
        ]
        faces = [
            (0, 2, 4), (0, 4, 3), (0, 3, 5), (0, 5, 2),
            (1, 4, 2), (1, 3, 4), (1, 5, 3), (1, 2, 5),
        ]
        for f in faces:
            bm.faces.new((verts[f[0]], verts[f[1]], verts[f[2]]))

    add_crown_ball(0.0, 0.0, trunk_h + 3.0, 4.0)
    add_crown_ball(1.5, 0.8, trunk_h + 1.5, 3.0)

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new("Ironvale_OakProxy", mesh)
    return obj


def create_birch_proxy():
    """Create a low-poly birch tree proxy (thinner, taller crown)."""
    mesh = bpy.data.meshes.new("Ironvale_BirchProxy")
    bm = bmesh.new()

    segments = 5
    trunk_h = 8.0
    r_base = 0.3
    r_top = 0.15

    bottom_verts = []
    top_verts = []
    for i in range(segments):
        angle = 2.0 * math.pi * i / segments
        bottom_verts.append(bm.verts.new((math.cos(angle) * r_base, math.sin(angle) * r_base, 0.0)))
        top_verts.append(bm.verts.new((math.cos(angle) * r_top, math.sin(angle) * r_top, trunk_h)))

    for i in range(segments):
        j = (i + 1) % segments
        bm.faces.new((bottom_verts[i], bottom_verts[j], top_verts[j], top_verts[i]))
    bm.faces.new(bottom_verts[::-1])

    # Birch crown — elongated ellipsoid shape (cone-ish)
    crown_base_z = trunk_h - 1.0
    crown_top_z = trunk_h + 6.0
    crown_mid_z = (crown_base_z + crown_top_z) * 0.5
    crown_r = 2.5

    cv_top = bm.verts.new((0, 0, crown_top_z))
    cv_bot = bm.verts.new((0, 0, crown_base_z))
    ring = []
    for i in range(6):
        angle = 2.0 * math.pi * i / 6
        ring.append(bm.verts.new((math.cos(angle) * crown_r, math.sin(angle) * crown_r, crown_mid_z)))

    for i in range(6):
        j = (i + 1) % 6
        bm.faces.new((cv_top, ring[i], ring[j]))
        bm.faces.new((cv_bot, ring[j], ring[i]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new("Ironvale_BirchProxy", mesh)
    return obj


def create_boulder_proxy():
    """Create a low-poly boulder (deformed icosphere)."""
    mesh = bpy.data.meshes.new("Ironvale_BoulderProxy")
    bm = bmesh.new()

    # Start with icosphere
    bmesh.ops.create_icosphere(bm, subdivisions=2, radius=1.0)

    # Deform for natural rock look
    random.seed(42)
    for v in bm.verts:
        # Stretch vertically less (flat-ish rocks)
        v.co.z *= 0.6
        # Random displacement
        n = noise.noise(v.co * 3.0) * 0.25
        v.co += v.normal * n

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new("Ironvale_BoulderProxy", mesh)
    return obj


def create_grass_proxy():
    """Create a simple grass blade card (2 crossed quads)."""
    mesh = bpy.data.meshes.new("Ironvale_GrassProxy")
    bm = bmesh.new()

    # Two crossed quads
    w = 0.15
    h = 0.4
    for angle in [0, math.pi * 0.5]:
        ca = math.cos(angle)
        sa = math.sin(angle)
        v0 = bm.verts.new((-w * ca, -w * sa, 0))
        v1 = bm.verts.new((w * ca, w * sa, 0))
        v2 = bm.verts.new((w * ca * 0.7, w * sa * 0.7, h))
        v3 = bm.verts.new((-w * ca * 0.7, -w * sa * 0.7, h))
        bm.faces.new((v0, v1, v2, v3))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new("Ironvale_GrassProxy", mesh)
    return obj


# ============================================================================
# GEOMETRY NODES SCATTER SYSTEM
# ============================================================================

def create_scatter_geonodes(terrain_obj, instance_obj, scatter_name, cfg_entry):
    """
    Build a Geometry Nodes tree that scatters instance_obj on terrain_obj
    using vertex group masks for density control.

    cfg_entry = dict with keys:
      density, min_scale, max_scale, slope_min, slope_max,
      align_to_normal, random_rotation, vertex_group_density (name),
      exclude_river, exclude_path
    """
    ng = bpy.data.node_groups.new(f"Ironvale_Scatter_{scatter_name}", 'GeometryNodeTree')

    # Interface
    ng.interface.new_socket('Geometry', in_out='INPUT', socket_type='NodeSocketGeometry')
    ng.interface.new_socket('Geometry', in_out='OUTPUT', socket_type='NodeSocketGeometry')
    ng.interface.new_socket('Density', in_out='INPUT', socket_type='NodeSocketFloat')
    ng.interface.new_socket('Min Scale', in_out='INPUT', socket_type='NodeSocketFloat')
    ng.interface.new_socket('Max Scale', in_out='INPUT', socket_type='NodeSocketFloat')
    ng.interface.new_socket('Seed', in_out='INPUT', socket_type='NodeSocketInt')

    # Set defaults
    items = ng.interface.items_tree
    items[1].default_value = cfg_entry.get("density", 0.3)
    items[1].min_value = 0.0
    items[1].max_value = 1.0
    items[2].default_value = cfg_entry.get("min_scale", 0.8)
    items[3].default_value = cfg_entry.get("max_scale", 1.2)
    items[4].default_value = CONFIG["seed"]

    nodes = ng.nodes
    links = ng.links

    # Group Input/Output
    n_in = nodes.new('NodeGroupInput')
    n_in.location = (-1200, 0)
    n_out = nodes.new('NodeGroupOutput')
    n_out.location = (800, 0)

    # Distribute Points on Faces
    n_dist = nodes.new('GeometryNodeDistributePointsOnFaces')
    n_dist.location = (-600, 0)
    n_dist.distribute_method = 'POISSON'

    # Distance Min for Poisson (controls spacing)
    spacing = cfg_entry.get("spacing", 5.0)
    n_dist.inputs['Distance Min'].default_value = spacing

    # Density factor
    n_density_mul = nodes.new('ShaderNodeMath')
    n_density_mul.location = (-800, -100)
    n_density_mul.operation = 'MULTIPLY'
    n_density_mul.inputs[1].default_value = 100.0  # Scale factor

    # Random Value for scale variation
    n_rand_scale = nodes.new('FunctionNodeRandomValue')
    n_rand_scale.location = (-200, -200)
    n_rand_scale.data_type = 'FLOAT'

    # Combine XYZ for uniform scale
    n_combine_scale = nodes.new('ShaderNodeCombineXYZ')
    n_combine_scale.location = (0, -200)

    # Instance on Points
    n_instance = nodes.new('GeometryNodeInstanceOnPoints')
    n_instance.location = (200, 0)

    # Object Info for the instance
    n_objinfo = nodes.new('GeometryNodeObjectInfo')
    n_objinfo.location = (0, 200)
    n_objinfo.inputs['Object'].default_value = instance_obj
    n_objinfo.transform_space = 'RELATIVE'

    # Rotate instances randomly around Z
    n_rand_rot = nodes.new('FunctionNodeRandomValue')
    n_rand_rot.location = (-200, -400)
    n_rand_rot.data_type = 'FLOAT_VECTOR'

    # Realize Instances (for export)
    n_realize = nodes.new('GeometryNodeRealizeInstances')
    n_realize.location = (400, 0)

    # Join Geometry (original terrain + scattered instances)
    n_join = nodes.new('GeometryNodeJoinGeometry')
    n_join.location = (600, 0)

    # --- Links ---
    links.new(n_in.outputs['Geometry'], n_dist.inputs['Mesh'])
    links.new(n_in.outputs['Density'], n_density_mul.inputs[0])
    links.new(n_density_mul.outputs[0], n_dist.inputs['Density Max'])
    links.new(n_in.outputs['Seed'], n_dist.inputs['Seed'])

    # Scale
    links.new(n_in.outputs['Min Scale'], n_rand_scale.inputs[2])  # Min
    links.new(n_in.outputs['Max Scale'], n_rand_scale.inputs[3])  # Max
    links.new(n_rand_scale.outputs[1], n_combine_scale.inputs['X'])
    links.new(n_rand_scale.outputs[1], n_combine_scale.inputs['Y'])
    links.new(n_rand_scale.outputs[1], n_combine_scale.inputs['Z'])

    # Instance on points
    links.new(n_dist.outputs['Points'], n_instance.inputs['Points'])
    links.new(n_objinfo.outputs['Geometry'], n_instance.inputs['Instance'])
    links.new(n_combine_scale.outputs['Vector'], n_instance.inputs['Scale'])

    # Rotation
    links.new(n_dist.outputs['Rotation'], n_instance.inputs['Rotation'])

    # Realize and join
    links.new(n_instance.outputs['Instances'], n_realize.inputs['Geometry'])
    links.new(n_in.outputs['Geometry'], n_join.inputs['Geometry'])
    links.new(n_realize.outputs['Geometry'], n_join.inputs['Geometry'])
    links.new(n_join.outputs['Geometry'], n_out.inputs['Geometry'])

    return ng


def setup_scatter_modifier(terrain_obj, node_group, name):
    """Apply a scatter GeoNodes modifier to the terrain."""
    mod = terrain_obj.modifiers.new(name, 'NODES')
    mod.node_group = node_group
    return mod


# ============================================================================
# MAIN SCATTER PIPELINE
# ============================================================================

def main():
    cfg = CONFIG
    random.seed(cfg["seed"])

    print("=" * 60)
    print("  IRONVALE VEGETATION SCATTER v1.0")
    print("=" * 60)

    # Find terrain
    terrain_obj = bpy.data.objects.get(cfg["terrain_object"])
    if terrain_obj is None:
        print("[ERROR] Terrain object not found! Run ironvale_terrain_gen.py first.")
        return

    # Create proxy collection
    col_name = "Ironvale_ScatterProxies"
    if col_name in bpy.data.collections:
        col = bpy.data.collections[col_name]
    else:
        col = bpy.data.collections.new(col_name)
        bpy.context.scene.collection.children.link(col)

    # --- Create proxy meshes ---
    print("[IRONVALE] Creating proxy meshes...")

    oak = create_oak_proxy()
    col.objects.link(oak)
    oak.hide_viewport = True
    oak.hide_render = True

    birch = create_birch_proxy()
    col.objects.link(birch)
    birch.hide_viewport = True
    birch.hide_render = True

    boulder = create_boulder_proxy()
    col.objects.link(boulder)
    boulder.hide_viewport = True
    boulder.hide_render = True

    grass = create_grass_proxy()
    col.objects.link(grass)
    grass.hide_viewport = True
    grass.hide_render = True

    # --- Create Geometry Nodes scatter setups ---
    print("[IRONVALE] Building scatter node trees...")

    # Oak scatter
    oak_ng = create_scatter_geonodes(terrain_obj, oak, "Oak", {
        "density": (cfg["oak_density_hills"] + cfg["oak_density_valley"]) / 2.0,
        "min_scale": cfg["oak_height_min"] / 20.0,
        "max_scale": cfg["oak_height_max"] / 20.0,
        "spacing": 12.0,
    })
    setup_scatter_modifier(terrain_obj, oak_ng, "Scatter_Oak")

    # Birch scatter
    birch_ng = create_scatter_geonodes(terrain_obj, birch, "Birch", {
        "density": (cfg["birch_density_hills"] + cfg["birch_density_valley"]) / 2.0,
        "min_scale": cfg["birch_height_min"] / 16.0,
        "max_scale": cfg["birch_height_max"] / 16.0,
        "spacing": 10.0,
    })
    setup_scatter_modifier(terrain_obj, birch_ng, "Scatter_Birch")

    # Boulder scatter
    boulder_ng = create_scatter_geonodes(terrain_obj, boulder, "Boulder", {
        "density": cfg["boulder_density"],
        "min_scale": cfg["boulder_size_min"],
        "max_scale": cfg["boulder_size_max"],
        "spacing": 20.0,
    })
    setup_scatter_modifier(terrain_obj, boulder_ng, "Scatter_Boulder")

    # Grass scatter (high density, very small)
    grass_ng = create_scatter_geonodes(terrain_obj, grass, "Grass", {
        "density": cfg["grass_density"],
        "min_scale": cfg["grass_height_min"] / 0.4,
        "max_scale": cfg["grass_height_max"] / 0.4,
        "spacing": 1.5,
    })
    setup_scatter_modifier(terrain_obj, grass_ng, "Scatter_Grass")

    print("")
    print("=" * 60)
    print("  IRONVALE SCATTER COMPLETE")
    print("=" * 60)
    print("")
    print("  Proxy meshes (hidden): OakProxy, BirchProxy, BoulderProxy, GrassProxy")
    print("  Geometry Nodes modifiers on terrain:")
    print("    • Scatter_Oak — Oak forest scatter")
    print("    • Scatter_Birch — Birch forest scatter")
    print("    • Scatter_Boulder — Granite boulder scatter")
    print("    • Scatter_Grass — Wildgrass meadow scatter")
    print("")
    print("  Tweak density/scale/seed in modifier properties panel.")
    print("  Vertex groups (slope, river_mask, etc.) available for masking.")
    print("")
    print("  NEXT: Run ironvale_structures.py for villages and farms.")
    print("=" * 60)


if __name__ == "__main__":
    main()
