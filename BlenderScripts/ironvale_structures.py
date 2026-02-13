"""
IRONVALE Structure Generator — Blender 4.2+
=============================================
Auto-places: 3 villages (45 pop, thatch-timber houses, palisades),
8 strip farms (wheat fields, stone boundaries), 1 stone bridge, 1 watermill.

Prerequisites: Run ironvale_terrain_gen.py first (reads POI markers).
"""

import bpy
import bmesh
import math
import random
from mathutils import Vector

CONFIG = {
    "seed": 1337,
    "terrain_object": "Ironvale_Terrain",

    # Village
    "houses_per_village": 12,
    "house_width": (5.0, 8.0),
    "house_depth": (6.0, 10.0),
    "house_wall_height": (2.8, 3.5),
    "house_roof_pitch": (0.6, 0.9),
    "palisade_radius": 60.0,
    "palisade_height": 3.5,
    "palisade_post_spacing": 2.0,
    "palisade_post_radius": 0.15,

    # Farm
    "farm_length": 80.0,
    "farm_width": 25.0,
    "farm_furrow_spacing": 0.8,
    "boundary_wall_height": 0.9,
    "boundary_wall_width": 0.5,

    # Bridge
    "bridge_length": 30.0,
    "bridge_width": 4.5,
    "bridge_arch_height": 3.0,
    "bridge_thickness": 1.2,

    # Watermill
    "mill_body_width": 6.0,
    "mill_body_depth": 8.0,
    "mill_body_height": 4.5,
    "wheel_radius": 3.0,
    "wheel_width": 1.0,
    "wheel_paddles": 12,
}


# ============================================================================
# BUILDING GENERATORS
# ============================================================================

def create_timber_house(name, width, depth, wall_h, roof_pitch):
    """Thatch-timber house: box walls + pitched roof."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    hw, hd = width / 2, depth / 2
    ridge_h = wall_h + roof_pitch * hw

    # Wall vertices (box)
    v = [
        bm.verts.new((-hw, -hd, 0)),      # 0 front-left
        bm.verts.new((hw, -hd, 0)),       # 1 front-right
        bm.verts.new((hw, hd, 0)),        # 2 back-right
        bm.verts.new((-hw, hd, 0)),       # 3 back-left
        bm.verts.new((-hw, -hd, wall_h)), # 4
        bm.verts.new((hw, -hd, wall_h)),  # 5
        bm.verts.new((hw, hd, wall_h)),   # 6
        bm.verts.new((-hw, hd, wall_h)),  # 7
        # Ridge vertices
        bm.verts.new((0, -hd, ridge_h)),  # 8 front ridge
        bm.verts.new((0, hd, ridge_h)),   # 9 back ridge
    ]

    # Floor
    bm.faces.new((v[0], v[3], v[2], v[1]))
    # Front wall
    bm.faces.new((v[0], v[1], v[5], v[4]))
    # Back wall
    bm.faces.new((v[2], v[3], v[7], v[6]))
    # Left wall
    bm.faces.new((v[3], v[0], v[4], v[7]))
    # Right wall
    bm.faces.new((v[1], v[2], v[6], v[5]))
    # Front gable
    bm.faces.new((v[4], v[5], v[8]))
    # Back gable
    bm.faces.new((v[6], v[7], v[9]))
    # Roof left
    bm.faces.new((v[7], v[4], v[8], v[9]))
    # Roof right
    bm.faces.new((v[5], v[6], v[9], v[8]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


def create_palisade(name, radius, height, spacing, post_r):
    """Circular wooden palisade from vertical posts."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    circumference = 2.0 * math.pi * radius
    num_posts = int(circumference / spacing)

    for i in range(num_posts):
        angle = 2.0 * math.pi * i / num_posts
        cx = math.cos(angle) * radius
        cy = math.sin(angle) * radius

        # Each post: 4-sided prism
        segs = 4
        bottom = []
        top = []
        for s in range(segs):
            a = 2.0 * math.pi * s / segs
            dx = math.cos(a) * post_r
            dy = math.sin(a) * post_r
            bottom.append(bm.verts.new((cx + dx, cy + dy, 0)))
            # Pointed top
            top_h = height + random.uniform(-0.3, 0.3)
            top.append(bm.verts.new((cx + dx * 0.5, cy + dy * 0.5, top_h)))

        for s in range(segs):
            ns = (s + 1) % segs
            bm.faces.new((bottom[s], bottom[ns], top[ns], top[s]))
        bm.faces.new(top)

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


def create_farm_strip(name, length, width, furrow_spacing):
    """Farm strip: flat plane with furrow geometry for wheat fields."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    hl, hw = length / 2, width / 2
    num_furrows = int(width / furrow_spacing)

    for i in range(num_furrows):
        y = -hw + i * furrow_spacing
        yn = y + furrow_spacing * 0.5
        # Furrow ridge
        v0 = bm.verts.new((-hl, y, 0))
        v1 = bm.verts.new((hl, y, 0))
        v2 = bm.verts.new((hl, yn, 0.15))
        v3 = bm.verts.new((-hl, yn, 0.15))
        bm.faces.new((v0, v1, v2, v3))

        # Furrow trough
        yn2 = y + furrow_spacing
        v4 = bm.verts.new((hl, yn2, 0))
        v5 = bm.verts.new((-hl, yn2, 0))
        bm.faces.new((v3, v2, v4, v5))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


def create_boundary_wall(name, length, width, height, wall_w):
    """Stone boundary wall around farm perimeter."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    hl, hw = length / 2, width / 2
    hw2 = wall_w / 2

    # 4 wall segments as boxes
    segments = [
        ((-hl, -hw - hw2, 0), (hl, -hw + hw2, height)),   # front
        ((-hl, hw - hw2, 0), (hl, hw + hw2, height)),      # back
        ((-hl - hw2, -hw, 0), (-hl + hw2, hw, height)),    # left
        ((hl - hw2, -hw, 0), (hl + hw2, hw, height)),      # right
    ]

    for (x0, y0, z0), (x1, y1, z1) in segments:
        v = [
            bm.verts.new((x0, y0, z0)),
            bm.verts.new((x1, y0, z0)),
            bm.verts.new((x1, y1, z0)),
            bm.verts.new((x0, y1, z0)),
            bm.verts.new((x0, y0, z1)),
            bm.verts.new((x1, y0, z1)),
            bm.verts.new((x1, y1, z1)),
            bm.verts.new((x0, y1, z1)),
        ]
        bm.faces.new((v[0], v[3], v[2], v[1]))  # bottom
        bm.faces.new((v[4], v[5], v[6], v[7]))  # top
        bm.faces.new((v[0], v[1], v[5], v[4]))
        bm.faces.new((v[2], v[3], v[7], v[6]))
        bm.faces.new((v[1], v[2], v[6], v[5]))
        bm.faces.new((v[3], v[0], v[4], v[7]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


def create_stone_bridge(name, length, width, arch_h, thickness):
    """Stone bridge with arch underneath."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    hl = length / 2
    hw = width / 2
    segments = 16

    # Arch profile along X axis
    top_verts_left = []
    top_verts_right = []
    bot_verts_left = []
    bot_verts_right = []

    for i in range(segments + 1):
        t = i / segments
        x = -hl + t * length
        # Arch curve (parabolic)
        arch_y = arch_h * (1.0 - (2.0 * t - 1.0) ** 2)
        z_top = arch_y + thickness
        z_bot = arch_y

        top_verts_left.append(bm.verts.new((x, -hw, z_top)))
        top_verts_right.append(bm.verts.new((x, hw, z_top)))
        bot_verts_left.append(bm.verts.new((x, -hw, z_bot)))
        bot_verts_right.append(bm.verts.new((x, hw, z_bot)))

    for i in range(segments):
        # Top surface
        bm.faces.new((top_verts_left[i], top_verts_right[i],
                       top_verts_right[i+1], top_verts_left[i+1]))
        # Bottom surface
        bm.faces.new((bot_verts_left[i], bot_verts_left[i+1],
                       bot_verts_right[i+1], bot_verts_right[i]))
        # Left side
        bm.faces.new((top_verts_left[i], top_verts_left[i+1],
                       bot_verts_left[i+1], bot_verts_left[i]))
        # Right side
        bm.faces.new((top_verts_right[i], bot_verts_right[i],
                       bot_verts_right[i+1], top_verts_right[i+1]))

    # End caps
    bm.faces.new((top_verts_left[0], bot_verts_left[0],
                   bot_verts_right[0], top_verts_right[0]))
    bm.faces.new((top_verts_left[-1], top_verts_right[-1],
                   bot_verts_right[-1], bot_verts_left[-1]))

    # Low railings
    rail_h = 0.8
    rail_w = 0.2
    for side_y in [-hw, hw - rail_w]:
        for i in range(segments):
            x0 = -hl + (i / segments) * length
            x1 = -hl + ((i + 1) / segments) * length
            t0 = i / segments
            t1 = (i + 1) / segments
            z0 = arch_h * (1.0 - (2.0 * t0 - 1.0) ** 2) + thickness
            z1 = arch_h * (1.0 - (2.0 * t1 - 1.0) ** 2) + thickness

            rv = [
                bm.verts.new((x0, side_y, z0)),
                bm.verts.new((x1, side_y, z1)),
                bm.verts.new((x1, side_y + rail_w, z1)),
                bm.verts.new((x0, side_y + rail_w, z0)),
                bm.verts.new((x0, side_y, z0 + rail_h)),
                bm.verts.new((x1, side_y, z1 + rail_h)),
                bm.verts.new((x1, side_y + rail_w, z1 + rail_h)),
                bm.verts.new((x0, side_y + rail_w, z0 + rail_h)),
            ]
            bm.faces.new((rv[4], rv[5], rv[6], rv[7]))  # top
            bm.faces.new((rv[0], rv[1], rv[5], rv[4]))
            bm.faces.new((rv[2], rv[3], rv[7], rv[6]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


def create_watermill(name, cfg):
    """Watermill: house body + water wheel."""
    mesh = bpy.data.meshes.new(name)
    bm = bmesh.new()

    bw = cfg["mill_body_width"] / 2
    bd = cfg["mill_body_depth"] / 2
    bh = cfg["mill_body_height"]
    ridge_h = bh + 1.5

    # Body (same as house)
    v = [
        bm.verts.new((-bw, -bd, 0)), bm.verts.new((bw, -bd, 0)),
        bm.verts.new((bw, bd, 0)), bm.verts.new((-bw, bd, 0)),
        bm.verts.new((-bw, -bd, bh)), bm.verts.new((bw, -bd, bh)),
        bm.verts.new((bw, bd, bh)), bm.verts.new((-bw, bd, bh)),
        bm.verts.new((0, -bd, ridge_h)), bm.verts.new((0, bd, ridge_h)),
    ]
    bm.faces.new((v[0], v[3], v[2], v[1]))
    bm.faces.new((v[0], v[1], v[5], v[4]))
    bm.faces.new((v[2], v[3], v[7], v[6]))
    bm.faces.new((v[3], v[0], v[4], v[7]))
    bm.faces.new((v[1], v[2], v[6], v[5]))
    bm.faces.new((v[4], v[5], v[8]))
    bm.faces.new((v[6], v[7], v[9]))
    bm.faces.new((v[7], v[4], v[8], v[9]))
    bm.faces.new((v[5], v[6], v[9], v[8]))

    # Water wheel (on the side)
    wr = cfg["wheel_radius"]
    ww = cfg["wheel_width"] / 2
    wheel_x = bw + ww + 0.3
    wheel_z = wr * 0.6
    paddles = cfg["wheel_paddles"]

    hub_verts_l = []
    hub_verts_r = []
    for i in range(paddles):
        angle = 2.0 * math.pi * i / paddles
        rx = math.cos(angle) * wr
        rz = math.sin(angle) * wr
        hub_verts_l.append(bm.verts.new((wheel_x - ww, rx * 0.1, wheel_z + rz)))
        hub_verts_r.append(bm.verts.new((wheel_x + ww, rx * 0.1, wheel_z + rz)))

        # Paddle
        pr = wr * 0.15
        p0 = bm.verts.new((wheel_x - ww, math.cos(angle) * (wr - pr), wheel_z + math.sin(angle) * (wr - pr)))
        p1 = bm.verts.new((wheel_x + ww, math.cos(angle) * (wr - pr), wheel_z + math.sin(angle) * (wr - pr)))
        p2 = bm.verts.new((wheel_x + ww, math.cos(angle) * (wr + pr), wheel_z + math.sin(angle) * (wr + pr)))
        p3 = bm.verts.new((wheel_x - ww, math.cos(angle) * (wr + pr), wheel_z + math.sin(angle) * (wr + pr)))
        bm.faces.new((p0, p1, p2, p3))

    # Connect hub ring
    for i in range(paddles):
        j = (i + 1) % paddles
        bm.faces.new((hub_verts_l[i], hub_verts_l[j], hub_verts_r[j], hub_verts_r[i]))

    bm.to_mesh(mesh)
    bm.free()
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    return obj


# ============================================================================
# PLACEMENT ENGINE
# ============================================================================

def get_terrain_height(terrain_obj, x, y):
    """Raycast down onto terrain to find Z height at (x,y)."""
    result, location, normal, index = terrain_obj.ray_cast(
        Vector((x, y, 5000.0)), Vector((0, 0, -1))
    )
    if result:
        return location.z
    return 0.0


def place_village(terrain_obj, center_x, center_y, village_idx, collection, cfg):
    """Place houses in a cluster + palisade around village center."""
    random.seed(cfg["seed"] + village_idx * 100)
    num_houses = cfg["houses_per_village"]

    z_center = get_terrain_height(terrain_obj, center_x, center_y)

    # Place houses in a rough circle
    for h in range(num_houses):
        angle = 2.0 * math.pi * h / num_houses + random.uniform(-0.3, 0.3)
        dist = random.uniform(15.0, 45.0)
        hx = center_x + math.cos(angle) * dist
        hy = center_y + math.sin(angle) * dist
        hz = get_terrain_height(terrain_obj, hx, hy)

        w = random.uniform(*cfg["house_width"])
        d = random.uniform(*cfg["house_depth"])
        wh = random.uniform(*cfg["house_wall_height"])
        rp = random.uniform(*cfg["house_roof_pitch"])

        house = create_timber_house(f"House_V{village_idx}_{h}", w, d, wh, rp)
        house.location = (hx, hy, hz)
        house.rotation_euler.z = angle + random.uniform(-0.5, 0.5)
        collection.objects.link(house)

    # Palisade
    palisade = create_palisade(
        f"Palisade_V{village_idx}",
        cfg["palisade_radius"], cfg["palisade_height"],
        cfg["palisade_post_spacing"], cfg["palisade_post_radius"]
    )
    palisade.location = (center_x, center_y, z_center)
    collection.objects.link(palisade)

    print(f"[IRONVALE] Village {village_idx}: {num_houses} houses + palisade at ({center_x:.0f}, {center_y:.0f})")


def place_farm(terrain_obj, center_x, center_y, farm_idx, collection, cfg):
    """Place a strip farm with boundary wall."""
    z = get_terrain_height(terrain_obj, center_x, center_y)
    rot_z = random.uniform(0, math.pi)

    farm = create_farm_strip(
        f"Farm_{farm_idx}",
        cfg["farm_length"], cfg["farm_width"], cfg["farm_furrow_spacing"]
    )
    farm.location = (center_x, center_y, z)
    farm.rotation_euler.z = rot_z
    collection.objects.link(farm)

    wall = create_boundary_wall(
        f"FarmWall_{farm_idx}",
        cfg["farm_length"], cfg["farm_width"],
        cfg["boundary_wall_height"], cfg["boundary_wall_width"]
    )
    wall.location = (center_x, center_y, z)
    wall.rotation_euler.z = rot_z
    collection.objects.link(wall)

    print(f"[IRONVALE] Farm {farm_idx} at ({center_x:.0f}, {center_y:.0f})")


# ============================================================================
# MAIN
# ============================================================================

def main():
    cfg = CONFIG
    random.seed(cfg["seed"])
    size = 5000.0

    print("=" * 60)
    print("  IRONVALE STRUCTURE GENERATOR v1.0")
    print("=" * 60)

    terrain_obj = bpy.data.objects.get(cfg["terrain_object"])
    if terrain_obj is None:
        print("[ERROR] Terrain not found! Run ironvale_terrain_gen.py first.")
        return

    # Ensure depsgraph is current for raycasting
    bpy.context.view_layer.update()

    # Collection
    col = bpy.data.collections.new("Ironvale_Structures")
    bpy.context.scene.collection.children.link(col)

    # Read POI markers or use config positions
    poi_col = bpy.data.collections.get("Ironvale_POIs")

    village_positions = []
    farm_positions = []
    bridge_pos = None
    mill_pos = None

    if poi_col:
        for obj in poi_col.objects:
            pt = obj.get("poi_type", "")
            if pt == "village":
                village_positions.append((obj.location.x, obj.location.y))
            elif pt == "farm":
                farm_positions.append((obj.location.x, obj.location.y))
            elif pt == "bridge":
                bridge_pos = (obj.location.x, obj.location.y)
            elif pt == "watermill":
                mill_pos = (obj.location.x, obj.location.y)

    # Fallback to config if no POIs found
    if not village_positions:
        village_positions = [((v[0]-0.5)*size, (v[1]-0.5)*size) for v in cfg.get("village_sites", [(0.35,0.3),(0.6,0.55),(0.45,0.78)])]
    if not farm_positions:
        farm_positions = [((f[0]-0.5)*size, (f[1]-0.5)*size) for f in cfg.get("farm_sites", [])]
    if bridge_pos is None:
        bridge_pos = (100.0, 0.0)
    if mill_pos is None:
        mill_pos = (0.0, 600.0)

    # Place villages
    for i, (vx, vy) in enumerate(village_positions):
        place_village(terrain_obj, vx, vy, i, col, cfg)

    # Place farms
    for i, (fx, fy) in enumerate(farm_positions):
        place_farm(terrain_obj, fx, fy, i, col, cfg)

    # Place bridge
    bx, by = bridge_pos
    bz = get_terrain_height(terrain_obj, bx, by)
    bridge = create_stone_bridge(
        "Ironvale_Bridge",
        cfg["bridge_length"], cfg["bridge_width"],
        cfg["bridge_arch_height"], cfg["bridge_thickness"]
    )
    bridge.location = (bx, by, bz - cfg["bridge_arch_height"] * 0.3)
    col.objects.link(bridge)
    print(f"[IRONVALE] Bridge at ({bx:.0f}, {by:.0f})")

    # Place watermill
    mx, my = mill_pos
    mz = get_terrain_height(terrain_obj, mx, my)
    mill = create_watermill("Ironvale_Watermill", cfg)
    mill.location = (mx, my, mz)
    col.objects.link(mill)
    print(f"[IRONVALE] Watermill at ({mx:.0f}, {my:.0f})")

    print("")
    print("=" * 60)
    print("  IRONVALE STRUCTURES COMPLETE")
    print("=" * 60)
    print(f"  {len(village_positions)} villages, {len(farm_positions)} farms, 1 bridge, 1 watermill")
    print("  NEXT: Run ironvale_materials.py for PBR shaders.")
    print("=" * 60)


if __name__ == "__main__":
    main()
