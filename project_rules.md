# Apartment Project - Core Architecture & Rules

## 1. Pipeline constraints
- **STRICT Modern OpenGL (3.3 Core Profile):** Use GLFW, GLAD, and GLM.
- **NO Legacy OpenGL:** No `glBegin`, `glEnd`, `glPushMatrix`, `glPopMatrix`, `glTranslate`, `glRotate`.
- **NO Fixed-Function Pipeline:** All lighting, colors, and transformations MUST be done via Shaders (`.vert`, `.frag`).
- **VAO/VBOs ONLY:** Geometry must be uploaded via Vertex Array Objects and Vertex Buffer Objects during initialization.

## 2. Geometry & Placement Math
- **Y=0 Local Anchor:** EVERY object must be modeled as if its exact center is at `X=0, Z=0` and its absolute bottom touches `Y=0`.
- **GLM Math:** Model matrices must be calculated using GLM and the **Translate -> Rotate -> Scale (SRT)** ordering.
- **Master Placement Function:** Use a global placement function `placeObject()` to place anchors from local space to global space.

## 3. Data-Driven Output & Modularity
- **Decoupled Logic:** Separate the geometry data from the layout. Define configurations (room layouts, sizing variables) globally at the top of the file/struct.
- **Testbed Mode:** Always maintain a toggle (e.g. `RenderMode`) to switch between the Full World view and a Testbed view (rendering a single item at `0,0,0`).

## 4. Debugging & AI Anchor Tags
- Provide anchor tags in code to separate components for easy lookup:
  `// === [DEBUG_ID: OBJECT_NAME] ===`
- Keep variables/parameters detached from the vertex array generation so they can be changed quickly without digging through math.
- **Object Function Comments**: Each object making function should have a comment indicating the object name (e.g., `// table function`).
- **Placement Comments**: Each time an object is placed, there should be a `// place object` comment, so instances can be found by searching.

## 5. Course Requirements Checklist
- [ ] **Curves** (mathematically generated vertices for ropes/arches)
- [ ] **Textures** (`sampler2D` in frag shaders for wood, brick, etc.)
- [ ] **Viewing** (Camera with `glm::lookAt` and `glm::perspective`)
- [ ] **Lightings** (Custom Phong Shader model, managed via a room-aware Lighting Manager)
- [ ] **Fractals** (Mathematical shader fractal on a 2D quad/TV, or 3D L-System tree)
- [ ] **Animation/Interaction** (Variables updated via time/keyboard affecting model matrices)
