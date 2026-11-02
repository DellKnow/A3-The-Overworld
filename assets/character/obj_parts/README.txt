Pumpkin Scarecrow -- articulated OBJ meshes for CSCI441 OpenGL

Use CSCI441::ModelLoader on each OBJ. The MTL with the same name is read automatically.
Each OBJ contains all colors/materials for one animated part.
The parts.csv offset is relative to its parent joint, in OpenGL Y-up coordinates.
Geometry is centered at its own joint, not at the global world origin.
Construct matrices: parentMatrix * translate(offset) * rotate(animatedAngle).
ROOT has no OBJ: it is the character placement node.
For CSCI441::ModelLoader::draw, use vec4 material uniforms (it uploads 4 floats).
In your shader use materialDiffColor.rgb / materialAmbColor.rgb / materialSpecColor.rgb.
Set useTexture=0 for these untextured meshes.

The exported character faces +Z (Blender forward -Y). Ground is at Y=0.
Files: 15 OBJ, total 3009 triangles.
