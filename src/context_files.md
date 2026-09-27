# Project Architecture Guide

This document outlines the structure of the 3D OpenGL project, explaining the purpose of each file and folder. Use the "Where to Look" section to quickly navigate the codebase when building, fixing, or modifying features.

---

## 📂 Directory & File Overview

### Root Directory (`./`)
* **`main.cpp`**: The entry point of the application. Handles window creation (GLFW), capturing mouse/keyboard inputs, initializing OpenGL, and running the main application loop.
* **`glad.c`**: The OpenGL function loader. It acts as a bridge between your hardware's OpenGL drivers and your C++ code. You should never need to edit this.
* **`apartment`**: Likely the compiled executable binary for your project.
* **`project_rules.md`**: Text file containing project-specific rules, guidelines, or prompts for AI agents.
* **`Untitled document.txt`**: Scratchpad files.

### 📂 `include/`
Contains third-party header files required to run OpenGL. 
* **`glad/glad.h`** & **`KHR/khrplatform.h`**: Headers for the GLAD loader. You do not need to touch these.

### 📂 `shaders/`
Contains GLSL (OpenGL Shading Language) programs that run directly on the GPU.
* **`core.vert` / `core.frag`**: Standard vertex and fragment shaders. Likely used for basic geometric rendering, UI, or debugging grids.
* **`room.vert`**: The vertex shader specifically for the room geometry. It takes the local 3D coordinates from C++ and applies the Model/View/Projection matrices.
* **`room.frag`**: The fragment shader for the room. This is the **most important file for visuals**. It handles applying solid colors to the walls and executing the `discard` logic to cut holes (doors/windows) based on local 2D coordinates.

### 📂 `src/`
The core C++ source code for the engine and application logic.
* **`Camera.h`**: Manages the 3D camera. Handles the View matrix, looking around (mouse input), and moving through the world (keyboard input).
* **`Geometry.h` / `Geometry.cpp`**: The mathematical heart of the project. Contains the structs (`RoomParams`, `RoomFace`, `Hole`) and the logic to generate raw 3D vertex data (rectangles/planes) centered at `(0,0,0)`.
* **`Scene.h` / `Scene.cpp`**: The director of the project. This manages the placement and rendering of objects. It takes the raw models from `Geometry`, applies Model matrices (position/rotation/scale), binds the correct shaders, and draws them to the screen.
* **`Shader.h` / `Shader.cpp`**: A utility class to easily load, compile, and link the `.vert` and `.frag` files from the `shaders/` folder. It also handles sending variables (Uniforms) like matrices and colors to the GPU.
* **`context_files.md`**: Additional project documentation or AI context logic.

---

## 🎯 "Where to Look" Guide (Task-Based Navigation)

### 1. I want to change how a Room is built (Math/Vertices)
👉 **Look in:** `src/Geometry.cpp` and `src/Geometry.h`
* **Why:** If you want to change how the vertex data is generated, add new types of shapes (like a slanted roof), or modify the `RoomParams` struct, this is the file. Remember, geometry is always generated at `(0,0,0)`.

### 2. I want to place, move, or rotate a Room in the world
👉 **Look in:** `src/Scene.cpp`
* **Why:** This is where Model Matrices are applied. If you want to move a room 10 units to the right, or rotate it 90 degrees, you modify the `glm::mat4 model` transformations right before the `draw` call in this file.

### 3. I want to fix a bug with doors/windows not cutting out properly
👉 **Look in:** `shaders/room.frag`
* **Why:** The holes are not real 3D geometry; they are created by discarding pixels in the fragment shader. Check the math in this file that checks if a pixel is inside a hole's bounds.

### 4. I want to change the color, lighting, or visual style of the walls
👉 **Look in:** `shaders/room.frag` and `shaders/core.frag`
* **Why:** All final pixel colors are determined here. If you want to add textures, lighting calculations, or change how colors are processed, edit these shaders. Keep in mind that we now utilize a universal `useVertexColor` flag! If `useVertexColor` is mathematically set to true natively by `ProceduralMesh` or `RoomMesh`, then the fragment evaluates the intrinsic 11-float vertex layout. If false, it acts as a fallback and evaluates the solid `objectColor` uniform instead!

### 5. I want to change how fast the camera moves or look sensitivity
👉 **Look in:** `src/Camera.h` (and possibly `main.cpp` for input handling)
* **Why:** The camera class stores the `movementSpeed` and `mouseSensitivity` variables. 

### 6. I want to pass new data to the GPU (like a new uniform variable)
👉 **Look in:** `src/Shader.cpp` & `src/Scene.cpp`
* **Why:** First, ensure the variable is defined in your `.vert` or `.frag` file. Then, use the `Shader` class methods (e.g., `shader.setVec3("newColor", value)`) inside your rendering loop in `Scene.cpp` to send the data.



g++ main.cpp glad.c src/*.cpp -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -o apartment
g++ main.cpp glad.c src/*.cpp -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -o apartment
g++ main.cpp glad.c src/*.cpp -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl -o apartment 