#===================================================================================#
#               PUMPKIN SCARECROW — A3 OpenGL character generator                   #
#===================================================================================#

Explanation of how to use python generator for Blender character.

FILES: pumpkin_scarecrow_A3.py  — Python script for Blender 4.2 / 5.x

The angry eyes and mouth are layered dark inset meshes, not Boolean holes.
This makes the model more robust and saves polygons.
The SET / camera, lights, and backdrop are intentionally not attached to
ROOT and are omitted by the optional selected-hierarchy GLB export.

#===================================================================================#
#                           TO CREATE THE CHARACTER                                 #
#===================================================================================#

1. Open a NEW Blender project (the script clears the current scene).
2. Scripting workspace > Text Editor > Open > pumpkin_scarecrow_A3.py.
3. Press Alt+P (Run Script), with mouse over the Text Editor.
4. Go to Layout and select 'ROOT | animate entire character'.
5. Save your scene with File > Save As > PumpkinScarecrow.blend.

THE CHARACTERS' FACE IS POINTED TOWARD NEGATIVE Y.

#===================================================================================#
#                                 HIERARCHY                                         #
#===================================================================================#

HIERARCHY
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

#===================================================================================#
#                               POLYGON COUNT                                       #
#===================================================================================#

+-------------------------------+-----------+
| Component                     | Triangles |
+-------------------------------+-----------+

| Pumpkin head                  |     1,036 |
| Face, stem, and details       |       129 |
| Body, clothes, scarf, satchel |       568 |
| Both arms and hands           |       648 |
| Both legs and feet            |       488 |
| Wooden staff                  |       140 |
+-------------------------------+-----------+

| Total                         |     3,009 |
+-------------------------------+-----------+
