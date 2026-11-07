# A3 — DellKnow in 3D

- **Name:** Mark Suchodolskyi
- **Hero:** DellKnow
- **Assignment:** A3

---

## Description

DellKnow is back, now in 3D: a pumpkin-headed scarecrow with a wooden staff,
walking around a snowy Minecraft-style island. The world has mountains, roads,
houses, trees, icy water, and clouds that slowly move across the sky. Explore the
island from third person or first person, fly around freely, or inspect the hero
up close. There is also an optional second view in the corner of the screen.

---

## How to Play

### Camera Views

| Key | Action |
|:---:|---|
| `1` | Third-person arcball camera (follows the character) |
| `2` | First-person camera (moves with the character) |
| `3` | Free-flying camera (moves independently of the character) |
| `4` | Inspect camera (orbit and zoom around the character) |
| `5` | Show/hide the second camera view in the top-right quarter |

> **Note on view 5:** when the main view is 1, the inset shows 2; when the main
> view is 2, the inset shows 1. It starts off and is intended for views 1 and 2.

### Movement

| Key | Action |
|:---:|---|
| `W` / `S` | Forward / backward in views 1, 2, and 3 |
| `A` / `D` | Left / right in views 1, 2, and 3 |
| `W` / `S` *(view 4)* | Move the inspect camera's focus up/down the character |
| `W` + `A`, etc. | Diagonal movement works; speed is normalized |

### Mouse

| Input | Action |
|:---:|---|
| Left drag | Orbit / look around using the active camera |
| `Shift` + left drag | Zoom in/out in views other than 2 (the first-person camera rotates instead) |
| Scroll wheel | Zoom or move camera forward/back (disabled in view 2) |

### Display / Extra Options

| Key | Action |
|:---:|---|
| `G` | Toggle the ground grid |
| `T` | Toggle world textures on/off |
| `L` | Toggle unlit world rendering |
| `C` | Toggle the character's separate unlit mode (view 3) |
| `X` | Toggle X-ray / wireframe world (view 3) |
| `F11` | Toggle fullscreen |
| `Q` / `Esc` | Quit |

---

## What to Look For

- **Hierarchical model** — the 3D pumpkin has a separate head, torso, shoulders,
  elbows, hands, hips, knees, feet, and staff. Rotating one joint moves
  everything below it.
- **Walking** — opposite arms and legs swing, the knees bend, the feet stay
  level, the body bobs, leans forward, and twists, and the staff stays upright
  with its tip near the ground. The pose blends smoothly in and out when you
  start and stop.
- **Idle** — after 2 seconds without movement, the hero starts breathing,
  looking around, and tilting the head, with a gentle arm sway.
- **World** — a block-built snowy island with mountain tiers, roads, houses,
  trees, and water. The character cannot walk into buildings or trees, enter
  water, leave the map, or climb a step higher than one block.
- **Lighting** — full Phong illumination (ambient + diffuse + specular),
  calculated using Gouraud shading. Different model parts have different
  material coefficients.
- **Textures** — Minecraft-style block textures can be switched on/off with `T`.
  The cube's face normals choose the texture projection, so tall blocks repeat
  their textures instead of stretching them vertically.
- **Sky** — a blue background and 50 randomized flat clouds of different widths,
  depths, positions, and heights. They drift along X and wrap around.
- **Cameras** — four separate modes, including a close-up inspect view and a
  first-person camera without zoom.
- **Doom iddt** — press `5` to show the opposite (1/2) camera in the top-right
  quarter while exploring. Press `5` again to hide it.
- **Stats** — frame-rate and frame-time statistics are displayed on screen.

---

## Compiling

Build and run:

```powershell
cmake -S . -B build -G "MinGW Makefiles" ; cmake --build build ; .\build\a3.exe
```

Refresh CMake:

```powershell
cmake --fresh -S . -B build -G "MinGW Makefiles"
```

**Notes**

- The CMake configuration requires the course's OpenGL / `Machine_Setup`
  libraries (GLFW, GLAD, GLM, and CSCI441). Check the local path to
  `OpenGLMake.cmake` or `Machine_Setup` in `CMakeLists.txt` and adjust it for
  your computer.
- Keep the shader files, OBJ/MTL models, `parts.csv`, textures, and font in their
  relative `assets/` and `shaders/` directories. The application loads these
  files at runtime, so **launch it from the project directory**.
- Make sure `CMakeLists.txt` includes `Character.cpp`, `World.cpp`,
  `A3Engine.cpp`, and any other added source files.

---

## Implementation Details

### Model

The 2D pumpkin from A2 became a 3D scarecrow. The meshes were generated in
Blender and exported as separate OBJ parts with MTL files, so each movable piece
can contain multiple colors without manually drawing primitives.

The character hierarchy is:

```text
ROOT/                       (whole character position and heading)
├── TORSO/                  (body and clothes)
│   ├── HEAD/               (pumpkin, face, stem)
│   ├── ARM_L/              (left shoulder)
│   │   └── ELBOW_L/
│   │       └── HAND_L/
│   └── ARM_R/              (right shoulder)
│       └── ELBOW_R/
│           └── HAND_R/
│               └── STAFF/
├── LEG_L/                  (left hip)
│   └── KNEE_L/
│       └── FOOT_L/
└── LEG_R/                  (right hip)
    └── KNEE_R/
        └── FOOT_R/
```

Each `Part` contains an optional `CharacterMesh`, an offset from its parent, an
animated shift, a rotation quaternion, and the indices of its children. `ROOT`
does not need a mesh. The joint positions come from `parts.csv`. Drawing is
recursive:

```text
parent matrix × translate(offset + shift) × rotation  →  draw mesh, then children
```

`CharacterMesh` wraps `CSCI441::ModelLoader`. The loader reads OBJ vertices,
normals, and material groups, sets up VAO/VBO data, and draws the groups using
material values from the MTL files. The character and world share the Gouraud
shader, but each mesh/material gets its own color and reflection coefficients.

### Animation

`update(dt, distanceMoved)` runs every frame. The walk phase advances with the
distance actually walked, so the feet don't slide and walking into a wall stops
the stride. Opposite arms and legs swing using `sin(phase)`, the knees bend
during alternating halves of a step, the feet counter-rotate to stay level, and
the body bobs twice per stride. The shoulder resting angles keep the arms
lowered instead of the original T-pose.

Walking and idling are weights between 0 and 1 that move toward their targets
with `1 − e^(−speed·dt)`, so every transition is smooth and frame-rate
independent. After two seconds standing still, the idle weight fades in:
breathing, looking around, head tilt, and arm sway.

The staff is a child of the right hand, but each frame its local transform is
solved so that it stays upright with its tip near the ground: the hand matrix is
found in `ROOT` space, the staff is placed there, then converted back with the
inverse hand matrix.

### World

`World` builds a 128 × 128 block grid around the origin. Terrain heights form a
snowy island with stone mountain layers, paths, buildings, and randomly placed
trees. Blocks are grouped by `BlockType`, so materials and textures can be bound
once per type before drawing its cubes.

The hero starts at a valid open position. Movement checks whether the next
location is walkable. The test rejects water, solid obstacles, locations outside
the map, and climbs higher than one block. The character's Y position follows
the ground height.

### Shaders and Textures

A vertex and fragment shader implement Phong illumination with Gouraud shading.
Ambient, diffuse, specular, and shininess come from the world materials or the
OBJ/MTL model materials.

World textures are sampled in the fragment shader. The vertex shader projects
`(u, v)` from world position based on the cube face normal.
`CSCI441::drawSolidCubeFlat()` is used for textured blocks, because flat face
normals give the correct projection on each cube side. `GL_REPEAT` repeats the
texture on scaled columns, and nearest-neighbor filtering keeps the Minecraft
pixel-art look. Transparent leaf pixels are cut out.

The flat shader draws the sun, grid, and cloud shapes. Clouds have a fixed
thickness, random dimensions and heights, and cyclic movement along the X axis.
Alpha blending makes the clouds partly transparent. This is a simple sky effect,
not a six-face cubemap skybox.

### Cameras and Viewports

| View | Camera | Description |
|:---:|---|---|
| 1 | `ArcballCam` | Follows the character and can orbit around it |
| 2 | `FreeCam` | Placed at the character's eye height, slightly forward of the pumpkin; its direction controls first-person movement |
| 3 | `FreeCam` | Separate camera for exploring the world independently |
| 4 | `ArcballCam` | Second arcball for inspecting the hero up close; `W`/`S` changes the orbit target's height |

The extra corner viewport uses `glViewport` and a scissor-limited clear to render
the other follow camera in the top-right quarter. The main viewport is restored
afterward. This is the **Doom iddt** extra feature.

### Extras

- All movement is time-based. Diagonal directions are normalized.
- Key toggles are handled on `GLFW_PRESS`, so one press toggles once.
- The game starts with the main third-person camera and the inset off.
- Texture, grid, unlit, and X-ray modes can be enabled independently.
- `F11` switches between windowed and fullscreen mode.
- FPS and milliseconds per frame are shown as a text overlay.

---

## Questions

**How long did this assignment take you?**

> Around 20–25 hours. I was perfecting it too much.

**How much did the lab help you for this assignment?** *(1 — did not help at all, 10 — this was exactly the same as the lab)*

> **10** — I straight up copied the lab07 template here. Also, the free cam idea
> from lab02 as well.

**How fun was this assignment?** *(1 — discontinue this assignment, 10 — I wish I had more time to make it even better)*

> **10** — though I think it is perfect. There are obviously lots of things to
> possibly add, but it already looks good.
