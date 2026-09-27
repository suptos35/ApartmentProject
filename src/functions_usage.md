<!-- Room generation function -->

***

### How to Build Your Own Room
I have already placed a working example near the top of your [main() ](cci:1://file:///home/sup35/Documents/ApartmentProject/main.cpp:81:0-206:1) function (around `Line 106`), but here is exactly how the system works and how you can define your own rooms!

#### Step 1: Initialize the Size
You create a [RoomParams](cci:2://file:///home/sup35/Documents/ApartmentProject/src/Scene.h:22:0-27:1) object and give it the outer dimensions. (These are absolute OpenGL units).
```cpp
RoomParams myRoom;
myRoom.width = 8.0f;   // Length along the X axis
myRoom.height = 3.0f;  // Height along the Y axis
myRoom.depth = 6.0f;   // Depth along the Z axis
```

#### Step 2: Toggle Visibility
The [RoomFace](cci:2://file:///home/sup35/Documents/ApartmentProject/src/Scene.h:16:0-20:1) array has exactly 6 items, mapped to specific sides. If you don't want a wall to be drawn at all, you set `isVisible` to `false`. 
* `faces[0]`: Front (Z+)
* `faces[1]`: Back (Z-)
* `faces[2]`: Left (X-)
* `faces[3]`: Right (X+)
* `faces[4]`: Top / Ceiling (Y+)
* `faces[5]`: Bottom / Floor (Y-)

For example, to hide the right wall so you can look inside like a dollhouse:
```cpp
for (int i = 0; i < 6; ++i) { 
    myRoom.faces[i].isVisible = true; // Turn them all on first
}
myRoom.faces[3].isVisible = false;    // Turn just the Right wall off
```

#### Step 3: Set Face Colors
You assign a solid `glm::vec3(Red, Green, Blue)` color to each face.
```cpp
myRoom.faces[0].color = glm::vec3(0.8f, 0.3f, 0.3f); // Red front wall
myRoom.faces[4].color = glm::vec3(0.9f, 0.9f, 0.9f); // Bright ceiling
```

#### Step 4: Add Holes (Windows/Doors/Passageways)
You can punch interactive holes in any face by pushing a rule into its `holes` array. 
The system requires 7 parameters per hole: `{ X, Y, Width, Height, Type, IsOpen, "Label" }`.

* **X and Y**: Coordinates starting from `0.0` at the **Bottom-Left corner** of the local 2D wall.
* **Type Enum**: `Hole::WINDOW`, `Hole::DOOR`, or `Hole::EDGE_CONNECT`.
  * *Note: `EDGE_CONNECT` is for permanent structural pass-throughs (like archways or floor cutouts) and cannot be closed.*
* **IsOpen**: A boolean `true` (see-through) or `false` (renders closed panel in wood/glass). 
* **Label**: A string identifier printed to the terminal when the user presses `E` to interact.

```cpp
// Add an interactive Door to the Front Wall
// Centers a 2.0x3.0 door on the floor. Spawns CLOSED (false).
myRoom.faces[0].holes.push_back({ 3.0f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "My Front Door" });

// Add a Window to the Back Wall
// Raised 1.0 unit (Y = 1.0f). Spawns OPEN (true).
myRoom.faces[1].holes.push_back({ 2.5f, 1.0f, 3.0f, 1.0f, Hole::WINDOW, true, "My Window" });

// Add a structural opening block for connecting to another room
myRoom.faces[2].holes.push_back({ 0.0f, 0.0f, 8.0f, 6.0f, Hole::EDGE_CONNECT, true, "" });
```
#### Step 5: Generate and Draw
Once your [RoomParams](cci:2://file:///home/sup35/Documents/ApartmentProject/src/Scene.h:22:0-27:1) blueprint is fully designed, you convert it into actual graphical vertex data, which you can then draw continuously in your render loop.

**Run once at launch (Already on `Line 134` of [main.cpp](cci:7://file:///home/sup35/Documents/ApartmentProject/main.cpp:0:0-0:0)):**
```cpp
// This reads your measurements and builds the actual VAO!
RoomMesh generatedRoom = generateRoomGeometry(myRoom);
```

**Run inside `RENDER_TESTBED` or `RENDER_WORLD` in your render loop:**
```cpp
Transform testbedRoom;
testbedRoom.position = glm::vec3(0.0f, 0.0f, 0.0f);
testbedRoom.rotation = glm::vec3(0.0f, 45.0f, 0.0f);

placeObject(roomShader, generatedRoom, testbedRoom);
```

***

### How to Build a Staircase
Stairs are built using a 3D "bounding-box" division system. You define the total dimensions the staircase will take up, and the engine automatically calculates the step dimensions.

#### Step 1: Define the Bounding Box
```cpp
StairParams myStairs;
myStairs.width = 3.0f;    // How wide the staircase is (X axis limit)
myStairs.height = 4.0f;   // Total absolute rise from floor to top landing (Y axis limit)
myStairs.depth = 6.0f;    // Total run length extending forward into the room (Z axis limit)
```

#### Step 2: Set the Step Count and Colors
```cpp
myStairs.numSteps = 10;   // The engine automatically calculates rise=0.4 and run=0.6 per step
myStairs.riserColor = glm::vec3(0.75f, 0.60f, 0.40f); // Vertical kickboard color
myStairs.treadColor = glm::vec3(0.55f, 0.45f, 0.30f); // Horizontal step surface color
```

#### Step 3: Configure Railings (Optional)
```cpp
myStairs.hasRailings = true;
myStairs.railHeight = 1.0f;                           // Height of railing above each tread
myStairs.railColor = glm::vec3(0.85f, 0.82f, 0.78f);  // Railing panel color
```

#### Step 4: Generate
```cpp
StairMesh generatedStairs = generateStairGeometry(myStairs);
```
*Note: The local `(0,0,0)` origin anchor point of a generated StairMesh is always the **bottom-center of the very first step**. By default, it builds upwards (+Y) and extends outward into the screen (-Z).*

***

<!-- placement function -->
### The Universal Placement System (`placeObject`)

We have unified everything under a single overloaded `placeObject` function that relies on the `Transform` structure. 

#### 1. Define Where It Goes
Instead of manually calculating complex `glm::translate` and `glm::rotate` commands via matrices, you just use the `Transform` struct.
```cpp
Transform myProps;
myProps.position = glm::vec3(10.0f, 0.0f, -5.0f); // X, Y, Z coordinates
myProps.rotation = glm::vec3(0.0f, 90.0f, 0.0f);  // Pitch(X), Yaw(Y), Roll(Z) in Degrees
myProps.scale = glm::vec3(2.0f, 2.0f, 2.0f);      // Double the size
```

#### 2. Define What To Draw
The `placeObject` function will automatically adapt based on what type of mesh you pass into it!

- **Advanced Rooms (`RoomMesh`)**: Requires the UV-based `roomShader`. It automatically handles drawing the 6 faces distinctively and calculating window bounds.
- **Generic Shapes (`BasicMesh`)**: Perfect for standard primitive floor tiles or items you import in the future that only require one solid color. Requires the primitive `ourShader`.
- **Staircases (`StairMesh`)**: Handles bounding-box stair generation with calculated risers, treads, and optional railings. Requires the primitive `ourShader`.

#### Example: How to Place Geometry
```cpp
// Placing a normal generic cube at 0, 0, 0
BasicMesh basicCube = { cubeVAO, 36, glm::vec3(0.0f, 1.0f, 0.0f) }; // Green Cube
Transform cubePos; 
placeObject(ourShader, basicCube, cubePos);

// Placing your massive generated room using the exact same syntax!
Transform roomPos;
roomPos.position = glm::vec3(-50.0f, 0.0f, 0.0f);
placeObject(roomShader, generatedRoom, roomPos);

// Placing a generated stair
StairParams stairDef;
stairDef.width = 3.0f; stairDef.height = 4.0f; stairDef.depth = 6.0f; stairDef.numSteps = 10;
StairMesh strMesh = generateStairGeometry(stairDef);
Transform stairPos;
stairPos.position = glm::vec3(0.0f, 0.0f, 0.0f);
placeObject(ourShader, strMesh, stairPos);
```

## 5. Procedural Parametric Geometry
In `ProceduralMesh.h`, the engine now supports mathematically generated 3D primitives that sit at `(0,0,0)` and use the `ProceduralMesh` struct. You place them just like everything else. Pass to `placeObject` alongside `ourShader`. Keep in mind that since generic primitives only carry 3 geometric Position floats instead of our robust 11-float architecture, you must manually cap the shader state first! Note that the system automatically handles setting this for advanced meshes.
```cpp
ourShader.setBool("useVertexColor", false);
placeObject(ourShader, myCustomMesh, myTransform);
```

***

### 🧱 The 11-Float Vertex Architecture
As of our latest engine updates, every parametric shape (`ProceduralMesh`), room (`RoomMesh`), and staircase (`StairMesh`) is universally built upon a standardized 11-float stride structure mathematically calculated during loop generation:
- `layout (location = 0)` **Position** `(X, Y, Z)`
- `layout (location = 1)` **Normal** `(NX, NY, NZ)`
- `layout (location = 2)` **TexCoords** `(U, V)`
- `layout (location = 3)` **Color** `(R, G, B)`

If a geometry type supports this (like `ProceduralMesh`), the backend automatically pipes `shader.setBool("useVertexColor", true)` and allows the engine to blend vertex shadows/textures directly into `FragColor`!

### Basic Primitives
1. **Hemisphere (Dome):**
   ```cpp
   ProceduralMesh dome = generateHemisphere(radius, sectorCount, stackCount, glm::vec3(r,g,b));
   ```
2. **Surface of Revolution (Vase/Pot):**
   Takes a 2D profile `std::vector<glm::vec2>` and rotates it 360° around the Y-axis.
   ```cpp
   ProceduralMesh vase = generateSurfaceOfRevolution(profilePoints, sectorCount, color);
   ```
3. **Superellipsoid (Sofa Cushion / Pillows):**
   Controls the "squareness" of an ellipsoid. `n` controls latitudinal squareness, `e` controls longitudinal. Sofa cushions are usually `n=0.3, e=1.0`. For cubes use `n=0.1, e=0.1`.
   ```cpp
   ProceduralMesh cushion = generateSuperellipsoid(sizeX, sizeY, sizeZ, n, e, sectors, stacks, color);
   ```
4. **Torus (Ring/Donut):**
   ```cpp
   ProceduralMesh ring = generateTorus(mainRadius, tubeRadius, mainSegments, tubeSegments, color);
   ```
5. **Spiral Staircase (Solid Wedges):**
   Builds a continuous helical staircase around a hexagonal core pillar.
   ```cpp
   ProceduralMesh spiral = generateSpiralStair(innerRadius, stairWidth, stepHeight, numSteps, anglePerStep, color);
   ```

### Composite Scene Objects (`Scene.h`)
These use multiple `ProceduralMesh` parts internally but provide a single, easy-to-use struct for large sets of furniture.

1. **Parametric Sofa** (Made of superellipsoids)
   ```cpp
   SofaParams sofaDef;
   sofaDef.seats = 3;  // dynamic sizing
   sofaDef.totalWidth = 3.0f;
   sofaDef.cushionColor = glm::vec3(0.3f, 0.2f, 0.5f);
   SofaMesh mySofa = generateSofaGeometry(sofaDef);
   
   placeObject(ourShader, mySofa, t); // Places all parts together
   ```

2. **Card Table Gazebo** (Table, chairs, playing cards, pillars, dome)
   ```cpp
   GazeboParams gazeboDef;
   gazeboDef.domeColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red dome
   GazeboMesh myGazebo = generateGazeboGeometry(gazeboDef);
   
   // It is scalable!
   Transform t; t.scale = glm::vec3(2.0f); 
   placeObject(ourShader, myGazebo, t); 
   ```

3. **Potted Fractal Trees** (L-System recursive geometry)
   ```cpp
   PottedTreeParams treeDef;
   treeDef.treeParams.maxDepth = 4;
   treeDef.treeParams.trunkColor = glm::vec3(0.4, 0.3, 0.2);
   PottedTreeMesh myTree = generatePottedTreeGeometry(treeDef);
   placeObject(ourShader, myTree, t);
   ```

4. **Dynamic Dolna (Swing Set)** (Cubic Bezier Tubes + Local Pivot Physics)
   ```cpp
   SwingParams swingDef;
   SwingMesh mySwing = generateSwingGeometry(swingDef);
   
   // Unlike static meshes, the swing takes an active 'swingAngle' to correctly apply pendulum physics matrix transforms to the attached seat and ropes!
   placeObject(ourShader, mySwing, t, swingAngle);
   ```

5. **Lamppost** (Pole + Bulb composite)
   ```cpp
   LamppostParams lampDef;
   lampDef.height = 4.5f;
   lampDef.poleRadius = 0.07f;
   lampDef.bulbRadius = 0.12f;
   lampDef.poleColor = glm::vec3(0.15f, 0.15f, 0.18f);

   LamppostMesh lampMesh = generateLamppostGeometry(lampDef);

   Transform lampT;
   lampT.position = glm::vec3(0.0f, 12.003f, 0.0f); // place base on roof level
   placeObject(ourShader, lampMesh, lampT);
   ```