"""Pumpkin Scarecrow

Designed for a C++ / OpenGL hierarchical character: each arm, elbow, hand,
hip, knee, foot, head and accessory is a separate object / group.

RUN: Blender > Scripting > Open this file > Run Script (Alt+P).
IMPORTANT: CLEAR_SCENE=True deletes all objects in the open Blender scene.

Front direction: -Y; right: +X; up: +Z; feet rest at Z=0.
Character height ~4.65 Blender units (including stem).

Script was assisted with chatGPT.
"""
import bpy
import math
from mathutils import Vector
from math import sin, cos, pi, sqrt
from pathlib import Path

#================================ Settings ============================================#
CLEAR_SCENE = True
ADD_STAFF = True
EXPORT_GLB = False
GLB_PATH = str(Path.home() / 'PumpkinScarecrow.glb')
SHOW_EXTRA_SCENE_OBJECTS = False  # no camera / floor / lights by default

#================================= General ============================================#
if CLEAR_SCENE:
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

# Two collections keep mesh geometry and helper pivots distinct in Outliner.
def collection(name):
    c = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(c)
    return c

MESHES = collection('PumpkinGuy | mesh geometry')
RIG = collection('PumpkinGuy | pivots for animation')


def move_to_collection(obj, target):
    for c in list(obj.users_collection):
        c.objects.unlink(obj)
    target.objects.link(obj)


def mat(name, color, metallic=0.0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1.0)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    if bsdf:
        bsdf.inputs['Base Color'].default_value = (*color, 1.0)
        bsdf.inputs['Roughness'].default_value = 0.76
        bsdf.inputs['Metallic'].default_value = metallic
    return m

ORANGE = mat('01 | Golden pumpkin', (0.90, 0.40, 0.09))
ORANGE_HI = mat('02 | Pumpkin highlights', (0.98, 0.60, 0.18))
ORANGE_DARK = mat('03 | Pumpkin groove brown', (0.40, 0.17, 0.06))
DARK = mat('04 | Deep carved sockets', (0.07, 0.038, 0.024))
AMBER = mat('05 | Carved inner edge', (0.36, 0.16, 0.065))
STEM = mat('06 | Dry green stem', (0.38, 0.42, 0.16))
WOOD = mat('07 | Warm brown branch', (0.36, 0.23, 0.12))
WOOD_L = mat('08 | Dry branch highlight', (0.52, 0.35, 0.18))
WOOD_D = mat('09 | Root tips', (0.21, 0.14, 0.08))
SHIRT = mat('10 | Straw yellow shirt', (0.89, 0.73, 0.35))
VEST = mat('11 | Russet red vest', (0.57, 0.18, 0.12))
SHORTS = mat('12 | Brick red shorts', (0.56, 0.20, 0.14))
SCARF = mat('13 | Muted blue scarf', (0.48, 0.69, 0.80))
GLOVES = mat('14 | Pale blue gloves', (0.63, 0.84, 0.88))
CUFFS = mat('15 | Blue cuff trim', (0.34, 0.55, 0.65))
SHOULDER = mat('16 | Warm yellow sleeves', (0.87, 0.62, 0.23))
LEATHER = mat('17 | Dark leather satchel', (0.25, 0.18, 0.14))
FLAP = mat('18 | Leather flap', (0.37, 0.26, 0.19))
BUCKLE = mat('19 | Brass buckle', (0.83, 0.66, 0.34), metallic=0.2)


def add_material(obj, material):
    obj.data.materials.append(material)
    move_to_collection(obj, MESHES)
    return obj


def mesh(name, verts, faces, material, smooth=False):
    m = bpy.data.meshes.new(name + '_mesh')
    m.from_pydata(verts, [], faces)
    m.update()
    o = bpy.data.objects.new(name, m)
    MESHES.objects.link(o)
    add_material(o, material)
    for polygon in m.polygons:
        polygon.use_smooth = smooth
    return o


def ellipsoid(name, position, radius_xyz, material, segments=12, rings=6, smooth=False):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=position)
    o = bpy.context.object
    o.name = name
    o.scale = radius_xyz
    add_material(o, material)
    if smooth:
        for poly in o.data.polygons:
            poly.use_smooth = True
    return o


def cylinder(name, a, b, r1, r2, material, sides=7):
    a, b = Vector(a), Vector(b)
    dv = b-a
    if dv.length == 0:
        raise ValueError('zero length cylinder: ' + name)
    bpy.ops.mesh.primitive_cone_add(vertices=sides, radius1=r1, radius2=r2,
                                    depth=dv.length, location=(a+b)/2)
    o = bpy.context.object
    o.name = name
    o.rotation_mode = 'QUATERNION'
    o.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(dv.normalized())
    return add_material(o, material)


def prism(name, verts_xz, y, depth, material):
    # A small 3D polygon in the character's front XZ plane.
    n = len(verts_xz)
    verts = [(x,y-depth*.5,z) for x,z in verts_xz] + [(x,y+depth*.5,z) for x,z in verts_xz]
    faces = [tuple(range(n)), tuple(range(2*n-1,n-1,-1))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    return mesh(name, verts, faces, material)


def small_box(name, pos, dimensions, material, bevel=.02):
    bpy.ops.mesh.primitive_cube_add(size=1, location=pos)
    o=bpy.context.object
    o.name=name
    o.dimensions=dimensions
    # IMPORTANT: Apply scale so bevel width is predictable.
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    add_material(o, material)
    if bevel:
        bev=o.modifiers.new('One segment bevel', 'BEVEL')
        bev.width=bevel
        bev.segments=1
        o.modifiers.new('Weighted normals', 'WEIGHTED_NORMAL')
    return o


def ribbon(name, pts, width, material):
    # Thin flat fabric/leather strip, across XZ plane, no floating cubes.
    pts=[Vector(v) for v in pts]
    verts=[]
    for i,p in enumerate(pts):
        t=(pts[min(i+1,len(pts)-1)]-pts[max(0,i-1)])
        normal=Vector((-t.z,0,t.x)).normalized()*width*.5
        verts += [p+normal,p-normal]
    return mesh(name,verts,[(2*i,2*i+1,2*i+3,2*i+2) for i in range(len(pts)-1)],material)

# Pivots are stored at known WORLD coordinates. All parts and pivots are
# created in an unrotated rest pose. We explicitly convert their world
# positions to LOCAL positions rather than relying on Blender's deferred
# evaluation of matrix_world. This fixes parts shifting when parented.
WORLD_PIVOTS={}

def pivot(name, world_pos, parent=None):
    p=bpy.data.objects.new(name,None)
    RIG.objects.link(p)
    p.empty_display_type='PLAIN_AXES'
    p.empty_display_size=.045
    wp=Vector(world_pos)
    WORLD_PIVOTS[p]=wp
    if parent is not None:
        p.parent=parent
        p.location=wp-WORLD_PIVOTS[parent]
    else:
        p.location=wp
    return p


def attach(obj,parent):
    # 'obj' initially has WORLD location and no parent.
    # Its pivot-relative local location is world_location - pivot_position.
    obj.parent=parent
    obj.location=Vector(obj.location)-WORLD_PIVOTS[parent]
    return obj

ROOT=pivot('ROOT / move whole character', (0,0,0))
TORSO=pivot('TORSO / bob and lean', (0,0,1.80), ROOT)
HEAD=pivot('HEAD / tilt and turn', (0,0,2.97), TORSO)

SHOULDER_L=pivot('ARM_L / shoulder', (-.38,0,2.53),TORSO)
ELBOW_L=pivot('ELBOW_L / bend', (-.96,0,2.49),SHOULDER_L)
WRIST_L=pivot('HAND_L / wrist',(-1.45,0,2.49),ELBOW_L)
SHOULDER_R=pivot('ARM_R / shoulder',(.38,0,2.53),TORSO)
ELBOW_R=pivot('ELBOW_R / bend',(.96,0,2.49),SHOULDER_R)
WRIST_R=pivot('HAND_R / wrist',(1.45,0,2.49),ELBOW_R)

HIP_L=pivot('LEG_L / hip',(-.19,0,1.64),ROOT)
KNEE_L=pivot('KNEE_L / bend',(-.20,0,.87),HIP_L)
FOOT_L=pivot('FOOT_L / ankle',(-.22,0,.16),KNEE_L)
HIP_R=pivot('LEG_R / hip',(.19,0,1.64),ROOT)
KNEE_R=pivot('KNEE_R / bend',(.20,0,.87),HIP_R)
FOOT_R=pivot('FOOT_R / ankle',(.22,0,.16),KNEE_R)

#========================= Body and costume ============================================#
def ragged_tunic():
    rings=[(1.67,.39,.27),(2.03,.33,.24),(2.62,.35,.24)]
    sides=12
    verts=[]
    for i,(z,rx,ry) in enumerate(rings):
        for j in range(sides):
            a=2*pi*j/sides
            stagger=(.06 if j in (0,4,8) else -.04 if j in (2,6,10) else 0)
            verts.append((rx*cos(a),ry*sin(a),z+stagger if i==0 else z))
    faces=[]
    for r in range(len(rings)-1):
        for j in range(sides):
            u=r*sides+j; nxt=r*sides+(j+1)%sides
            faces.append((u,nxt,nxt+sides,u+sides))
    faces.append(tuple(range((len(rings)-1)*sides,len(rings)*sides)))
    return mesh('Tunic / tapered ragged shirt',verts,faces,SHIRT)

attach(ragged_tunic(),TORSO)

# Asymmetric red vest panels; these should lie IN FRONT of the shirt,
# but the front cloth remains the main visible mass (not a giant shoulder block).
attach(prism('Vest / left', [(-.32,2.53),(-.07,2.49),(-.10,1.88),(-.27,1.68),(-.41,1.79)], -.285,.050,VEST),TORSO)
attach(prism('Vest / right', [(.08,2.49),(.32,2.53),(.39,1.76),(.24,1.70),(.08,1.91)], -.285,.050,VEST),TORSO)

# Scarf: soft collar and two short hanging fabric tails.
def scarf_ring():
    u,v=22,7
    major,minor=.285,.115
    cz=2.77
    verts=[]
    for i in range(u):
        a=2*pi*i/u
        for j in range(v):
            b=2*pi*j/v
            r=major+minor*cos(b)
            verts.append((r*cos(a),r*sin(a)-.015,cz+minor*sin(b)))
    faces=[]
    for i in range(u):
        for j in range(v):
            faces.append((i*v+j,((i+1)%u)*v+j,((i+1)%u)*v+(j+1)%v,i*v+(j+1)%v))
    return mesh('Scarf / rounded collar',verts,faces,SCARF,True)

attach(scarf_ring(),TORSO)
attach(prism('Scarf / loose left tail', [(-.25,2.72),(-.06,2.72),(-.22,2.18),(-.38,2.04),(-.31,2.39)],-.370,.025,SCARF),TORSO)
attach(prism('Scarf / loose right tail',[(.02,2.72),(.20,2.69),(.29,2.19),(.40,2.09),(.27,2.23)],-.362,.025,SCARF),TORSO)

# Slender leather cross-body strap + compact bag OFF-CENTER.
attach(ribbon('Satchel strap / diagonal', [(-.24,-.325,2.52),(-.02,-.358,2.26),(.27,-.34,1.94),(.54,-.34,1.69)],.066,LEATHER),TORSO)
attach(small_box('Satchel / small leather bag',(.55,-.38,1.64),(.34,.17,.43),LEATHER,.035),TORSO)
attach(small_box('Satchel / front flap',(.55,-.48,1.73),(.35,.055,.15),FLAP,.020),TORSO)
attach(small_box('Satchel / brass clasp',(.55,-.516,1.69),(.045,.02,.052),BUCKLE,.006),TORSO)

# Shorts are two small stubby cuffs, not a single solid skirt.
for s,label,hip,knee,foot in [(-1,'L',HIP_L,KNEE_L,FOOT_L),(1,'R',HIP_R,KNEE_R,FOOT_R)]:
    x=s*.19
    attach(cylinder('Shorts '+label,(x,0,1.42),(x,0,1.77),.157,.174,SHORTS,9),hip)
    attach(cylinder('Rolled shorts hem '+label,(x,0,1.42),(x,0,1.50),.162,.162,VEST,9),hip)
    attach(cylinder('Upper wooden shin '+label,(x,0,1.43),(s*.20,0,.87),.059,.060,WOOD,7),hip)
    attach(ellipsoid('Knee knot '+label,(s*.20,0,.87),(.073,.071,.082),WOOD_L,8,4),knee)
    attach(cylinder('Lower wooden shin '+label,(s*.20,0,.87),(s*.22,0,.17),.056,.054,WOOD,7),knee)
    attach(ellipsoid('Root heel '+label,(s*.22,-.017,.13),(.10,.115,.10),WOOD_D,8,4),foot)
    attach(cylinder('Bent toe '+label,(s*.22,-.02,.12),(s*.24,-.25,.08),.061,.028,WOOD,6),foot)
    attach(cylinder('Point of toe '+label,(s*.24,-.25,.08),(s*.25,-.34,.13),.028,.006,WOOD_L,5),foot)

#========================= Arms and gloves ============================================#
for s,label,shoulder,elbow,wrist in [
    (-1,'L',SHOULDER_L,ELBOW_L,WRIST_L),
    (1,'R',SHOULDER_R,ELBOW_R,WRIST_R),
]:
    attach(cylinder('Small fabric shoulder '+label,(s*.35,0,2.53),(s*.56,0,2.51),.128,.104,SHOULDER,9),shoulder)
    attach(cylinder('Upper twig arm '+label,(s*.52,0,2.51),(s*.96,0,2.49),.047,.052,WOOD,7),shoulder)
    attach(ellipsoid('Elbow knot '+label,(s*.96,0,2.49),(.065,.06,.066),WOOD_L,8,4),elbow)
    attach(cylinder('Forearm twig '+label,(s*.96,0,2.49),(s*1.45,0,2.49),.052,.043,WOOD,7),elbow)
    attach(cylinder('Blue wrist cuff '+label,(s*1.45,0,2.49),(s*1.56,0,2.49),.103,.103,CUFFS,9),wrist)
    attach(ellipsoid('Rounded glove palm '+label,(s*1.62,0,2.49),(.12,.089,.115),GLOVES,10,6,True),wrist)
    # 3 delicate fingers: noticeably smaller than the large boxy palms in v1.
    for k,dz in enumerate((-.075,0,.075)):
        attach(cylinder('Glove finger '+label+' '+str(k),
             (s*1.71,0,2.49+dz),(s*1.88,-.012,2.49+dz*1.05),
             .025,.016,GLOVES,5),wrist)
    attach(cylinder('Glove thumb '+label,(s*1.60,-.035,2.40),(s*1.69,-.065,2.33),.031,.014,GLOVES,5),wrist)

# Standalone staff follows the RIGHT glove via child pivot.
if ADD_STAFF:
    STAFF=pivot('STAFF / removable prop',(1.93,0,2.49),WRIST_R)
    pieces=[
        ((1.93,.02,.20),(1.94,.02,1.45),.040,.045),
        ((1.94,.02,1.45),(1.90,.03,2.58),.045,.040),
        ((1.90,.03,2.58),(1.96,.02,3.50),.040,.052),
        ((1.96,.02,3.50),(1.94,.03,4.46),.052,.030),
        ((1.94,.03,4.46),(2.10,.03,4.68),.030,.012),
        ((1.94,.03,3.77),(2.12,.03,3.93),.023,.012),
        ((2.12,.03,3.93),(2.16,.04,4.08),.012,.005),
    ]
    for i,(a,b,r1,r2) in enumerate(pieces):
        attach(cylinder('Staff branch %02d'%i,a,b,r1,r2,WOOD,6),STAFF)


#========================= Pumpkin head ============================================#
HZ=3.56
RX,RY,RZ=.82,.70,.73
LOBES=10

def head_mesh():
    # 40 radial segments x 12 vertical rings keeps game mesh inexpensive.
    SEG,RINGS=40,12
    verts=[]
    for i in range(RINGS+1):
        v=.003+(pi-.006)*i/RINGS
        z=HZ+RZ*cos(v)
        belt=sin(v)
        for j in range(SEG):
            a=2*pi*j/SEG
            rib=(1.0+.055*cos(LOBES*a))
            verts.append((RX*belt*cos(a)*rib,RY*belt*sin(a)*rib,z))
    faces=[]
    for i in range(RINGS):
        for j in range(SEG):
            k=i*SEG+j; nxt=i*SEG+(j+1)%SEG
            faces.append((k,k+SEG,nxt+SEG,nxt))
    faces.extend([tuple(range(SEG-1,-1,-1)),tuple(range(RINGS*SEG,(RINGS+1)*SEG))])
    return mesh('Pumpkin HEAD / fluted shell',verts,faces,ORANGE,True)

attach(head_mesh(),HEAD)

# Stem top and crooked green vine.
attach(cylinder('Stem trunk',(0,0,4.27),(.075,.03,4.53),.12,.07,STEM,8),HEAD)
attach(cylinder('Stem bent top',(.075,.03,4.53),(-.01,-.01,4.65),.07,.021,WOOD_L,7),HEAD)
attach(cylinder('Stem curling leaf',(.10,.03,4.47),(.29,.06,4.43),.028,.010,STEM,5),HEAD)


def face_surface_y(x,z,push=.05):
    # Negative Y = front of head. Position face geometry on curved surface.
    normalized=1-(x/RX)**2-((z-HZ)/RZ)**2
    yy=-RY*sqrt(max(.08,normalized))
    theta=math.atan2(yy/RY,x/RX)
    return yy*(1.0+.055*cos(LOBES*theta))-push


def carved_face(name, xy, material, push=.06):
    # Triangles fan around the center. Conforms to round pumpkin front.
    verts=[(x,face_surface_y(x,z,push),z) for x,z in xy]
    cx=sum(x for x,z in xy)/len(xy)
    cz=sum(z for x,z in xy)/len(xy)
    verts.append((cx,face_surface_y(cx,cz,push+.008),cz))
    n=len(xy)
    faces=[(i,(i+1)%n,n) for i in range(n)]
    return mesh(name,verts,faces,material)

# Dark, inward-slanted eyes on both sides, with a slightly wider brown rim.
# Unlike v1, there are no vertical cylinders over the eye sockets.
for s,label in [(-1,'L'),(1,'R')]:
    contour=[(.13,3.76),(.36,3.72),(.62,3.82),(.64,3.63),(.52,3.47),(.28,3.48),(.16,3.58)]
    rim=[(s*x,z) for x,z in contour]
    inner=[(s*(x*.94),HZ+(z-HZ)*.92) for x,z in contour]
    attach(carved_face('Carved eye rim '+label,rim,AMBER,.046),HEAD)
    attach(carved_face('Dark eye socket '+label,inner,DARK,.081),HEAD)
    # Angry brow is a slender triangular shape, not a row of sticks.
    brow=[(s*.12,3.81),(s*.61,3.91),(s*.65,3.82),(s*.30,3.73)]
    attach(carved_face('Raised angry brow '+label,brow,ORANGE_DARK,.092),HEAD)

attach(carved_face('Nose / small triangle', [(-.074,3.42),(.074,3.42),(0,3.28)],DARK,.097),HEAD)

# Full jagged scowl. Make quads between two 9-point polylines: this
# avoids overlapping fan triangles that a concave mouth polygon would cause.
scowl_x=[-.41,-.30,-.22,-.11,0,.12,.24,.33,.41]
scowl_upper=[3.20,3.21,3.15,3.18,3.15,3.18,3.15,3.21,3.20]
scowl_lower=[3.10,3.07,3.12,3.08,3.05,3.10,3.07,3.09,3.10]
scowl_verts=[]
for x,zt,zb in zip(scowl_x,scowl_upper,scowl_lower):
    scowl_verts.extend([(x,face_surface_y(x,zt,.081),zt),
                        (x,face_surface_y(x,zb,.081),zb)])
scowl_faces=[(2*i,2*(i+1),2*(i+1)+1,2*i+1) for i in range(len(scowl_x)-1)]
attach(mesh('Jagged pumpkin scowl',scowl_verts,scowl_faces,DARK),HEAD)
for s,label in [(-1,'L'),(1,'R')]:
    attach(carved_face('One tooth '+label,
                       [(s*.22,3.16),(s*.13,3.16),(s*.17,3.06)],ORANGE_HI,.109),HEAD)

#========================= Clean Blender viewport ============================================#
# Hide extra viewport relationship/pivot lines, which made v1 look crowded.
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type=='VIEW_3D':
            sp=area.spaces.active
            sp.overlay.show_relationship_lines=False
            sp.overlay.show_extras=False
            sp.shading.color_type='MATERIAL'

# Set up optional simple camera, lights, and ground only if requested.
if SHOW_EXTRA_SCENE_OBJECTS:
    bpy.ops.object.camera_add(location=(5,-9,4.7))
    cam=bpy.context.object
    cam.name='PREVIEW / camera (not part of character)'
    tgt=Vector((0,0,2.47)); direct=tgt-cam.location
    cam.rotation_euler=direct.to_track_quat('-Z','Y').to_euler()
    cam.data.type='ORTHO'; cam.data.ortho_scale=5.35
    bpy.context.scene.camera=cam
    bpy.ops.object.light_add(type='AREA',location=(-3,-4,6))
    light=bpy.context.object; light.data.energy=1000; light.data.size=5
    light.rotation_euler=(tgt-light.location).to_track_quat('-Z','Y').to_euler()

bpy.ops.object.select_all(action='DESELECT')
ROOT.select_set(True)
bpy.context.view_layer.objects.active=ROOT

# Export root + all descendant pivots/meshes; ignores cameras and lights.
if EXPORT_GLB:
    bpy.ops.object.select_all(action='DESELECT')
    def select_all_children(o):
        o.select_set(True)
        for child in o.children:
            select_all_children(child)
    select_all_children(ROOT)
    bpy.context.view_layer.objects.active=ROOT
    bpy.ops.export_scene.gltf(filepath=GLB_PATH,export_format='GLB',use_selection=True,
                              export_apply=False,export_materials='EXPORT')
    print('Exported GLB to',GLB_PATH)

print('SUCCESS: Pumpkin Scarecrow v2 is in the scene.')
print('Mesh objects:',sum(1 for o in bpy.data.objects if o.type=='MESH'))
print('For controls expand ROOT in Outliner; rotate ARM_L / ARM_R / LEG_L / LEG_R.')
