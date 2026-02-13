"""
IRONVALE Terrain Generator — Blender 4.2+ Python Script
========================================================
Generates a 5km² realistic medieval valley with:
  - Ridged multifractal hills via Geometry Nodes
  - Central winding river (25m wide, 8m deep)
  - Muddy path network connecting village sites
  - LOD-ready chunked topology for UE5 Landscape import

Usage:
  1. Open Blender 4.2+
  2. Switch to Scripting workspace
  3. Open this file → Run Script
  4. Terrain appears at world origin

Seed: 1337 (deterministic). Modify CONFIG dict to tweak.

Author: IRONVALE Asset Pipeline
"""

import bpy
import bmesh
import math
import random
from mathutils import Vector, noise, kdtree

# ============================================================================
# CONFIGURATION — Tweak these values to reshape the world
# ============================================================================
CONFIG = {
    # World dimensions
    "seed": 1337,
    "terrain_size": 5000.0,        # 5km square
    "terrain_subdivisions": 512,    # Vertex resolution per axis
    "chunk_grid": 8,                # 8x8 = 64 chunks for UE5 streaming

    # Height layers
    "base_height": 0.0,
    "max_hill_height": 280.0,      # Ridge peaks
    "valley_floor": 20.0,          # Minimum valley elevation
    "foothill_blend": 0.6,         # How far foothills extend into valley

    # Ridged multifractal noise
    "ridge_octaves": 6,
    "ridge_lacunarity": 2.1,
    "ridge_gain": 0.45,
    "ridge_offset": 1.0,
    "ridge_frequency": 0.0008,     # Controls hill spacing

    # Secondary detail noise
    "detail_frequency": 0.003,
    "detail_amplitude": 15.0,
    "detail_octaves": 4,

    # Erosion simulation (thermal)
    "erosion_iterations": 3,
    "erosion_talus": 0.6,          # Max slope angle before erosion
    "erosion_strength": 0.35,

    # River
    "river_width": 25.0,
    "river_depth": 8.0,
    "river_meander_freq": 0.0006,
    "river_meander_amp": 400.0,    # How much the river curves
    "river_bank_falloff": 15.0,    # Soft bank transition width

    # Paths
    "path_width": 3.5,
    "path_depth": 0.4,
    "path_falloff": 2.0,

    # Village site positions (normalized 0-1 within terrain)
    "village_sites": [
        (0.35, 0.30),  # Village A — western valley
        (0.60, 0.55),  # Village B — central crossing
        (0.45, 0.78),  # Village C — southern riverside
    ],

    # Farm strip positions (normalized)
    "farm_sites": [
        (0.30, 0.35), (0.38, 0.25), (0.55, 0.50),
        (0.62, 0.60), (0.48, 0.72), (0.40, 0.82),
        (0.52, 0.40), (0.58, 0.68),
    ],

    # Bridge location (normalized)
    "bridge_site": (0.52, 0.50),

    # Watermill location (normalized)
    "watermill_site": (0.50, 0.62),

    # Biome mask (for future swamp/forest expansion)
    "biome_id": "valley",
}


# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def set_seed(seed):
    """Set deterministic seed for all random operations."""
    random.seed(seed)


def norm_to_world(nx, ny, size):
    """Convert normalized (0-1) coords to world coords centered at origin."""
    return (nx - 0.5) * size, (ny - 0.5) * size


def world_to_norm(wx, wy, size):
    """Convert world coords to normalized (0-1)."""
    return wx / size + 0.5, wy / size + 0.5


def ridged_multifractal(pos, octaves, lacunarity, gain, offset, frequency):
    """
    Compute ridged multifractal noise at a 2D position.
    Attempt to use Blender's noise module with manual ridge folding.
    """
    x, y = pos[0] * frequency, pos[1] * frequency
    value = 0.0
    weight = 1.0
    amplitude = 1.0
    freq = 1.0

    for _ in range(octaves):
        # Sample Perlin noise, fold into ridges
        signal = noise.noise(Vector((x * freq, y * freq, 0.0)))
        signal = offset - abs(signal)
        signal *= signal  # Sharpen ridges
        signal *= weight

        weight = max(0.0, min(1.0, signal * gain))
        value += signal * amplitude
        amplitude *= gain
        freq *= lacunarity

    return value


def fbm_noise(pos, octaves, frequency, amplitude):
    """Standard fractal Brownian motion for detail."""
    x, y = pos[0] * frequency, pos[1] * frequency
    value = 0.0
    amp = amplitude
    freq = 1.0

    for _ in range(octaves):
        value += noise.noise(Vector((x * freq, y * freq, 1.5))) * amp
        amp *= 0.5
        freq *= 2.0

    return value


def river_distance(wx, wy, cfg):
    """
    Compute signed distance to the river centerline.
    River runs roughly north-south with Perlin meander.
    """
    size = cfg["terrain_size"]
    # River runs along Y axis, centered at X ≈ 0 with meander
    meander = noise.noise(
        Vector((wy * cfg["river_meander_freq"], 0.3, 2.0))
    ) * cfg["river_meander_amp"]

    # Secondary smaller wiggles
    meander += noise.noise(
        Vector((wy * cfg["river_meander_freq"] * 3.0, 0.7, 2.0))
    ) * cfg["river_meander_amp"] * 0.2

    center_x = meander  # River center X at this Y
    dist = abs(wx - center_x)
    return dist


def river_profile(dist, cfg):
    """
    Compute river depth at a given distance from centerline.
    Returns negative value (carving depth) or 0.
    """
    half_w = cfg["river_width"] * 0.5
    falloff = cfg["river_bank_falloff"]
    depth = cfg["river_depth"]

    if dist < half_w:
        # Inside river — full depth with parabolic bed
        t = dist / half_w
        return -depth * (1.0 - t * t * 0.3)
    elif dist < half_w + falloff:
        # Bank transition — smooth falloff
        t = (dist - half_w) / falloff
        t = t * t * (3.0 - 2.0 * t)  # Smoothstep
        return -depth * (1.0 - t) * 0.3
    return 0.0


def valley_mask(wx, wy, cfg):
    """
    Create a mask that places hills on edges, valley in center.
    Returns 0 = valley floor, 1 = full hill height.
    """
    size = cfg["terrain_size"]
    half = size * 0.5

    # Distance from center, normalized
    dx = abs(wx) / half
    dy = abs(wy) / half
    edge_dist = max(dx, dy)

    # Blend zone: foothills transition
    blend = cfg["foothill_blend"]
    if edge_dist < blend:
        return 0.0
    elif edge_dist < 1.0:
        t = (edge_dist - blend) / (1.0 - blend)
        # Smooth cubic ramp
        return t * t * (3.0 - 2.0 * t)
    return 1.0


def compute_path_segments(cfg):
    """
    Build a path network connecting villages, farms, and the bridge.
    Returns list of (start_world, end_world) tuples.
    """
    size = cfg["terrain_size"]
    segments = []

    # All points of interest in world coords
    pois = []
    for vx, vy in cfg["village_sites"]:
        pois.append(norm_to_world(vx, vy, size))
    for fx, fy in cfg["farm_sites"]:
        pois.append(norm_to_world(fx, fy, size))

    bridge = norm_to_world(*cfg["bridge_site"], size)
    mill = norm_to_world(*cfg["watermill_site"], size)
    pois.append(bridge)
    pois.append(mill)

    # Connect villages to each other
    villages_w = [norm_to_world(v[0], v[1], size) for v in cfg["village_sites"]]
    for i in range(len(villages_w)):
        for j in range(i + 1, len(villages_w)):
            segments.append((villages_w[i], villages_w[j]))

    # Connect each farm to nearest village
    for fx, fy in cfg["farm_sites"]:
        fw = norm_to_world(fx, fy, size)
        nearest = min(villages_w, key=lambda v: (v[0]-fw[0])**2 + (v[1]-fw[1])**2)
        segments.append((fw, nearest))

    # Connect bridge and mill to nearest village
    for poi in [bridge, mill]:
        nearest = min(villages_w, key=lambda v: (v[0]-poi[0])**2 + (v[1]-poi[1])**2)
        segments.append((poi, nearest))

    return segments


def path_distance(wx, wy, segments):
    """
    Compute minimum distance from world point to any path segment.
    """
    min_dist = float('inf')
    p = Vector((wx, wy))

    for (ax, ay), (bx, by) in segments:
        a = Vector((ax, ay))
        b = Vector((bx, by))
        ab = b - a
        ab_len_sq = ab.length_squared
        if ab_len_sq < 0.001:
            dist = (p - a).length
        else:
            t = max(0.0, min(1.0, (p - a).dot(ab) / ab_len_sq))
            closest = a + ab * t
            dist = (p - closest).length
        if dist < min_dist:
            min_dist = dist

    return min_dist


def path_carve(dist, cfg):
    """Compute path depression depth based on distance from path center."""
    half_w = cfg["path_width"] * 0.5
    falloff = cfg["path_falloff"]
    depth = cfg["path_depth"]

    if dist < half_w:
        return -depth
    elif dist < half_w + falloff:
        t = (dist - half_w) / falloff
        t = t * t * (3.0 - 2.0 * t)
        return -depth * (1.0 - t)
    return 0.0


# ============================================================================
# TERRAIN MESH GENERATION
# ============================================================================

def generate_terrain_heightfield(cfg):
    """
    Compute the full heightfield as a 2D list.
    Returns (heights, x_coords, y_coords) where heights[iy][ix] = z.
    """
    set_seed(cfg["seed"])

    size = cfg["terrain_size"]
    res = cfg["terrain_subdivisions"] + 1  # Vertex count per axis
    step = size / cfg["terrain_subdivisions"]
    half = size * 0.5

    # Precompute path segments
    path_segs = compute_path_segments(cfg)

    print(f"[IRONVALE] Generating {res}x{res} heightfield ({res*res} vertices)...")

    heights = []
    x_coords = []
    y_coords = []

    for iy in range(res):
        row = []
        wy = -half + iy * step

        if iy == 0:
            x_coords = [-half + ix * step for ix in range(res)]
        y_coords.append(wy)

        for ix in range(res):
            wx = x_coords[ix]

            # 1) Valley mask — hills at edges, flat center
            vmask = valley_mask(wx, wy, cfg)

            # 2) Ridged multifractal for hills
            ridge_h = ridged_multifractal(
                (wx, wy),
                cfg["ridge_octaves"],
                cfg["ridge_lacunarity"],
                cfg["ridge_gain"],
                cfg["ridge_offset"],
                cfg["ridge_frequency"],
            )
            ridge_h = max(0.0, ridge_h) * cfg["max_hill_height"] * vmask

            # 3) Valley floor gentle undulation
            valley_h = cfg["valley_floor"]
            valley_h += fbm_noise(
                (wx, wy), 3, 0.001, 8.0
            ) * (1.0 - vmask)

            # 4) Detail noise everywhere
            detail_h = fbm_noise(
                (wx, wy),
                cfg["detail_octaves"],
                cfg["detail_frequency"],
                cfg["detail_amplitude"],
            )

            # 5) Combine
            z = ridge_h + valley_h + detail_h

            # 6) River carving
            r_dist = river_distance(wx, wy, cfg)
            r_carve = river_profile(r_dist, cfg)
            z += r_carve

            # 7) Path carving
            p_dist = path_distance(wx, wy, path_segs)
            p_carve = path_carve(p_dist, cfg)
            z += p_carve

            # Clamp
            z = max(-cfg["river_depth"], z)

            row.append(z)

        heights.append(row)

        # Progress
        if iy % 64 == 0:
            pct = (iy / res) * 100
            print(f"[IRONVALE]   {pct:.0f}% complete...")

    print("[IRONVALE] Heightfield generation complete.")
    return heights, x_coords, y_coords


def apply_thermal_erosion(heights, cfg):
    """
    Simple thermal erosion: material slides from steep slopes to neighbors.
    Softens harsh ridges for more natural appearance.
    """
    res = len(heights)
    talus = cfg["erosion_talus"]
    strength = cfg["erosion_strength"]
    iterations = cfg["erosion_iterations"]
    step = cfg["terrain_size"] / (res - 1)

    print(f"[IRONVALE] Applying thermal erosion ({iterations} passes)...")

    for iteration in range(iterations):
        for iy in range(1, res - 1):
            for ix in range(1, res - 1):
                h = heights[iy][ix]
                neighbors = [
                    (iy - 1, ix), (iy + 1, ix),
                    (iy, ix - 1), (iy, ix + 1),
                ]
                max_diff = 0.0
                total_diff = 0.0
                diffs = []

                for ny, nx in neighbors:
                    diff = h - heights[ny][nx]
                    slope = diff / step
                    if slope > talus:
                        diffs.append((ny, nx, diff))
                        total_diff += diff
                        max_diff = max(max_diff, diff)

                if total_diff > 0:
                    move = max_diff * 0.5 * strength
                    heights[iy][ix] -= move
                    for ny, nx, diff in diffs:
                        share = (diff / total_diff) * move
                        heights[ny][nx] += share

        print(f"[IRONVALE]   Erosion pass {iteration + 1}/{iterations} done.")

    return heights


def create_terrain_mesh(heights, x_coords, y_coords, cfg):
    """
    Build the Blender mesh object from the heightfield.
    """
    res_x = len(x_coords)
    res_y = len(y_coords)

    print(f"[IRONVALE] Building mesh ({res_x}x{res_y})...")

    # Create mesh
    mesh = bpy.data.meshes.new("Ironvale_Terrain")
    bm = bmesh.new()

    # Create vertices
    vert_grid = []
    for iy in range(res_y):
        row = []
        for ix in range(res_x):
            v = bm.verts.new((x_coords[ix], y_coords[iy], heights[iy][ix]))
            row.append(v)
        vert_grid.append(row)

    bm.verts.ensure_lookup_table()

    # Create faces (quads)
    for iy in range(res_y - 1):
        for ix in range(res_x - 1):
            v0 = vert_grid[iy][ix]
            v1 = vert_grid[iy][ix + 1]
            v2 = vert_grid[iy + 1][ix + 1]
            v3 = vert_grid[iy + 1][ix]
            bm.faces.new((v0, v1, v2, v3))

    bm.faces.ensure_lookup_table()

    # UV unwrap — planar projection from top
    uv_layer = bm.loops.layers.uv.new("UVMap")
    size = cfg["terrain_size"]
    half = size * 0.5
    for face in bm.faces:
        for loop in face.loops:
            co = loop.vert.co
            loop[uv_layer].uv = ((co.x + half) / size, (co.y + half) / size)

    # Smooth normals
    for face in bm.faces:
        face.smooth = True

    bm.to_mesh(mesh)
    bm.free()

    mesh.update()

    # Create object
    obj = bpy.data.objects.new("Ironvale_Terrain", mesh)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)

    # Add smooth shading
    bpy.ops.object.shade_smooth()

    print(f"[IRONVALE] Mesh created: {len(mesh.vertices)} verts, {len(mesh.polygons)} faces.")
    return obj


# ============================================================================
# GEOMETRY NODES SETUP — Ridged Hills Modifier (Blender 4.2+)
# ============================================================================

def create_ridge_geonodes(terrain_obj, cfg):
    """
    Create a Geometry Nodes modifier for additional ridge displacement.
    This supplements the pre-baked heightfield with live-tweakable ridges.
    Uses Blender 4.2+ node API.
    """
    print("[IRONVALE] Setting up Geometry Nodes ridge modifier...")

    # Create node group
    ng = bpy.data.node_groups.new("Ironvale_RidgeDisplace", 'GeometryNodeTree')

    # Interface: input geometry, output geometry
    ng.interface.new_socket('Geometry', in_out='INPUT', socket_type='NodeSocketGeometry')
    ng.interface.new_socket('Geometry', in_out='OUTPUT', socket_type='NodeSocketGeometry')

    # Tweakable parameters
    ng.interface.new_socket('Ridge Scale', in_out='INPUT',
                           socket_type='NodeSocketFloat')
    ng.interface.new_socket('Ridge Strength', in_out='INPUT',
                           socket_type='NodeSocketFloat')
    ng.interface.new_socket('Detail Scale', in_out='INPUT',
                           socket_type='NodeSocketFloat')
    ng.interface.new_socket('Detail Strength', in_out='INPUT',
                           socket_type='NodeSocketFloat')

    # Set default values on the interface items
    # Ridge Scale
    ng.interface.items_tree[1].default_value = 200.0
    ng.interface.items_tree[1].min_value = 10.0
    ng.interface.items_tree[1].max_value = 2000.0
    # Ridge Strength
    ng.interface.items_tree[2].default_value = 0.0
    ng.interface.items_tree[2].min_value = 0.0
    ng.interface.items_tree[2].max_value = 100.0
    # Detail Scale
    ng.interface.items_tree[3].default_value = 50.0
    ng.interface.items_tree[3].min_value = 1.0
    ng.interface.items_tree[3].max_value = 500.0
    # Detail Strength
    ng.interface.items_tree[4].default_value = 0.0
    ng.interface.items_tree[4].min_value = 0.0
    ng.interface.items_tree[4].max_value = 50.0

    nodes = ng.nodes
    links = ng.links

    # --- Nodes ---
    # Group Input / Output
    n_in = nodes.new('NodeGroupInput')
    n_in.location = (-800, 0)
    n_out = nodes.new('NodeGroupOutput')
    n_out.location = (600, 0)

    # Position node
    n_pos = nodes.new('GeometryNodeInputPosition')
    n_pos.location = (-800, -200)

    # Noise Texture for ridges
    n_noise_ridge = nodes.new('ShaderNodeTexNoise')
    n_noise_ridge.location = (-400, -100)
    n_noise_ridge.inputs['Detail'].default_value = 6.0
    n_noise_ridge.inputs['Roughness'].default_value = 0.7
    n_noise_ridge.inputs['Distortion'].default_value = 0.8
    n_noise_ridge.noise_dimensions = '3D'

    # Math: Absolute (for ridge folding)
    n_abs = nodes.new('ShaderNodeMath')
    n_abs.location = (-200, -100)
    n_abs.operation = 'ABSOLUTE'

    # Math: Subtract from 1 (invert for ridges)
    n_invert = nodes.new('ShaderNodeMath')
    n_invert.location = (0, -100)
    n_invert.operation = 'SUBTRACT'
    n_invert.inputs[0].default_value = 1.0

    # Math: Power (sharpen ridges)
    n_power = nodes.new('ShaderNodeMath')
    n_power.location = (100, -100)
    n_power.operation = 'POWER'
    n_power.inputs[1].default_value = 2.0

    # Math: Multiply by ridge strength
    n_mul_ridge = nodes.new('ShaderNodeMath')
    n_mul_ridge.location = (200, -100)
    n_mul_ridge.operation = 'MULTIPLY'

    # Noise Texture for detail
    n_noise_detail = nodes.new('ShaderNodeTexNoise')
    n_noise_detail.location = (-400, -400)
    n_noise_detail.inputs['Detail'].default_value = 4.0
    n_noise_detail.inputs['Roughness'].default_value = 0.5
    n_noise_detail.noise_dimensions = '3D'

    # Math: Multiply detail
    n_mul_detail = nodes.new('ShaderNodeMath')
    n_mul_detail.location = (0, -400)
    n_mul_detail.operation = 'MULTIPLY'

    # Math: Add ridge + detail
    n_add = nodes.new('ShaderNodeMath')
    n_add.location = (300, -200)
    n_add.operation = 'ADD'

    # Combine XYZ for displacement vector (Z only)
    n_combine = nodes.new('ShaderNodeCombineXYZ')
    n_combine.location = (400, -200)
    n_combine.inputs['X'].default_value = 0.0
    n_combine.inputs['Y'].default_value = 0.0

    # Set Position
    n_setpos = nodes.new('GeometryNodeSetPosition')
    n_setpos.location = (500, 0)

    # --- Links ---
    # Ridge noise chain
    links.new(n_pos.outputs['Position'], n_noise_ridge.inputs['Vector'])
    links.new(n_in.outputs['Ridge Scale'], n_noise_ridge.inputs['Scale'])
    links.new(n_noise_ridge.outputs['Fac'], n_abs.inputs[0])
    links.new(n_abs.outputs[0], n_invert.inputs[1])
    links.new(n_invert.outputs[0], n_power.inputs[0])
    links.new(n_power.outputs[0], n_mul_ridge.inputs[0])
    links.new(n_in.outputs['Ridge Strength'], n_mul_ridge.inputs[1])

    # Detail noise chain
    links.new(n_pos.outputs['Position'], n_noise_detail.inputs['Vector'])
    links.new(n_in.outputs['Detail Scale'], n_noise_detail.inputs['Scale'])
    links.new(n_noise_detail.outputs['Fac'], n_mul_detail.inputs[0])
    links.new(n_in.outputs['Detail Strength'], n_mul_detail.inputs[1])

    # Combine
    links.new(n_mul_ridge.outputs[0], n_add.inputs[0])
    links.new(n_mul_detail.outputs[0], n_add.inputs[1])
    links.new(n_add.outputs[0], n_combine.inputs['Z'])

    # Set position
    links.new(n_in.outputs['Geometry'], n_setpos.inputs['Geometry'])
    links.new(n_combine.outputs['Vector'], n_setpos.inputs['Offset'])
    links.new(n_setpos.outputs['Geometry'], n_out.inputs['Geometry'])

    # Apply modifier to terrain
    mod = terrain_obj.modifiers.new("Ironvale_Ridges", 'NODES')
    mod.node_group = ng

    print("[IRONVALE] Geometry Nodes ridge modifier created.")
    print("[IRONVALE]   → Ridge Strength defaults to 0 (heightfield already has ridges).")
    print("[IRONVALE]   → Increase Ridge/Detail Strength to add live displacement on top.")
    return ng


# ============================================================================
# VERTEX GROUPS & ATTRIBUTES — For scatter/material masking
# ============================================================================

def create_terrain_attributes(terrain_obj, heights, cfg):
    """
    Bake useful vertex attributes for downstream scripts:
      - slope: Normalized slope (0=flat, 1=cliff)
      - river_mask: 1.0 inside river, 0 outside
      - path_mask: 1.0 on paths, 0 outside
      - valley_mask: 0=valley floor, 1=hilltop
      - height_normalized: 0-1 height range
    """
    print("[IRONVALE] Computing vertex attributes (slope, masks)...")

    mesh = terrain_obj.data
    size = cfg["terrain_size"]
    half = size * 0.5
    res = cfg["terrain_subdivisions"] + 1
    step = size / cfg["terrain_subdivisions"]

    path_segs = compute_path_segments(cfg)

    # Create vertex groups
    vg_slope = terrain_obj.vertex_groups.new(name="slope")
    vg_river = terrain_obj.vertex_groups.new(name="river_mask")
    vg_path = terrain_obj.vertex_groups.new(name="path_mask")
    vg_valley = terrain_obj.vertex_groups.new(name="valley_mask")
    vg_height = terrain_obj.vertex_groups.new(name="height_normalized")

    # Find height range for normalization
    min_h = float('inf')
    max_h = float('-inf')
    for row in heights:
        for h in row:
            min_h = min(min_h, h)
            max_h = max(max_h, h)
    h_range = max_h - min_h if max_h > min_h else 1.0

    for iy in range(res):
        for ix in range(res):
            vi = iy * res + ix
            wx = -half + ix * step
            wy = -half + iy * step
            h = heights[iy][ix]

            # Slope from finite differences
            if 0 < ix < res - 1 and 0 < iy < res - 1:
                dzdx = (heights[iy][ix + 1] - heights[iy][ix - 1]) / (2.0 * step)
                dzdy = (heights[iy + 1][ix] - heights[iy - 1][ix]) / (2.0 * step)
                slope = math.sqrt(dzdx * dzdx + dzdy * dzdy)
                slope = min(1.0, slope / 2.0)  # Normalize: slope=2 → 1.0
            else:
                slope = 0.0

            # River mask
            r_dist = river_distance(wx, wy, cfg)
            r_half = cfg["river_width"] * 0.5 + cfg["river_bank_falloff"]
            river_val = max(0.0, 1.0 - r_dist / r_half) if r_half > 0 else 0.0

            # Path mask
            p_dist = path_distance(wx, wy, path_segs)
            p_half = cfg["path_width"] * 0.5 + cfg["path_falloff"]
            path_val = max(0.0, 1.0 - p_dist / p_half) if p_half > 0 else 0.0

            # Valley mask
            v_mask = valley_mask(wx, wy, cfg)

            # Height normalized
            h_norm = (h - min_h) / h_range

            # Assign
            vg_slope.add([vi], slope, 'REPLACE')
            vg_river.add([vi], river_val, 'REPLACE')
            vg_path.add([vi], path_val, 'REPLACE')
            vg_valley.add([vi], v_mask, 'REPLACE')
            vg_height.add([vi], h_norm, 'REPLACE')

    print("[IRONVALE] Vertex attributes created: slope, river_mask, path_mask, valley_mask, height_normalized.")


# ============================================================================
# WATER PLANE
# ============================================================================

def create_water_plane(cfg):
    """Create a flat water surface at river level."""
    print("[IRONVALE] Creating water plane...")

    size = cfg["terrain_size"]

    bpy.ops.mesh.primitive_plane_add(size=size, location=(0, 0, -0.5))
    water = bpy.context.active_object
    water.name = "Ironvale_WaterPlane"

    # Subdivide for wave displacement later
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.subdivide(number_cuts=64)
    bpy.ops.object.mode_set(mode='OBJECT')

    # UV unwrap
    mesh = water.data
    bm = bmesh.new()
    bm.from_mesh(mesh)
    uv_layer = bm.loops.layers.uv.verify()
    half = size * 0.5
    for face in bm.faces:
        for loop in face.loops:
            co = loop.vert.co
            loop[uv_layer].uv = ((co.x + half) / size, (co.y + half) / size)
    bm.to_mesh(mesh)
    bm.free()

    print("[IRONVALE] Water plane created at Z=-0.5.")
    return water


# ============================================================================
# SCENE SETUP
# ============================================================================

def setup_scene():
    """Clean the scene and set up units/scale for real-world meters."""
    print("[IRONVALE] Setting up scene...")

    # Delete all existing objects
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

    # Set units to metric (meters)
    scene = bpy.context.scene
    scene.unit_settings.system = 'METRIC'
    scene.unit_settings.scale_length = 1.0
    scene.unit_settings.length_unit = 'METERS'

    # Set clip distances for large terrain
    for area in bpy.context.screen.areas:
        if area.type == 'VIEW_3D':
            for space in area.spaces:
                if space.type == 'VIEW_3D':
                    space.clip_start = 1.0
                    space.clip_end = 50000.0

    # Set world background
    world = bpy.data.worlds.get("World")
    if world is None:
        world = bpy.data.worlds.new("World")
    scene.world = world
    world.use_nodes = True

    print("[IRONVALE] Scene ready (metric meters, 50km clip).")


# ============================================================================
# CHUNK SPLITTING (for UE5 streaming levels)
# ============================================================================

def create_chunk_empties(cfg):
    """
    Create empty markers at chunk boundaries for the export script.
    The export script will use these to split the terrain into tiles.
    """
    grid = cfg["chunk_grid"]
    size = cfg["terrain_size"]
    chunk_size = size / grid
    half = size * 0.5

    print(f"[IRONVALE] Marking {grid}x{grid} chunk grid ({chunk_size:.0f}m chunks)...")

    # Create collection for chunk markers
    col = bpy.data.collections.new("Ironvale_ChunkGrid")
    bpy.context.scene.collection.children.link(col)

    for cy in range(grid):
        for cx in range(grid):
            x = -half + (cx + 0.5) * chunk_size
            y = -half + (cy + 0.5) * chunk_size
            empty = bpy.data.objects.new(f"Chunk_{cx}_{cy}", None)
            empty.location = (x, y, 0)
            empty.empty_display_type = 'CUBE'
            empty.empty_display_size = chunk_size * 0.5
            empty["chunk_x"] = cx
            empty["chunk_y"] = cy
            empty["chunk_size"] = chunk_size
            col.objects.link(empty)

    print(f"[IRONVALE] Chunk grid created ({grid*grid} chunks).")


# ============================================================================
# POI MARKERS — For structure placement scripts
# ============================================================================

def create_poi_markers(cfg):
    """
    Place empties at village, farm, bridge, and watermill locations.
    The structures script reads these for placement.
    """
    size = cfg["terrain_size"]
    print("[IRONVALE] Placing POI markers...")

    col = bpy.data.collections.new("Ironvale_POIs")
    bpy.context.scene.collection.children.link(col)

    # Villages
    for i, (vx, vy) in enumerate(cfg["village_sites"]):
        wx, wy = norm_to_world(vx, vy, size)
        empty = bpy.data.objects.new(f"Village_{i}", None)
        empty.location = (wx, wy, 0)
        empty.empty_display_type = 'PLAIN_AXES'
        empty.empty_display_size = 50.0
        empty["poi_type"] = "village"
        empty["poi_index"] = i
        empty["population"] = 45
        col.objects.link(empty)

    # Farms
    for i, (fx, fy) in enumerate(cfg["farm_sites"]):
        wx, wy = norm_to_world(fx, fy, size)
        empty = bpy.data.objects.new(f"Farm_{i}", None)
        empty.location = (wx, wy, 0)
        empty.empty_display_type = 'SINGLE_ARROW'
        empty.empty_display_size = 30.0
        empty["poi_type"] = "farm"
        empty["poi_index"] = i
        col.objects.link(empty)

    # Bridge
    bx, by = norm_to_world(*cfg["bridge_site"], size)
    empty = bpy.data.objects.new("Bridge_0", None)
    empty.location = (bx, by, 0)
    empty.empty_display_type = 'ARROWS'
    empty.empty_display_size = 20.0
    empty["poi_type"] = "bridge"
    col.objects.link(empty)

    # Watermill
    mx, my = norm_to_world(*cfg["watermill_site"], size)
    empty = bpy.data.objects.new("Watermill_0", None)
    empty.location = (mx, my, 0)
    empty.empty_display_type = 'ARROWS'
    empty.empty_display_size = 15.0
    empty["poi_type"] = "watermill"
    col.objects.link(empty)

    print(f"[IRONVALE] POI markers placed: {len(cfg['village_sites'])} villages, "
          f"{len(cfg['farm_sites'])} farms, 1 bridge, 1 watermill.")


# ============================================================================
# MAIN EXECUTION
# ============================================================================

def main():
    """Run the complete terrain generation pipeline."""
    cfg = CONFIG.copy()

    print("=" * 60)
    print("  IRONVALE TERRAIN GENERATOR v1.0")
    print("  Seed:", cfg["seed"])
    print("  Size:", cfg["terrain_size"], "m")
    print("  Resolution:", cfg["terrain_subdivisions"] + 1, "vertices/axis")
    print("=" * 60)

    # Step 1: Scene setup
    setup_scene()

    # Step 2: Generate heightfield
    heights, x_coords, y_coords = generate_terrain_heightfield(cfg)

    # Step 3: Thermal erosion
    heights = apply_thermal_erosion(heights, cfg)

    # Step 4: Build mesh
    terrain_obj = create_terrain_mesh(heights, x_coords, y_coords, cfg)

    # Step 5: Vertex attributes for masking
    create_terrain_attributes(terrain_obj, heights, cfg)

    # Step 6: Geometry Nodes modifier (live-tweakable)
    create_ridge_geonodes(terrain_obj, cfg)

    # Step 7: Water plane
    create_water_plane(cfg)

    # Step 8: Chunk grid empties
    create_chunk_empties(cfg)

    # Step 9: POI markers
    create_poi_markers(cfg)

    # Final camera framing
    bpy.ops.object.select_all(action='DESELECT')
    terrain_obj.select_set(True)
    bpy.context.view_layer.objects.active = terrain_obj

    print("")
    print("=" * 60)
    print("  IRONVALE TERRAIN GENERATION COMPLETE")
    print("=" * 60)
    print("")
    print("  Objects created:")
    print("    • Ironvale_Terrain — Main terrain mesh with vertex groups")
    print("    • Ironvale_WaterPlane — River/lake water surface")
    print("    • Ironvale_ChunkGrid — 64 chunk empties for export")
    print("    • Ironvale_POIs — Village/farm/bridge/mill markers")
    print("")
    print("  Vertex groups on terrain:")
    print("    • slope — Surface steepness (0=flat, 1=cliff)")
    print("    • river_mask — River channel mask")
    print("    • path_mask — Footpath network mask")
    print("    • valley_mask — Valley(0) to hilltop(1) gradient")
    print("    • height_normalized — 0-1 elevation range")
    print("")
    print("  Geometry Nodes modifier: 'Ironvale_Ridges'")
    print("    → Adjust Ridge/Detail Strength to add live displacement")
    print("    → Defaults to 0 (pre-baked heightfield is primary)")
    print("")
    print("  NEXT: Run ironvale_scatter.py to populate vegetation.")
    print("=" * 60)


# Run when executed as script
if __name__ == "__main__":
    main()
