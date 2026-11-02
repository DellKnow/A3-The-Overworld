"""Export Pumpkin Scarecrow into CSCI441-friendly articulated OBJ/MTL parts.

HOW TO USE:
  1. Open finished PumpkinScarecrow.blend file in Blender.
  2. Save it (Ctrl+S), then open this script in the Scripting workspace.
  3. Run Script (Alt+P).
  4. Find the OBJ, MTL, and parts.csv files in:
            <folder holding the .blend>/obj_parts/

Each OBJ is a complete articulated section (e.g. entire head with facial details),
NOT a single geometric primitive. Individual objects' Blender materials are saved
as MTL usemtl sections, preserving all their colors in one model.

Geometry is written RELATIVE TO ITS ANIMATION PIVOT and converted from Blender's
Z-up coordinate system to typical OpenGL Y-up using (x, y, z) -> (x, z, -y).
All offsets in parts.csv use the same conversion and are relative to the parent.
The character's front thus points toward positive OpenGL Z.

This script reads the scene; it does not move or alter the character or scene.
No external Blender export add-ons are required.
"""

import bpy
import csv
import re
from pathlib import Path
from mathutils import Vector

ROOT_NAME_PREFIX = 'ROOT /'
OUTPUT_SUBFOLDER = 'obj_parts'


def safe_name(name):
    return re.sub(r'[^a-zA-Z0-9_]+', '_', name.strip()).strip('_')


def to_gl(v):
    """Blender Z-up to Y-up (proper right-handed rotation, not reflection)."""
    return Vector((v.x, v.z, -v.y))


def rgb_and_surface(mat):
    if mat is None:
        return (0.65, 0.65, 0.65), 0.76, 0.0
    rgba = mat.diffuse_color
    rgb = tuple(float(v) for v in rgba[:3])
    roughness = 0.76
    metallic = 0.0
    if mat.use_nodes and mat.node_tree:
        bsdf = next((node for node in mat.node_tree.nodes if node.type == 'BSDF_PRINCIPLED'), None)
        if bsdf:
            for k, default in (('Base Color', None), ('Roughness', roughness), ('Metallic', metallic)):
                inp = bsdf.inputs.get(k)
                if inp is None:
                    continue
                if k == 'Base Color':
                    rgb = tuple(float(v) for v in inp.default_value[:3])
                elif k == 'Roughness':
                    roughness = float(inp.default_value)
                elif k == 'Metallic':
                    metallic = float(inp.default_value)
    return rgb, roughness, metallic


def phong(mat):
    """Editable approximation of PBR settings for Phong/Gouraud illumination."""
    color, roughness, metallic = rgb_and_surface(mat)
    name = mat.name.lower() if mat else ''
    if any(word in name for word in ('shirt', 'vest', 'scarf', 'shorts', 'sleeve', 'fabric')):
        ks, ns = 0.04, 6.0
    elif any(word in name for word in ('wood', 'branch', 'root', 'stem')):
        ks, ns = 0.08, 8.0
    elif any(word in name for word in ('pumpkin', 'carved', 'socket')):
        ks, ns = 0.20, 16.0
    elif 'brass' in name or 'buckle' in name:
        ks, ns = 0.55, 50.0
    elif 'glove' in name or 'blue' in name:
        ks, ns = 0.18, 20.0
    elif 'leather' in name or 'satchel' in name:
        ks, ns = 0.10, 12.0
    else:
        ks, ns = 0.12, 14.0
    ks = min(1.0, ks + 0.2 * metallic)
    # Ka = a modest proportion of base color; Kd is the original base color.
    ka = tuple(0.16 * c for c in color)
    return ka, color, (ks, ks, ks), ns


def descendants(parent):
    for child in parent.children:
        yield child
        yield from descendants(child)


def render_part(pivot, meshes, folder, depsgraph):
    """Write one grouped OBJ, containing many colored mesh objects, plus an MTL."""
    short = safe_name(pivot.name.split('/')[0]).lower()
    obj_name = short + '.obj'
    mtl_name = short + '.mtl'
    obj_path = folder / obj_name
    mtl_path = folder / mtl_name

    materials = {}  # Blender material -> MTL identifier
    for obj in meshes:
        for material in obj.data.materials:
            if material is not None and material not in materials:
                materials[material] = 'material_%02d_%s' % (len(materials), safe_name(material.name))
    materials[None] = 'material_default'

    tri_count = 0
    with obj_path.open('w', encoding='utf-8') as f:
        f.write('# Articulated low-poly character part for CSCI441::ModelLoader\n')
        f.write('# Vertices relative to their pivot, in Y-up OpenGL coordinates\n')
        f.write('mtllib %s\n' % mtl_name)
        vertex_base, normal_base, uv_base = 0, 0, 0
        pivot_eval = pivot.evaluated_get(depsgraph)
        pivot_inv = pivot_eval.matrix_world.inverted_safe()

        for obj in sorted(meshes, key=lambda o: o.name):
            obj_eval = obj.evaluated_get(depsgraph)
            eval_mesh = obj_eval.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
            if eval_mesh is None:
                continue
            try:
                eval_mesh.calc_loop_triangles()
                if not eval_mesh.loop_triangles:
                    continue

                relative_mtx = pivot_inv @ obj_eval.matrix_world
                normal_mtx = relative_mtx.to_3x3().inverted_safe().transposed()
                uv_layer = eval_mesh.uv_layers.active

                f.write('\no %s\n' % safe_name(obj.name))
                for vert in eval_mesh.vertices:
                    co = to_gl(relative_mtx @ vert.co)
                    f.write('v %.8f %.8f %.8f\n' % tuple(co))

                # To preserve smooth/flat per-corner normals, write a normal
                # for each loop; this is small for our relatively low-poly model.
                corner_normals = getattr(eval_mesh, 'corner_normals', None)
                have_corner_normals = (corner_normals is not None and
                                       len(corner_normals) == len(eval_mesh.loops))
                fallback_face_normals = {}
                if not have_corner_normals:
                    for poly in eval_mesh.polygons:
                        for li in poly.loop_indices:
                            fallback_face_normals[li] = poly.normal
                for loop_idx, loop in enumerate(eval_mesh.loops):
                    n = (corner_normals[loop_idx].vector if have_corner_normals
                         else fallback_face_normals[loop_idx])
                    normal = to_gl(normal_mtx @ n).normalized()
                    f.write('vn %.8f %.8f %.8f\n' % tuple(normal))

                if uv_layer is not None:
                    for loop_idx in range(len(eval_mesh.loops)):
                        uv = uv_layer.data[loop_idx].uv
                        f.write('vt %.8f %.8f\n' % (uv.x, uv.y))

                last_mat = None
                for tri in eval_mesh.loop_triangles:
                    mat_index = eval_mesh.polygons[tri.polygon_index].material_index
                    mat = eval_mesh.materials[mat_index] if mat_index < len(eval_mesh.materials) else None
                    mat_name = materials.get(mat, 'material_default')
                    if mat_name != last_mat:
                        f.write('usemtl %s\n' % mat_name)
                        last_mat = mat_name
                    vertex_refs = []
                    for li in tri.loops:
                        loop = eval_mesh.loops[li]
                        vi = vertex_base + loop.vertex_index + 1
                        ni = normal_base + li + 1
                        if uv_layer is not None:
                            ti = uv_base + li + 1
                            vertex_refs.append('%d/%d/%d' % (vi, ti, ni))
                        else:
                            vertex_refs.append('%d//%d' % (vi, ni))
                    f.write('f %s\n' % ' '.join(vertex_refs))
                    tri_count += 1

                vertex_base += len(eval_mesh.vertices)
                normal_base += len(eval_mesh.loops)
                if uv_layer is not None:
                    uv_base += len(eval_mesh.loops)
            finally:
                obj_eval.to_mesh_clear()

    with mtl_path.open('w', encoding='utf-8') as f:
        f.write('# Each material contains Ka, Kd, Ks, Ns for Phong lighting\n')
        for mat, ident in materials.items():
            ka, kd, ks, ns = phong(mat)
            f.write('\nnewmtl %s\n' % ident)
            f.write('Ka %.6f %.6f %.6f\n' % ka)
            f.write('Kd %.6f %.6f %.6f\n' % kd)
            f.write('Ks %.6f %.6f %.6f\n' % ks)
            f.write('Ns %.4f\n' % ns)
            f.write('d 1.0\nillum 2\n')
    return obj_name, tri_count


def main():
    if not bpy.data.filepath:
        raise RuntimeError('Save your Blender scene as a .blend file before exporting.')
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')

    roots = [ob for ob in bpy.data.objects if ob.type == 'EMPTY' and ob.name.startswith(ROOT_NAME_PREFIX)]
    if len(roots) != 1:
        raise RuntimeError('Expected one ROOT / ... Empty. Found %d.' % len(roots))
    root = roots[0]

    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    output = Path(bpy.path.abspath('//')) / OUTPUT_SUBFOLDER
    output.mkdir(parents=True, exist_ok=True)

    pivots = [root] + [child for child in descendants(root) if child.type == 'EMPTY']
    rows = []
    total_tris = 0

    for p in pivots:
        # Only objects directly attached to THIS pivot. The nested pivot
        # will get its own OBJ, allowing elbow/knee/wrist rotations.
        meshes = [ob for ob in p.children if ob.type == 'MESH']
        part_file = ''
        triangles = 0
        if meshes:
            part_file, triangles = render_part(p, meshes, output, depsgraph)
        total_tris += triangles

        ident = safe_name(p.name.split('/')[0]).upper()
        parent_ident = safe_name(p.parent.name.split('/')[0]).upper() if p.parent else ''
        offset = to_gl(Vector(p.location))
        rows.append((ident, parent_ident, *[float(c) for c in offset], part_file, len(meshes), triangles))

    with (output / 'parts.csv').open('w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(('part', 'parent', 'offset_x', 'offset_y', 'offset_z', 'obj_file', 'mesh_objects', 'triangles'))
        writer.writerows(rows)

    with (output / 'README.txt').open('w', encoding='utf-8') as f:
        f.write('Pumpkin Scarecrow -- articulated OBJ meshes for CSCI441 OpenGL\n\n')
        f.write('Use CSCI441::ModelLoader on each OBJ. The MTL with the same name is read automatically.\n')
        f.write('Each OBJ contains all colors/materials for one animated part.\n')
        f.write('The parts.csv offset is relative to its parent joint, in OpenGL Y-up coordinates.\n')
        f.write('Geometry is centered at its own joint, not at the global world origin.\n')
        f.write('Construct matrices: parentMatrix * translate(offset) * rotate(animatedAngle).\n')
        f.write('ROOT has no OBJ: it is the character placement node.\n')
        f.write('For CSCI441::ModelLoader::draw, use vec4 material uniforms (it uploads 4 floats).\n')
        f.write('In your shader use materialDiffColor.rgb / materialAmbColor.rgb / materialSpecColor.rgb.\n')
        f.write('Set useTexture=0 for these untextured meshes.\n\n')
        f.write('The exported character faces +Z (Blender forward -Y). Ground is at Y=0.\n')
        f.write('Files: %d OBJ, total %d triangles.\n' % (sum(bool(row[5]) for row in rows), total_tris))
    print('SUCCESS:', output)
    print('Created', sum(bool(row[5]) for row in rows), 'OBJ/MTL groups; triangles =', total_tris)
    for r in rows:
        print(r[0], 'parent=', r[1], 'obj=', r[5], 'triangles=', r[7])

main()
