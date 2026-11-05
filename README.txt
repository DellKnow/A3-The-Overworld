#===================================================================================#
#                                    Name                                           #
#===================================================================================#

Name: Mark Suchodolskyi
Hero: DellKnow

#===================================================================================#
#                                  Homework                                         #
#===================================================================================#

Lab: A3

#===================================================================================#
#                                Description                                        #
#===================================================================================#

DellKnow is back, now in 3D: a pumpkin-headed scarecrow with a wooden staff,
walking around a snowy Minecraft-style island. The world has mountains, roads,
houses, trees, icy water, and clouds that slowly move across the sky. Explore the
island from third person or first person, fly around freely, or inspect the hero
up close. There is also an optional second view in the corner of the screen.

HOW TO PLAY

  CAMERA VIEWS
    1                 Third-person arcball camera (follow the character)
    2                 First-person camera (move with the character)
    3                 Free-flying camera (move independently of the character)
    4                 Inspect camera (orbit and zoom around the character)
    5                 Show/hide the second camera view in the top-right quarter
                      (when the main view is 1, the inset shows 2; for 2, it shows 1)
                      (starts off; intended for views 1 and 2)

  MOVEMENT
    W / S             Forward / backward in views 1, 2, and 3
    A / D             Left / right in views 1, 2, and 3
    W / S in view 4   Move the inspect camera's focus up/down the character
    W + A, etc.      Diagonal movement works; speed is normalized

  MOUSE
    Left drag         Orbit / look around using the active camera
    Shift + left drag Zoom in/out in views other than 2
    Scroll wheel      Zoom or move camera forward/back (disabled in view 2)
                      First-person camera rotates rather than zooming with Shift + drag

  DISPLAY / EXTRA OPTIONS
    G                 Toggle the ground grid
    T                 Toggle world textures on/off
    L                 Toggle unlit world rendering
    C                 Toggle the character's separate unlit mode (view 3)
    X                 Toggle X-ray / wireframe world (view 3)
    F11               Toggle fullscreen
    Q / Esc           Quit

WHAT TO LOOK FOR

  - The 3D pumpkin has a separate head, torso, shoulders, elbows, hands,
    hips, knees, feet, and staff. Rotating one joint moves everything below it.
  - Walking: opposite arms and legs swing, the knees bend, and the motion
    continues through the last part of a step after you release the keys.
  - Idle: after 2 seconds without movement, the hero starts making small
    motions, including head movement and gentle arm/body sway.
  - World: a block-built snowy island with mountain tiers, roads, houses,
    trees, and water. The character cannot walk into buildings or trees,
    enter water, leave the map, or climb a step higher than one block.
  - Lighting: full Phong illumination (ambient + diffuse + specular),
    calculated using Gouraud shading. Different model parts have different
    material coefficients.
  - Textures: Minecraft-style block textures can be switched on/off with T.
    The cube's face normals choose the texture projection, so tall blocks
    repeat their textures instead of stretching them vertically.
  - Sky: a blue background and 50 randomized flat clouds of different widths,
    depths, positions, and heights. They drift along X and wrap around.
  - Cameras: four separate modes, including a close-up inspect view and a
    first-person camera without zoom.
  - Doom iddt: press 5 to show the opposite (1/2) camera in the top-right
    quarter while exploring. Press 5 again to hide it.
  - Frame-rate and frame-time statistics are displayed on screen.


#===================================================================================#
#                          Instructions on Compiling                                #
#===================================================================================#

Run command:
cmake -S . -B build -G "MinGW Makefiles" ; cmake --build build ; .\build\a3.exe

Refresh cmake:
cmake --fresh -S . -B build -G "MinGW Makefiles"

The CMake configuration requires the course's OpenGL / Machine_Setup libraries
(GLFW, GLAD, GLM, and CSCI441). Check the local path to OpenGLMake.cmake or
Machine_Setup in CMakeLists.txt and adjust it for your computer.

Keep the shader files, OBJ/MTL models, parts.csv, textures, and font in their
relative assets/ and shaders/ directories. The application loads these files
at runtime, so launch it from the project directory.

Make sure the current CMakeLists.txt includes Character.cpp, World.cpp,
A3Engine.cpp, and any other added source files.

#===================================================================================#
#                           Implementation details                                  #
#===================================================================================#

MODEL

    The 2D pumpkin from A2 became a 3D scarecrow. The meshes were generated in
    Blender and exported as separate OBJ parts with MTL files, so each movable
    piece can contain multiple colors without manually drawing primitives.

    The character hierarchy is:

    ROOT/                               (whole character position and heading)
        TORSO/                          (body and clothes)
            HEAD/                       (pumpkin, face, stem)
            ARM_L/                     (left shoulder)
                ELBOW_L/
                    HAND_L/
            ARM_R/                     (right shoulder)
                ELBOW_R/
                    HAND_R/
                        STAFF/
        LEG_L/                          (left hip)
            KNEE_L/
                FOOT_L/
        LEG_R/                          (right hip)
            KNEE_R/
                FOOT_R/

    Each Part contains an optional CharacterMesh, offset from its parent,
    rotation axis, angle, and indices of its children. ROOT does not need a
    mesh. The joint positions come from parts.csv. Drawing is recursive:
    parent matrix * local translation * local rotation, then draw the mesh
    and its children with the resulting transformation.

    CharacterMesh wraps CSCI441::ModelLoader. The loader reads OBJ vertices,
    normals, and material groups, sets up VAO/VBO data, and draws the groups
    using material values from the MTL files. The character and world share
    the Gouraud shader, but each mesh/material gets its own color and
    reflection coefficients.

ANIMATION

    Movement and animation use elapsed time (dt), not frame count.
    Opposite arms and legs swing using sin(phase). The knees bend during
    alternating halves of a step. The shoulder resting angles keep the arms
    lowered instead of returning to the original T-pose.

    When movement stops, the remaining step animation can finish. After two
    seconds without movement, updateIdle() adds subtle motion to the head,
    torso, and arms. The staff is attached through the right hand, so it
    follows the right arm's hierarchy.

WORLD

    World builds a 128 x 128 block grid around the origin. Terrain heights
    form a snowy island with stone mountain layers, paths, buildings, and
    randomly placed trees. Blocks are grouped by BlockType, so materials
    and textures can be bound once per type before drawing its cubes.

    The hero starts at a valid open position. Movement checks whether the
    next location is walkable. The test rejects water, solid obstacles,
    locations outside the map, and climbs higher than one block. The
    character's Y position follows the ground height.

SHADERS AND TEXTURES

    A vertex and fragment shader implement Phong illumination with Gouraud
    shading. Ambient, diffuse, specular, and shininess come from the world
    materials or the OBJ/MTL model materials.

    World textures are sampled in the fragment shader. The vertex shader
    projects (u, v) from world position based on the cube face normal.
    CSCI441::drawSolidCubeFlat() is used for textured blocks, because flat
    face normals give the correct projection on each cube side. GL_REPEAT
    repeats the texture on scaled columns, and nearest-neighbor filtering
    keeps the Minecraft pixel-art look. Transparent leaf pixels are cut out.

    The flat shader draws the sun, grid, and cloud shapes. Clouds have a
    fixed thickness, random dimensions and heights, and cyclic movement
    along the X axis. Alpha blending makes the clouds partly transparent.
    This is a simple sky effect, not a six-face cubemap skybox.

CAMERAS AND VIEWPORTS

    View 1: ArcballCam follows the character and can orbit around it.
    View 2: FreeCam placed at the character's eye height, slightly forward
            of the pumpkin. Its direction controls first-person movement.
    View 3: separate FreeCam for exploring the world independently.
    View 4: second ArcballCam for inspecting the hero more closely; W/S
            changes the orbit target's height.

    The extra corner viewport uses glViewport and a scissor-limited clear
    to render the other follow camera in the top-right quarter. The main
    viewport is restored afterward. This is the Doom iddt extra feature.

EXTRAS

    - All movement is time-based. Diagonal directions are normalized.
    - Key toggles are handled on GLFW_PRESS, so one press toggles once.
    - The game starts with the main third-person camera and the inset off.
    - Texture, grid, unlit, and X-ray modes can be enabled independently.
    - F11 changes between windowed and fullscreen mode.
    - FPS and milliseconds per frame are shown as a text overlay.

#===================================================================================#
#                                    Questions                                      #
#===================================================================================#

How long did this assignment take you?
	
	Around 20-25 hours. I was perfecting it too much.

How much did the lab help you for this assignment? 1-10 (1 - did not help at all, 10 - this was exactly the same as the lab)
	
	10, I straight up copied lab07 template here. Also, the free cam idea from lab02 as well.

How fun was this assignment? 1-10 (1 - discontinue this assignment, 10 - I wish I had more time to make it even better)
	
	10, thought I think it is perfect. There obviously lots of think to possibly add, but it already looks good.