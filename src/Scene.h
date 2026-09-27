#ifndef SCENE_H
#define SCENE_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include "Shader.h"
#include "ProceduralMesh.h"

// =====================================
// HOLE & ROOM DATA STRUCTURES
// =====================================

struct Hole {
    float x;
    float y;
    float width;
    float height;

    enum Type { EDGE_CONNECT, DOOR, WINDOW } type = EDGE_CONNECT;
    bool isOpen = true; // doors closed by default (false), windows open (true)
    std::string label = "";
};

struct RoomFace {
    bool isVisible = true;
    glm::vec3 color = glm::vec3(0.8f);
    std::vector<Hole> holes;
};

struct RoomParams {
    float width;
    float height;
    float depth;
    RoomFace faces[6]; // Order: 0:Front, 1:Back, 2:Left, 3:Right, 4:Top, 5:Bottom
};

// Interactive hole entry, stored in world space after placement
struct InteractivePoint {
    glm::vec3 worldPos;
    bool* isOpen;        // pointer into actual Hole data
    std::string label;
};

struct RoomMesh {
    unsigned int vao;
    unsigned int vbo;
    struct FaceDrawInfo {
        int first;
        int count;
        glm::vec3 color;
        std::vector<Hole> holes;
        std::vector<glm::vec3> holeCentersLocal; // 3D local space center for each hole
    } faceDraws[6];
};


// Constants for floor layout
const float FLOOR_0 = 0.0f;
const float FLOOR_1 = 3.0f;
const float ROOF    = 6.0f;

const float ROOM_SIZE = 4.0f;
const float WALL_HEIGHT = 2.5f;
const float WALL_THICKNESS = 0.2f;

// =====================================
// LAMPPOST
// =====================================
struct LamppostParams {
    float height = 4.5f;
    float poleRadius = 0.05f;
    float bulbRadius = 0.3f;
    glm::vec3 poleColor = glm::vec3(0.2f);
    glm::vec3 bulbColor = glm::vec3(1.0f);
};
struct LamppostMesh {
    ProceduralMesh pole;
    ProceduralMesh bulb;
};

// =====================================
// ADVANCED ARCHITECTURAL MESHES
// =====================================
struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 toMatrix() const {
        glm::mat4 m = glm::mat4(1.0f);
        m = glm::translate(m, position);
        m = glm::rotate(m, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        m = glm::rotate(m, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        m = glm::rotate(m, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::scale(m, scale);
        return m;
    }
};

struct BasicMesh {
    unsigned int vao;
    int vertexCount;
    glm::vec3 color;
};

// =====================================
// PLACEMENT FUNCTIONS (OVERLOADED)
// =====================================

// Place a dynamically generated room
void placeObject(Shader& shader, const RoomMesh& mesh, const Transform& transform);

// Place a generic simple solid-colored primitive (e.g. cube, sphere)
void placeObject(Shader& shader, const BasicMesh& mesh, const Transform& transform);

// Register interactive holes into a global list after placement
void registerRoomInteractions(RoomMesh& mesh, const Transform& transform, std::vector<InteractivePoint>& out);

// =====================================
// STAIR GENERATOR
// =====================================

// stair function
struct StairParams {
    float width;     // X extent
    float height;    // Y extent (total rise)
    float depth;     // Z extent (total run into -Z)
    int   numSteps;
    glm::vec3 riserColor  = glm::vec3(0.75f, 0.60f, 0.40f);
    glm::vec3 treadColor  = glm::vec3(0.55f, 0.45f, 0.30f);
    bool  hasRailings     = true;
    float railHeight      = 1.0f;                           // height of railing above tread
    glm::vec3 railColor   = glm::vec3(0.85f, 0.82f, 0.78f); // light concrete/metal
};

// Mesh with separate draw range per component — use placeObject(ourShader, stairMesh, transform)
struct StairMesh {
    unsigned int vao;
    int riserFirst,  riserCount;
    int treadFirst,  treadCount;
    int railFirst,   railCount;   // 0 if no railings
    glm::vec3 riserColor, treadColor, railColor;
};

// Generators
StairMesh generateStairGeometry(const StairParams& params);
RoomMesh  generateRoomGeometry (const RoomParams& params);

// Placement overload for stairs
void placeObject(Shader& shader, const StairMesh& mesh, const Transform& transform);

// =====================================
// SOFA GENERATOR (Composite Object)
// =====================================

struct SofaParams {
    int seats = 2; // e.g. 2-seater, 3-seater
    float totalWidth = 2.4f;
    glm::vec3 cushionColor = glm::vec3(0.4f, 0.3f, 0.6f);
    glm::vec3 frameColor = glm::vec3(0.2f, 0.2f, 0.2f);
};

struct SofaMesh {
    ProceduralMesh baseFrame;
    ProceduralMesh armRest;
    ProceduralMesh seatCushion;
    ProceduralMesh backCushion;
    
    int seats;
    float totalWidth;
};

SofaMesh generateSofaGeometry(const SofaParams& params);
void placeObject(Shader& shader, const SofaMesh& mesh, const Transform& transform);

// =====================================
// GAZEBO SCENE (Composite Object)
// =====================================
// A complete scene unit: round table, 4 chairs placed around it,
// playing items on the table, a dome overhead held up by 4 bar pillars.
// Local anchor: center at (0, 0, 0), bottom at Y=0.

struct GazeboMesh {
    // Table
    ProceduralMesh tableTop;    // surface of revolution disc
    ProceduralMesh tablePedestal; // surface of revolution stem

    // Dome structure
    ProceduralMesh dome;        // hemisphere
    ProceduralMesh pillar;      // 1 mesh, drawn 4 times at 45/135/225/315 deg

    // Chairs (1 mesh each, drawn 4 times)
    ProceduralMesh chairSeat;
    ProceduralMesh chairBack;
    ProceduralMesh chairLeg;    // 1 mesh, drawn 4 times per chair (16 draws total)

    // Playing items
    ProceduralMesh playCard;    // flat superellipsoid, drawn several times on table
    ProceduralMesh chip;        // small disc, drawn several times on table
};

struct GazeboParams {
    glm::vec3 tableColor   = glm::vec3(0.75f, 0.65f, 0.55f); // warm wood
    glm::vec3 chairColor   = glm::vec3(0.40f, 0.30f, 0.22f); // dark wood
    glm::vec3 domeColor    = glm::vec3(0.90f, 0.92f, 0.88f); // off-white glass
    glm::vec3 pillarColor  = glm::vec3(0.55f, 0.55f, 0.60f); // brushed metal
    glm::vec3 cardColor    = glm::vec3(0.98f, 0.98f, 0.96f); // card white
    glm::vec3 chipColor    = glm::vec3(0.20f, 0.50f, 0.85f); // poker blue
};

GazeboMesh generateGazeboGeometry(const GazeboParams& params);
void placeObject(Shader& shader, const GazeboMesh& mesh, const Transform& transform);

// =====================================
// POTTED TREE (Composite Object)
// =====================================
// A combined surface-of-revolution pot and recursive fractal tree.
struct PottedTreeParams {
    TreeParams treeParams;
    float potBaseRadius = 0.3f;
    float potTopRadius = 0.5f;
    float potHeight = 0.6f;
    float potBulge = 0.1f;
    glm::vec3 potColor = glm::vec3(0.6f, 0.4f, 0.3f); // terra cotta
};

struct PottedTreeMesh {
    ProceduralMesh pot;
    TreeMeshes tree;
};

LamppostMesh generateLamppostGeometry(const LamppostParams& params);
PottedTreeMesh generatePottedTreeGeometry(const PottedTreeParams& params);
void placeObject(Shader& shader, const PottedTreeMesh& mesh, const Transform& transform);

// =====================================
// SWING SET (Composite Object)
// =====================================
struct SwingParams {
    float ropeLength = 4.0f;
    float seatWidth = 1.4f;
    float seatDepth = 0.6f;
    float frameWidth = 2.4f;
    float frameHeight = 4.5f;
    glm::vec3 frameColor = glm::vec3(0.4f, 0.35f, 0.3f);
    glm::vec3 seatColor = glm::vec3(0.8f, 0.4f, 0.2f);
    glm::vec3 ropeColor = glm::vec3(0.85f, 0.8f, 0.7f);
};

struct SwingMesh {
    ProceduralMesh framePillar;
    ProceduralMesh frameBar;
    ProceduralMesh rope;
    ProceduralMesh seat;

    float ropeLength;
    float frameWidth;
    float frameHeight;
    float seatWidth;
};

SwingMesh generateSwingGeometry(const SwingParams& params);
// Requires dynamic swingAngle for the physics pendulum update
void placeObject(Shader& shader, const SwingMesh& mesh, const Transform& transform, float swingAngle);

// Place a parametric procedural mesh (Hemisphere, Torus, etc.)
void placeObject(Shader& shader, const ProceduralMesh& mesh, const Transform& transform);

// =====================================
// BED (Composite Object)
// =====================================
struct BedParams {
    float width = 2.0f;
    float length = 2.4f;
    float frameHeight = 0.3f;
    float mattressHeight = 0.2f;
    glm::vec3 frameColor = glm::vec3(0.3f, 0.2f, 0.1f);
    glm::vec3 mattressColor = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 blanketColor = glm::vec3(0.2f, 0.4f, 0.6f);
};

struct BedMesh {
    ProceduralMesh frame;
    ProceduralMesh mattress;
    ProceduralMesh blanket;
    ProceduralMesh pillow; // Drawn twice
};

BedMesh generateBedGeometry(const BedParams& params);
void placeObject(Shader& shader, const LamppostMesh& lamppost, const Transform& transform);
void placeObject(Shader& shader, const BedMesh& bed, const Transform& transform);

// =====================================
// TABLE (Composite Object)
// =====================================
struct TableParams {
    float width = 1.6f;
    float depth = 1.0f;
    float height = 1.0f;
    float topThickness = 0.05f;
    float legRadius = 0.04f;
    glm::vec3 color = glm::vec3(0.6f, 0.4f, 0.2f);
};

struct TableMesh {
    ProceduralMesh top;
    ProceduralMesh leg; // Drawn 4 times
    float width, depth, height;
};

TableMesh generateTableGeometry(const TableParams& params);
void placeObject(Shader& shader, const TableMesh& mesh, const Transform& transform);

// =====================================
// CHAIR (Composite Object)
// =====================================
struct ChairParams {
    float width = 0.5f;
    float depth = 0.5f;
    float seatHeight = 0.5f;
    float backHeight = 0.6f;
    glm::vec3 woodColor = glm::vec3(0.5f, 0.3f, 0.1f);
    glm::vec3 cushionColor = glm::vec3(0.2f, 0.2f, 0.2f);
};

struct ChairMesh {
    ProceduralMesh seat;
    ProceduralMesh backrest;
    ProceduralMesh leg; // Drawn 4 times
    float seatHeight, backHeight, width, depth;
};

ChairMesh generateChairGeometry(const ChairParams& params);
void placeObject(Shader& shader, const ChairMesh& mesh, const Transform& transform);

// =====================================
// DINING SET (Composite Object)
// =====================================
struct DiningSetParams {
    TableParams tableDef;
    ChairParams chairDef;
    int numChairs = 4; // 2 on each long side
};

struct DiningSetMesh {
    TableMesh table;
    ChairMesh chair;
    int numChairs;
};

DiningSetMesh generateDiningSetGeometry(const DiningSetParams& params);
void placeObject(Shader& shader, const DiningSetMesh& mesh, const Transform& transform);

// =====================================
// COMMODE (Composite Object)
// =====================================
struct CommodeParams {
    glm::vec3 porcelainColor = glm::vec3(0.95f, 0.95f, 0.95f);
};

struct CommodeMesh {
    ProceduralMesh bowl;
    ProceduralMesh tank;
    ProceduralMesh lid;
};

CommodeMesh generateCommodeGeometry(const CommodeParams& params);
void placeObject(Shader& shader, const CommodeMesh& mesh, const Transform& transform);

// =====================================
// SHOWER (Composite Object)
// =====================================
struct ShowerParams {
    float width = 1.2f;
    float depth = 1.2f;
    float height = 2.2f;
    glm::vec3 baseColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 glassColor = glm::vec3(0.6f, 0.8f, 0.9f); // Simple semi-transparent blue
};

struct ShowerMesh {
    ProceduralMesh base;
    ProceduralMesh glassWall;
    ProceduralMesh headPipe;
    ProceduralMesh headNozzle;
    float width, depth, height;
};

ShowerMesh generateShowerGeometry(const ShowerParams& params);
void placeObject(Shader& shader, const ShowerMesh& mesh, const Transform& transform);

#endif
