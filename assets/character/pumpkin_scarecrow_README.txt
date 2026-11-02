PUMPKIN SCARECROW — A3 OpenGL character generator
=================================================

FILES
  pumpkin_scarecrow_A3.py  — Python script for Blender 4.2 / 5.x

TO CREATE THE CHARACTER
  1. Open a NEW Blender project (the script clears the current scene).
  2. Scripting workspace > Text Editor > Open > pumpkin_scarecrow_A3.py.
  3. Press Alt+P (Run Script), with mouse over the Text Editor.
  4. Go to Layout and select 'ROOT | animate entire character'.
  5. Save your scene with File > Save As > PumpkinScarecrow.blend.

THE CHARACTERS' FACE IS POINTED TOWARD NEGATIVE Y.

HIERARCHY (look in the Outliner)
  ROOT | animate entire character
    TORSO | sway
      Tunic, vest, scarf, satchel, etc.
      HEAD | look left/right
        Pumpkin shell, carved face, stem, brows
      ARM_L / ARM_R | shoulder swing
        upper arm and padded shoulder
        ELBOW_L / ELBOW_R | bend
          forearm
          HAND_L / HAND_R | wrist
            glove, fingers, thumb
            STAFF | optional prop in right hand
    LEG_L / LEG_R | hip
      shorts, upper twig leg
      KNEE_L / KNEE_R | bend
        lower twig leg
        FOOT_L / FOOT_R | ankle
          shoes / root-like toes

All parts are regular, independent mesh objects. Parent pivots are Empties.
No armature is required. No baked animation.

BLENDER ANIMATION TEST
  Select a shoulder or hip Empty. Rotate it in local Y around +/-20 degrees.
  Its child joints and meshes move with it.

OPTIONAL EXPORT TO OPENGL
  The best format for hierarchy is GLB/glTF, which preserves nodes and
  material colors. Set EXPORT_GLB=True in the .py file, choose a path, and
  rerun the script. You can also use File > Export > glTF 2.0.

  For OpenGL (C++), load the .glb via Assimp (Importer::ReadFile).
  Assimp's aiNode hierarchy corresponds to Blender's empties/meshes.
  Create GPU buffers from each aiMesh (vertices, normals, indices) and
  traverse aiNodes with parent * local transforms.
  The walking animation belongs in C++: rotate shoulder/hip nodes and their
  child elbow/knee/wrist nodes with GLM matrices. Avoid baking a merged mesh.

  OBJ does NOT retain this hierarchy; export individual pieces and rebuild
  their pivot offsets in C++ if you must use OBJ.

  Blender uses Z up and glTF uses Y up. Assimp/glTF export may contain a
  root axis conversion; honor the imported node transforms instead of
  assuming mesh coordinates directly match your OpenGL world.

POLYGON COUNT
  Intentionally low-poly. The ribbed pumpkin head is the main source of
  polygons; lower HEAD_SEGMENTS (multiple of 10, e.g. 40) and HEAD_RINGS
  (e.g. 10) for fewer triangles.

NOTES
  The angry eyes and mouth are layered dark inset meshes, not Boolean holes.
  This makes the model more robust and saves polygons.
  The SET / camera, lights, and backdrop are intentionally not attached to
  ROOT and are omitted by the optional selected-hierarchy GLB export.
  Some visual details rely on material colors that you can replace in Blender.
  Check your class assignment's restrictions on imported geometry before
  using it as the submitted rendering implementation.
