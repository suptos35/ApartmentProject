#ifndef PROCEDURAL_MESH_H
#define PROCEDURAL_MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

// =====================================
// PROCEDURAL MESH SYSTEM
// =====================================
// All generators produce geometry anchored at local (0,0,0).
// y=0 is the bottom of the object (where applicable).
// VAO layout: attrib 0 = Position (vec3), 1 = Normal (vec3), 2 = TexCoords (vec2)
// Draw with: glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0)
// Compatible with placeObject(ourShader, mesh, transform)

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Color;
};

struct ProceduralMesh {
    unsigned int vao;
    unsigned int indexCount;
    glm::vec3 color;
};

// =====================================
// GENERATOR FUNCTION DECLARATIONS
// =====================================

// 1. hemisphere function
// Dome shape. Center at (0,0,0), opens downward, crown at +Y.
// phi sweeps 0 -> pi/2 (top hemisphere only).
ProceduralMesh generateHemisphere(float radius,
                                  int   sectorCount,
                                  int   stackCount,
                                  glm::vec3 color);

// 2. surface of revolution function
// Sweeps a 2D profile curve (XY plane) around the Y-axis by 2pi.
// profileCurve: list of (x, y) points from bottom to top, x >= 0.
ProceduralMesh generateSurfaceOfRevolution(const std::vector<glm::vec2>& profileCurve,
                                           int sectorCount,
                                           glm::vec3 color);

// 3. spiral staircase function
// A helical staircase winding around the Y-axis.
// Local anchor: center of the inner column at (0, 0, 0).
// Steps rise in +Y direction and wind counter-clockwise when viewed from above.
ProceduralMesh generateSpiralStair(float innerRadius,
                                   float stairWidth,
                                   float stepHeight,
                                   int   numSteps,
                                   float anglePerStep,   // degrees
                                   glm::vec3 color);

// 4. superellipsoid function
// Pillowy / rounded-box shape. Centered at (0,0,0).
// n and e control squareness. n=e=1.0 -> sphere, n=e=0.2 -> very box-like.
// Typical sofa cushion: n=0.3, e=1.0
ProceduralMesh generateSuperellipsoid(float sizeX,
                                      float sizeY,
                                      float sizeZ,
                                      float n,
                                      float e,
                                      int   sectorCount,
                                      int   stackCount,
                                      glm::vec3 color);

// 5. torus function
// Ring shape. Centered at (0,0,0), ring lies in the XZ plane.
// mainRadius: center of hole to center of tube. tubeRadius: tube thickness.
ProceduralMesh generateTorus(float mainRadius,
                             float tubeRadius,
                             int   mainSegments,
                             int   tubeSegments,
                             glm::vec3 color);
// 6. pot function
// Generates a smooth pot/planter shape using surface of revolution.
ProceduralMesh generatePot(float baseRadius,
                           float topRadius,
                           float height,
                           float curveBulge,
                           int   radialSegments,
                           glm::vec3 color);

// 7. fractal tree function
struct TreeParams {
    float trunkHeight;
    float trunkRadius;
    int   maxDepth;
    float lengthScale;
    float radiusScale;
//   sphere radius (r).
ProceduralMesh generateSphere(float radius = 1.0f, int sectorCount = 36, int stackCount = 18);

    float spreadAngle;      // degrees
    int   branchesPerNode;  // e.g. 2, 3, or 4
    glm::vec3 trunkColor;
    glm::vec3 foliageColor;
    int   foliageType;      // 0=None, 1=Spheres, 2=Quads
};

struct TreeMeshes {
    ProceduralMesh trunk;
    ProceduralMesh foliage;
};

// Generates a UV mapped spherical geometry array representing a planet or casing
ProceduralMesh generateSphere(float radius = 1.0f, int sectorCount = 36, int stackCount = 18);

// Generates a recursive tree starting at (0,0,0) growing in +Y direction.
TreeMeshes generateFractalTree(const TreeParams& params);

// 8. Bezier Tube function
// Sweeps a 3D circle along a cubic Bezier curve to generate a thick rope or chain.
ProceduralMesh generateBezierTube(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, 
                                  float radius, int segments, int radialSegments, glm::vec3 color);

#endif // PROCEDURAL_MESH_H
