"""
IRONVALE FBX & Heightmap Export Pipeline — Blender 4.2+
========================================================
Exports:
  - Terrain as chunked FBX static meshes (8x8 grid)
  - Props (houses, trees, rocks) as individual FBX
  - Heightmap as 16-bit EXR for UE5 Landscape import
  - Packed textures (diffuse, normal, AO, roughness) as PNG/EXR
  - Placement CSV for UE5 DataTable import

Output: /Ironvale/Assets/ (auto-created, cross-platform paths)

Prerequisites: Run all previous scripts first.
"""

import bpy
import bmesh
import os
import csv
import math
from pathlib import Path
from mathutils import Vector

# ============================================================================
# CONFIGURATION
# ============================================================================
CONFIG = {
    "terrain_object": "Ironvale_Terrain",
    "chunk_grid": 8,
    "terrain_size": 5000.0,

    # Export paths (relative to .blend file or project root)
    # Uses pathlib for cross-platform compatibility
    "export_base": "Assets",
    "terrain_subdir": "Terrain",
    "props_subdir": "Props",
    "textures_subdir": "Textures",
    "data_subdir": "Data",

    # FBX settings
    "fbx_scale": 1.0,
    "fbx_apply_transforms": True,
    "fbx_mesh_smooth": 'FACE',

    # Heightmap
    "heightmap_resolution": 4096,

    # LOD settings (distance thresholds in meters)
    "lod_levels": 4,
    "lod_ratios": [1.0, 0.5, 0.25, 0.1],  # Decimation ratios per LOD
}


# ============================================================================
# PATH UTILITIES
# ============================================================================

def get_export_root():
    """
    Determine export root directory.
    Prefers project root (where .uproject lives), falls back to .blend location.
    """
    # Try to find project root by looking for .uproject
    blend_path = bpy.data.filepath
    if blend_path:
        search = Path(blend_path).parent
    else:
        search = Path.cwd()

    # Walk up to find .uproject
    for parent in [search] + list(search.parents):
        uproject_files = list(parent.glob("*.uproject"))
        if uproject_files:
            return parent / CONFIG["export_base"]

    # Fallback: next to blend file or cwd
    return search / CONFIG["export_base"]


def ensure_dir(path):
    """Create directory if it doesn't exist."""
    path.mkdir(parents=True, exist_ok=True)
    return path


# ============================================================================
# TERRAIN CHUNK EXPORT
# ============================================================================

def export_terrain_chunks(cfg):
    """
    Split terrain into grid chunks and export each as FBX.
    Each chunk is a separate static mesh for UE5 World Partition / streaming.
    """
    terrain_obj = bpy.data.objects.get(cfg["terrain_object"])
    if terrain_obj is None:
        print("[ERROR] Terrain object not found!")
        return

    root = get_export_root()
    terrain_dir = ensure_dir(root / cfg["terrain_subdir"])

    grid = cfg["chunk_grid"]
    size = cfg["terrain_size"]
    chunk_size = size / grid
    half = size / 2

    print(f"[IRONVALE] Exporting {grid}x{grid} terrain chunks to {terrain_dir}")

    # We need to work with the evaluated mesh (with modifiers applied)
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = terrain_obj.evaluated_get(depsgraph)
    eval_mesh = eval_obj.to_mesh()

    for cy in range(grid):
        for cx in range(grid):
            chunk_name = f"Terrain_Chunk_{cx}_{cy}"

            # Chunk bounds
            x_min = -half + cx * chunk_size
            x_max = x_min + chunk_size
            y_min = -half + cy * chunk_size
            y_max = y_min + chunk_size

            # Create new mesh with only verts inside this chunk
            new_mesh = bpy.data.meshes.new(chunk_name)
            bm = bmesh.new()

            # Copy vertices and remap
            vert_map = {}
            for v in eval_mesh.vertices:
                co = v.co
                if x_min <= co.x <= x_max and y_min <= co.y <= y_max:
                    new_v = bm.verts.new(co - Vector((x_min + chunk_size/2, y_min + chunk_size/2, 0)))
                    vert_map[v.index] = new_v

            bm.verts.ensure_lookup_table()

            # Copy faces where all verts are in chunk
            for poly in eval_mesh.polygons:
                face_verts = []
                valid = True
                for vi in poly.vertices:
                    if vi in vert_map:
                        face_verts.append(vert_map[vi])
                    else:
                        valid = False
                        break
                if valid and len(face_verts) >= 3:
                    try:
                        bm.faces.new(face_verts)
                    except ValueError:
                        pass  # Duplicate face

            bm.faces.ensure_lookup_table()

            if len(bm.faces) == 0:
                bm.free()
                continue

            # UV layer
            uv_layer = bm.loops.layers.uv.new("UVMap")
            for face in bm.faces:
                for loop in face.loops:
                    co = loop.vert.co
                    loop[uv_layer].uv = (
                        (co.x + chunk_size/2) / chunk_size,
                        (co.y + chunk_size/2) / chunk_size
                    )

            bm.to_mesh(new_mesh)
            bm.free()
            new_mesh.update()

            # Create temporary object for export
            temp_obj = bpy.data.objects.new(chunk_name, new_mesh)
            bpy.context.collection.objects.link(temp_obj)

            # Copy material
            if terrain_obj.data.materials:
                temp_obj.data.materials.append(terrain_obj.data.materials[0])

            # Select only this chunk
            bpy.ops.object.select_all(action='DESELECT')
            temp_obj.select_set(True)
            bpy.context.view_layer.objects.active = temp_obj

            # Export FBX
            fbx_path = str(terrain_dir / f"{chunk_name}.fbx")
            bpy.ops.export_scene.fbx(
                filepath=fbx_path,
                use_selection=True,
                global_scale=cfg["fbx_scale"],
                apply_scale_options='FBX_SCALE_ALL',
                use_mesh_modifiers=True,
                mesh_smooth_type=cfg["fbx_mesh_smooth"],
                use_mesh_edges=False,
                path_mode='COPY',
            )

            # Remove temp object
            bpy.data.objects.remove(temp_obj)
            bpy.data.meshes.remove(new_mesh)

    eval_obj.to_mesh_clear()
    print(f"[IRONVALE] Terrain chunks exported.")


# ============================================================================
# PROP EXPORT
# ============================================================================

def export_props(cfg):
    """Export individual props (houses, trees, rocks, bridge, mill) as FBX."""
    root = get_export_root()
    props_dir = ensure_dir(root / cfg["props_subdir"])

    # Gather exportable objects (skip terrain, water, empties, hidden proxies)
    skip_names = {"ironvale_terrain", "ironvale_waterplane", "ironvale_water_plane"}
    skip_collections = {"Ironvale_ChunkGrid", "Ironvale_POIs", "Ironvale_ScatterProxies"}

    exported = []

    for obj in bpy.data.objects:
        if obj.type != 'MESH':
            continue
        if obj.name.lower().replace(" ", "_") in skip_names:
            continue
        if obj.hide_viewport or obj.hide_render:
            continue

        # Check if in skip collection
        in_skip = False
        for col in obj.users_collection:
            if col.name in skip_collections:
                in_skip = True
                break
        if in_skip:
            continue

        # Export this prop
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj

        safe_name = obj.name.replace(" ", "_").replace(".", "_")
        fbx_path = str(props_dir / f"{safe_name}.fbx")

        bpy.ops.export_scene.fbx(
            filepath=fbx_path,
            use_selection=True,
            global_scale=cfg["fbx_scale"],
            apply_scale_options='FBX_SCALE_ALL',
            use_mesh_modifiers=True,
            mesh_smooth_type=cfg["fbx_mesh_smooth"],
            use_mesh_edges=False,
            path_mode='COPY',
        )

        exported.append({
            "name": obj.name,
            "file": f"{safe_name}.fbx",
            "location_x": obj.location.x,
            "location_y": obj.location.y,
            "location_z": obj.location.z,
            "rotation_z": math.degrees(obj.rotation_euler.z),
            "scale": obj.scale.x,
        })

    print(f"[IRONVALE] Exported {len(exported)} props to {props_dir}")
    return exported


# ============================================================================
# LOD GENERATION
# ============================================================================

def generate_lod_meshes(obj, cfg):
    """
    Generate LOD variants of an object using decimation.
    Creates LOD0 (original) through LOD3.
    """
    lod_objects = []
    ratios = cfg["lod_ratios"]

    for lod_level, ratio in enumerate(ratios):
        if lod_level == 0:
            # LOD0 is the original
            lod_objects.append(obj)
            continue

        # Duplicate
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.duplicate()
        lod_obj = bpy.context.active_object
        lod_obj.name = f"{obj.name}_LOD{lod_level}"

        # Add decimation modifier
        mod = lod_obj.modifiers.new(f"LOD{lod_level}_Decimate", 'DECIMATE')
        mod.ratio = ratio

        # Apply modifier
        bpy.ops.object.modifier_apply(modifier=mod.name)

        lod_obj.hide_viewport = True
        lod_objects.append(lod_obj)

    return lod_objects


# ============================================================================
# HEIGHTMAP EXPORT
# ============================================================================

def export_heightmap(cfg):
    """
    Export terrain heightmap as 16-bit EXR for UE5 Landscape import.
    Renders a top-down orthographic depth pass.
    """
    terrain_obj = bpy.data.objects.get(cfg["terrain_object"])
    if terrain_obj is None:
        print("[ERROR] Terrain not found for heightmap export!")
        return

    root = get_export_root()
    tex_dir = ensure_dir(root / cfg["textures_subdir"])
    res = cfg["heightmap_resolution"]

    # Check if we have a pre-baked heightmap image
    img = bpy.data.images.get("Ironvale_Heightmap")
    if img and img.size[0] > 0:
        filepath = str(tex_dir / "Ironvale_Heightmap.exr")
        img.filepath_raw = filepath
        img.file_format = 'OPEN_EXR'
        img.save()
        print(f"[IRONVALE] Heightmap saved: {filepath}")
        return

    # Alternative: generate heightmap from mesh vertex data
    print("[IRONVALE] Generating heightmap from mesh data...")

    size = cfg["terrain_size"]
    half = size / 2

    # Create image
    img = bpy.data.images.new("Ironvale_Heightmap_Export", res, res, float_buffer=True)
    pixels = [0.0] * (res * res * 4)

    # Get evaluated mesh
    depsgraph = bpy.context.evaluated_depsgraph_get()
    eval_obj = terrain_obj.evaluated_get(depsgraph)
    eval_mesh = eval_obj.to_mesh()

    # Find height range
    min_z = min(v.co.z for v in eval_mesh.vertices)
    max_z = max(v.co.z for v in eval_mesh.vertices)
    z_range = max_z - min_z if max_z > min_z else 1.0

    # Sample heights using nearest vertex (fast approximation)
    from mathutils import kdtree as kd
    kdt = kd.KDTree(len(eval_mesh.vertices))
    for i, v in enumerate(eval_mesh.vertices):
        kdt.insert(v.co, i)
    kdt.balance()

    for py in range(res):
        wy = -half + (py / (res - 1)) * size
        for px in range(res):
            wx = -half + (px / (res - 1)) * size

            # Find nearest vertex
            co, idx, dist = kdt.find(Vector((wx, wy, 0)))
            z = eval_mesh.vertices[idx].co.z
            h_norm = (z - min_z) / z_range

            # RGBA pixel
            offset = (py * res + px) * 4
            pixels[offset] = h_norm
            pixels[offset + 1] = h_norm
            pixels[offset + 2] = h_norm
            pixels[offset + 3] = 1.0

    img.pixels[:] = pixels

    filepath = str(tex_dir / "Ironvale_Heightmap.exr")
    img.filepath_raw = filepath
    img.file_format = 'OPEN_EXR'
    img.save()

    eval_obj.to_mesh_clear()
    print(f"[IRONVALE] Heightmap exported: {filepath}")


# ============================================================================
# TEXTURE EXPORT
# ============================================================================

def export_textures(cfg):
    """Save all baked texture images to disk."""
    root = get_export_root()
    tex_dir = ensure_dir(root / cfg["textures_subdir"])

    texture_names = {
        "Ironvale_NormalMap": ("Ironvale_NormalMap.png", "PNG"),
        "Ironvale_AO": ("Ironvale_AO.png", "PNG"),
        "Ironvale_Diffuse": ("Ironvale_Diffuse.png", "PNG"),
        "Ironvale_Roughness": ("Ironvale_Roughness.png", "PNG"),
    }

    exported_count = 0
    for img_name, (filename, fmt) in texture_names.items():
        img = bpy.data.images.get(img_name)
        if img and img.size[0] > 0 and img.has_data:
            filepath = str(tex_dir / filename)
            img.filepath_raw = filepath
            img.file_format = fmt
            img.save()
            print(f"[IRONVALE] Texture saved: {filepath}")
            exported_count += 1
        else:
            print(f"[IRONVALE] Texture '{img_name}' not baked yet — skipping.")

    print(f"[IRONVALE] {exported_count} textures exported.")


# ============================================================================
# PLACEMENT DATA EXPORT (CSV for UE5 DataTable)
# ============================================================================

def export_placement_csv(props_data, cfg):
    """
    Export prop placement data as CSV for UE5 DataTable import.
    Columns: Name, MeshPath, X, Y, Z, RotationZ, Scale
    """
    root = get_export_root()
    data_dir = ensure_dir(root / cfg["data_subdir"])
    csv_path = str(data_dir / "Ironvale_Placement.csv")

    with open(csv_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Name", "MeshFile", "X", "Y", "Z", "RotationZ", "Scale"])
        for prop in props_data:
            writer.writerow([
                prop["name"],
                prop["file"],
                f"{prop['location_x']:.2f}",
                f"{prop['location_y']:.2f}",
                f"{prop['location_z']:.2f}",
                f"{prop['rotation_z']:.2f}",
                f"{prop['scale']:.3f}",
            ])

    print(f"[IRONVALE] Placement CSV: {csv_path} ({len(props_data)} entries)")


# ============================================================================
# WATER PLANE EXPORT
# ============================================================================

def export_water_plane(cfg):
    """Export water plane as separate FBX."""
    water = bpy.data.objects.get("Ironvale_WaterPlane")
    if water is None:
        print("[IRONVALE] No water plane found — skipping.")
        return

    root = get_export_root()
    terrain_dir = ensure_dir(root / cfg["terrain_subdir"])

    bpy.ops.object.select_all(action='DESELECT')
    water.select_set(True)
    bpy.context.view_layer.objects.active = water

    fbx_path = str(terrain_dir / "Ironvale_WaterPlane.fbx")
    bpy.ops.export_scene.fbx(
        filepath=fbx_path,
        use_selection=True,
        global_scale=cfg["fbx_scale"],
        apply_scale_options='FBX_SCALE_ALL',
        use_mesh_modifiers=True,
        path_mode='COPY',
    )

    print(f"[IRONVALE] Water plane exported: {fbx_path}")


# ============================================================================
# MAIN
# ============================================================================

def main():
    cfg = CONFIG

    print("=" * 60)
    print("  IRONVALE EXPORT PIPELINE v1.0")
    print("=" * 60)

    root = get_export_root()
    print(f"  Export root: {root}")
    print("")

    # Create all directories
    ensure_dir(root / cfg["terrain_subdir"])
    ensure_dir(root / cfg["props_subdir"])
    ensure_dir(root / cfg["textures_subdir"])
    ensure_dir(root / cfg["data_subdir"])

    # 1. Terrain chunks
    print("[IRONVALE] === TERRAIN CHUNKS ===")
    export_terrain_chunks(cfg)

    # 2. Water plane
    print("\n[IRONVALE] === WATER PLANE ===")
    export_water_plane(cfg)

    # 3. Props
    print("\n[IRONVALE] === PROPS ===")
    props_data = export_props(cfg)

    # 4. Heightmap
    print("\n[IRONVALE] === HEIGHTMAP ===")
    export_heightmap(cfg)

    # 5. Textures
    print("\n[IRONVALE] === TEXTURES ===")
    export_textures(cfg)

    # 6. Placement CSV
    print("\n[IRONVALE] === PLACEMENT DATA ===")
    export_placement_csv(props_data, cfg)

    print("")
    print("=" * 60)
    print("  IRONVALE EXPORT COMPLETE")
    print("=" * 60)
    print("")
    print(f"  Output directory: {root}")
    print(f"    {cfg['terrain_subdir']}/     — Terrain chunk FBX files + water plane")
    print(f"    {cfg['props_subdir']}/        — Individual prop FBX files")
    print(f"    {cfg['textures_subdir']}/    — Heightmap EXR + baked PNGs")
    print(f"    {cfg['data_subdir']}/         — Placement CSV for DataTable")
    print("")
    print("  UE5 IMPORT WORKFLOW:")
    print("  1. Import Ironvale_Heightmap.exr via Landscape → Import from File")
    print("  2. Import terrain chunks as Static Meshes (or use Landscape)")
    print("  3. Import props as Static Meshes")
    print("  4. Import Ironvale_Placement.csv as DataTable (struct: FIronvalePropPlacement)")
    print("  5. Use placement data to spawn props via construction script or runtime")
    print("  6. Assign texture maps to material instances")
    print("=" * 60)


if __name__ == "__main__":
    main()
