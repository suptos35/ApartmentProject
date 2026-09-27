#include "ProceduralMesh.h"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// =====================================
// PRIVATE HELPER: Upload to GPU
// =====================================
// Packs vertices + indices into a VAO/VBO/EBO.
// Attrib layout:
//   location 0: Position  (vec3, offset 0)
//   location 1: Normal    (vec3, offset 12)
//   location 2: TexCoords (vec2, offset 24)
static ProceduralMesh uploadMesh(const std::vector<Vertex>& vertices,
                                 const std::vector<unsigned int>& indices,
                                 glm::vec3 color)
{
    unsigned int vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices.size() * sizeof(Vertex),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    return { vao, (unsigned int)indices.size(), color };
}

// =====================================
// SIGNED POWER (for superellipsoid)
// =====================================
// sgn(x) * |x|^p — avoids NaN for x=0
static inline float sgnPow(float x, float p) {
    if (x == 0.0f) return 0.0f;
    return (x > 0.0f ? 1.0f : -1.0f) * std::pow(std::abs(x), p);
}

// =============================================================
// 1. HEMISPHERE
// =============================================================
// === [DEBUG_ID: HEMISPHERE] ===
// hemisphere function
// Crown at (0, +radius, 0). Flat opening faces downward (Y=0 plane).
// UVs: u = theta / 2pi, v = phi / (pi/2)
ProceduralMesh generateHemisphere(float radius, int sectorCount, int stackCount, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // phi: 0 (top) -> pi/2 (equator)
    for (int i = 0; i <= stackCount; ++i) {
        float phi = (float)M_PI * 0.5f * (float)i / (float)stackCount; // 0 to pi/2
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int j = 0; j <= sectorCount; ++j) {
            float theta = 2.0f * (float)M_PI * (float)j / (float)sectorCount;
            float sinT = std::sin(theta);
            float cosT = std::cos(theta);

            glm::vec3 pos(radius * sinPhi * cosT,
                          radius * cosPhi,
                          radius * sinPhi * sinT);
            glm::vec3 normal = glm::normalize(pos);
            glm::vec2 uv((float)j / sectorCount,
                         (float)i / stackCount);

            vertices.push_back({ pos, normal, uv, color });
        }
    }

    // Indices
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j < sectorCount; ++j) {
            unsigned int k1 = i * (sectorCount + 1) + j;
            unsigned int k2 = k1 + sectorCount + 1;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    return uploadMesh(vertices, indices, color);
}

// =============================================================
// 2. SURFACE OF REVOLUTION
// =============================================================
// === [DEBUG_ID: SURFACE_OF_REVOLUTION] ===
// surface of revolution function
// Rotates a 2D profile curve (XZ plane cross-section) around the Y-axis.
// profileCurve: vector of (x, y) points, x >= 0, ordered bottom-to-top.
// Normals computed from the 2D profile tangent, rotated into 3D.
ProceduralMesh generateSurfaceOfRevolution(const std::vector<glm::vec2>& profileCurve,
                                            int sectorCount,
                                            glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    int numPts = (int)profileCurve.size();

    for (int i = 0; i < numPts; ++i) {
        float x2d = profileCurve[i].x;
        float y2d = profileCurve[i].y;

        // Compute 2D tangent along the profile for outward normal
        glm::vec2 tangent2d;
        if (i == 0)
            tangent2d = profileCurve[1] - profileCurve[0];
        else if (i == numPts - 1)
            tangent2d = profileCurve[numPts - 1] - profileCurve[numPts - 2];
        else
            tangent2d = profileCurve[i + 1] - profileCurve[i - 1];

        // Outward 2D normal: perpendicular to tangent pointing outward
        glm::vec2 normal2d = glm::normalize(glm::vec2(tangent2d.y, -tangent2d.x));

        for (int j = 0; j <= sectorCount; ++j) {
            float theta = 2.0f * (float)M_PI * (float)j / (float)sectorCount;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);

            glm::vec3 pos(x2d * cosT, y2d, x2d * sinT);

            // Rotate the 2D normal around Y: (nx2d, 0) -> (nx2d*cosT, 0, nx2d*sinT)
            glm::vec3 normal(normal2d.x * cosT, normal2d.y, normal2d.x * sinT);
            normal = glm::normalize(normal);

            float u = (float)j / sectorCount;
            float v = (float)i / (numPts - 1);

            vertices.push_back({ pos, normal, {u, v}, color });
        }
    }

    // Quad indices
    for (int i = 0; i < numPts - 1; ++i) {
        for (int j = 0; j < sectorCount; ++j) {
            unsigned int k1 = i * (sectorCount + 1) + j;
            unsigned int k2 = k1 + sectorCount + 1;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    return uploadMesh(vertices, indices, color);
}

// =============================================================
// 3. SPIRAL STAIRCASE
// =============================================================
// === [DEBUG_ID: SPIRAL_STAIRCASE] ===
// spiral staircase function
// Each step is a SOLID WEDGE with 5 faces: top tread, front riser,
// inner arc face, outer arc face, and bottom.  Consecutive step
// bottoms/tops share the same Y level so the structure is fully
// connected.  A central hexagonal column is added for rigidity.
// Local anchor: column base centre at (0, 0, 0).
ProceduralMesh generateSpiralStair(float innerRadius,
                                    float stairWidth,
                                    float stepHeight,
                                    int   numSteps,
                                    float anglePerStep,
                                    glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float outerRadius = innerRadius + stairWidth;
    float angRad = (float)(anglePerStep * M_PI / 180.0);

    // Utility: add a quad as two CCW triangles, with a given flat face normal.
    auto addQuad = [&](glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
                       glm::vec3 normal) {
        unsigned int base = (unsigned int)vertices.size();
        vertices.push_back({ p0, normal, {0.0f, 0.0f}, color });
        vertices.push_back({ p1, normal, {1.0f, 0.0f}, color });
        vertices.push_back({ p2, normal, {1.0f, 1.0f}, color });
        vertices.push_back({ p3, normal, {0.0f, 1.0f}, color });
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 0); indices.push_back(base + 2); indices.push_back(base + 3);
    };

    const glm::vec3 up  ( 0.0f,  1.0f,  0.0f);
    const glm::vec3 down( 0.0f, -1.0f,  0.0f);

    for (int i = 0; i < numSteps; ++i) {
        float y0   = i * stepHeight;          // bottom of this step
        float y1   = y0 + stepHeight;         // top   of this step
        float ang0 = i * angRad;
        float ang1 = ang0 + angRad;

        float c0 = std::cos(ang0), s0 = std::sin(ang0);
        float c1 = std::cos(ang1), s1 = std::sin(ang1);

        // 8 corners of the wedge
        glm::vec3 ib0(innerRadius * c0, y0, innerRadius * s0);  // inner bottom ang0
        glm::vec3 ob0(outerRadius * c0, y0, outerRadius * s0);  // outer bottom ang0
        glm::vec3 ib1(innerRadius * c1, y0, innerRadius * s1);  // inner bottom ang1
        glm::vec3 ob1(outerRadius * c1, y0, outerRadius * s1);  // outer bottom ang1
        glm::vec3 it0(innerRadius * c0, y1, innerRadius * s0);  // inner top    ang0
        glm::vec3 ot0(outerRadius * c0, y1, outerRadius * s0);  // outer top    ang0
        glm::vec3 it1(innerRadius * c1, y1, innerRadius * s1);  // inner top    ang1
        glm::vec3 ot1(outerRadius * c1, y1, outerRadius * s1);  // outer top    ang1

        // --- 1. TOP FACE (tread) --- normal = +Y
        addQuad(it0, ot0, ot1, it1, up);

        // --- 2. BOTTOM FACE --- normal = -Y
        addQuad(ib0, ib1, ob1, ob0, down);

        // --- 3. FRONT RISER (at ang0) --- vertical, faces "backward" (-ang0 dir)
        {
            glm::vec3 n = glm::normalize(glm::vec3(-s0, 0.0f, c0)); // tangent normal at ang0
            addQuad(ib0, ob0, ot0, it0, n);
        }

        // --- 4. INNER ARC FACE (at innerRadius, from ang0 to ang1) ---
        // Straight-line approximation; normal points inward (toward centre)
        {
            glm::vec3 midN = glm::normalize(glm::vec3(-(c0 + c1), 0.0f, -(s0 + s1)));
            addQuad(ib1, ib0, it0, it1, midN);
        }

        // --- 5. OUTER ARC FACE (at outerRadius, from ang0 to ang1) ---
        // Normal points outward
        {
            glm::vec3 midN = glm::normalize(glm::vec3(c0 + c1, 0.0f, s0 + s1));
            addQuad(ob0, ob1, ot1, ot0, midN);
        }
    }

    // --- CENTRE COLUMN (hexagonal prism, innerRadius) ---
    // Gives the helical treads something solid to "rest against".
    float totalH = numSteps * stepHeight;
    int colSides = 8;
    for (int i = 0; i < colSides; ++i) {
        float a0 = 2.0f * (float)M_PI * (float)i       / colSides;
        float a1 = 2.0f * (float)M_PI * (float)(i + 1) / colSides;
        float c0c = std::cos(a0), s0c = std::sin(a0);
        float c1c = std::cos(a1), s1c = std::sin(a1);
        float r = innerRadius * 0.9f; // slightly inside tread inner edge
        glm::vec3 bt0(r * c0c, 0.0f,   r * s0c);
        glm::vec3 bt1(r * c1c, 0.0f,   r * s1c);
        glm::vec3 tp0(r * c0c, totalH, r * s0c);
        glm::vec3 tp1(r * c1c, totalH, r * s1c);
        glm::vec3 n = glm::normalize(glm::vec3(c0c + c1c, 0.0f, s0c + s1c));
        addQuad(bt0, bt1, tp1, tp0, n);
    }

    return uploadMesh(vertices, indices, color);
}

// =============================================================
// 4. SUPERELLIPSOID
// =============================================================
// === [DEBUG_ID: SUPERELLIPSOID] ===
// superellipsoid function
// Centered at (0, 0, 0). Pillowy / rounded-box shape.
// n controls east-west squareness (v-loop), e controls north-south (u-loop).
// n=e=1.0 -> ellipsoid. n=e=0.2 -> very boxy cube.
// Typical sofa cushion: n=0.3, e=1.0
ProceduralMesh generateSuperellipsoid(float sizeX, float sizeY, float sizeZ,
                                       float n, float e,
                                       int sectorCount, int stackCount,
                                       glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // v: -pi/2 -> pi/2 (latitude, Y axis)
    // u:  -pi  ->  pi  (longitude, XZ plane)
    for (int i = 0; i <= stackCount; ++i) {
        float v = -(float)M_PI * 0.5f + (float)M_PI * (float)i / (float)stackCount;
        float cosV = std::cos(v);
        float sinV = std::sin(v);

        for (int j = 0; j <= sectorCount; ++j) {
            float u = -(float)M_PI + 2.0f * (float)M_PI * (float)j / (float)sectorCount;
            float cosU = std::cos(u);
            float sinU = std::sin(u);

            // Superellipsoid positions
            float x = sizeX * sgnPow(cosV, n) * sgnPow(cosU, e);
            float y = sizeY * sgnPow(sinV, n);
            float z = sizeZ * sgnPow(cosV, n) * sgnPow(sinU, e);

            glm::vec3 pos(x, y, z);

            // Approximate normal: use (x/sizeX^2, y/sizeY^2, z/sizeZ^2) for an ellipsoid,
            // scaled by exponent for the superellipsoid curvature.
            // A robust approach: normalize the gradient of the implicit function.
            float nx = sgnPow(cosV, 2.0f - n) * sgnPow(cosU, 2.0f - e) / sizeX;
            float ny = sgnPow(sinV, 2.0f - n) / sizeY;
            float nz = sgnPow(cosV, 2.0f - n) * sgnPow(sinU, 2.0f - e) / sizeZ;
            glm::vec3 normal = glm::normalize(glm::vec3(nx, ny, nz));

            glm::vec2 uv((float)j / sectorCount, (float)i / stackCount);

            vertices.push_back({ pos, normal, uv, color });
        }
    }

    // Indices (same grid pattern as hemisphere)
    for (int i = 0; i < stackCount; ++i) {
        for (int j = 0; j < sectorCount; ++j) {
            unsigned int k1 = i * (sectorCount + 1) + j;
            unsigned int k2 = k1 + sectorCount + 1;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    return uploadMesh(vertices, indices, color);
}

// =============================================================
// 5. TORUS
// =============================================================
// === [DEBUG_ID: TORUS] ===
// torus function
// Ring lies flat in the XZ plane, centered at (0, 0, 0).
// mainRadius (R): distance from torus center to tube center.
// tubeRadius (r): radius of the tube cross-section.
// x = (R + r*cosV) * cosU
// y =  r * sinV
// z = (R + r*cosV) * sinU
ProceduralMesh generateTorus(float mainRadius, float tubeRadius,
                              int   mainSegments, int tubeSegments,
                              glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= mainSegments; ++i) {
        float u = 2.0f * (float)M_PI * (float)i / (float)mainSegments;
        float cosU = std::cos(u);
        float sinU = std::sin(u);

        for (int j = 0; j <= tubeSegments; ++j) {
            float v = 2.0f * (float)M_PI * (float)j / (float)tubeSegments;
            float cosV = std::cos(v);
            float sinV = std::sin(v);

            glm::vec3 pos(
                (mainRadius + tubeRadius * cosV) * cosU,
                tubeRadius * sinV,
                (mainRadius + tubeRadius * cosV) * sinU
            );

            // Normal: from tube center to vertex surface
            glm::vec3 normal(cosV * cosU, sinV, cosV * sinU);
            normal = glm::normalize(normal);

            glm::vec2 uv((float)i / mainSegments, (float)j / tubeSegments);

            vertices.push_back({ pos, normal, uv, color });
        }
    }

    // Indices
    for (int i = 0; i < mainSegments; ++i) {
        for (int j = 0; j < tubeSegments; ++j) {
            unsigned int k1 = i * (tubeSegments + 1) + j;
            unsigned int k2 = k1 + tubeSegments + 1;

            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);

            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    return uploadMesh(vertices, indices, color);
}

// =============================================================
// 6. POT GENERATOR
// =============================================================
ProceduralMesh generatePot(float baseRadius,
                           float topRadius,
                           float height,
                           float curveBulge,
                           int   radialSegments,
                           glm::vec3 color)
{
    std::vector<glm::vec2> profile;
    // Walk from bottom center (0,0) to lip and slightly down inside
    profile.push_back(glm::vec2(0.0f, 0.0f));
    profile.push_back(glm::vec2(baseRadius, 0.0f)); // flat bottom

    // curved outer wall
    int curvePoints = 8;
    for (int i = 1; i <= curvePoints; ++i) {
        float t = (float)i / curvePoints;
        float y = t * height;
        float currentR = glm::mix(baseRadius, topRadius, t);
        // Add parabolic outward bulge
        currentR += curveBulge * 4.0f * t * (1.0f - t);
        profile.push_back(glm::vec2(currentR, y));
    }

    // inner rim / thickness
    float thickness = 0.05f;
    float innerTopRadius = topRadius - thickness;
    profile.push_back(glm::vec2(innerTopRadius, height));
    
    // slope down inside
    profile.push_back(glm::vec2(innerTopRadius - 0.02f, height - 0.1f));
    profile.push_back(glm::vec2(innerTopRadius - 0.08f, height - 0.5f)); // pot dirt level

    // Let generateSurfaceOfRevolution do the heavy lifting
    return generateSurfaceOfRevolution(profile, radialSegments, color);
}

// =============================================================
// 7. FRACTAL TREE GENERATOR
// =============================================================
TreeMeshes generateFractalTree(const TreeParams& params)
{
    std::vector<Vertex> trunkVerts;
    std::vector<unsigned int> trunkInds;

    std::vector<Vertex> leavesVerts;
    std::vector<unsigned int> leavesInds;

    // Helper: generate cylinder for a branch
    auto addCylinder = [&](glm::mat4 transform, float radius, float length) {
        int segments = 6; // low poly for trees
        unsigned int baseIdx = (unsigned int)trunkVerts.size();
        
        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * M_PI * i / segments;
            float cx = std::cos(theta);
            float cz = std::sin(theta);
            glm::vec3 localNormal(cx, 0.0f, cz);
            
            glm::vec3 localBase(radius * cx, 0.0f, radius * cz);
            glm::vec3 localTop(radius * cx, length, radius * cz);
            
            glm::vec3 pBase = glm::vec3(transform * glm::vec4(localBase, 1.0f));
            glm::vec3 pTop  = glm::vec3(transform * glm::vec4(localTop,  1.0f));
            
            glm::vec3 nBase = glm::normalize(glm::vec3(transform * glm::vec4(localNormal, 0.0f)));
            glm::vec3 nTop  = nBase;

            float u = (float)i / segments;
            trunkVerts.push_back({ pBase, nBase, {u, 0.0f} });
            trunkVerts.push_back({ pTop,  nTop,  {u, 1.0f} });
        }
        
        for (int i = 0; i < segments; ++i) {
            unsigned int i0 = baseIdx + i*2;
            unsigned int i1 = i0 + 1;
            unsigned int i2 = baseIdx + (i+1)*2;
            unsigned int i3 = i2 + 1;

            trunkInds.push_back(i0); trunkInds.push_back(i1); trunkInds.push_back(i2);
            trunkInds.push_back(i2); trunkInds.push_back(i1); trunkInds.push_back(i3);
        }
    };

    // Helper: generate foliage cluster at branch tip
    auto addFoliage = [&](glm::mat4 transform) {
        if (params.foliageType == 0) return;
        
        unsigned int b = (unsigned int)leavesVerts.size();
        glm::vec3 center = glm::vec3(transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

        if (params.foliageType == 2) { // QUADS (3 intersecting planes)
            float size = params.trunkHeight * 1.5f; 
            glm::vec3 up = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
            glm::vec3 right = glm::normalize(glm::vec3(transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
            glm::vec3 fwd = glm::normalize(glm::vec3(transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
            
            auto addPlane = [&](glm::vec3 u, glm::vec3 v, glm::vec3 norm) {
                leavesVerts.push_back({ center - u*size - v*size,  norm, {0,0}, params.foliageColor });
                leavesVerts.push_back({ center + u*size - v*size,  norm, {1,0}, params.foliageColor });
                leavesVerts.push_back({ center + u*size + v*size,  norm, {1,1}, params.foliageColor });
                leavesVerts.push_back({ center - u*size + v*size,  norm, {0,1}, params.foliageColor });
                // front face
                leavesInds.push_back(b+0); leavesInds.push_back(b+1); leavesInds.push_back(b+2);
                leavesInds.push_back(b+0); leavesInds.push_back(b+2); leavesInds.push_back(b+3);
                // back face 
                leavesInds.push_back(b+0); leavesInds.push_back(b+2); leavesInds.push_back(b+1);
                leavesInds.push_back(b+0); leavesInds.push_back(b+3); leavesInds.push_back(b+2);
                b += 4;
            };
            addPlane(right, up, fwd);
            addPlane(fwd, up, right);
            addPlane(right, fwd, up);

        } else if (params.foliageType == 1) { // SPHERES (approximated as octahedrons)
            float size = params.trunkHeight * 1.0f;
            glm::vec3 top = center + glm::vec3(0,size,0);
            glm::vec3 bot = center + glm::vec3(0,-size,0);
            glm::vec3 xp  = center + glm::vec3(size,0,0);
            glm::vec3 xn  = center + glm::vec3(-size,0,0);
            glm::vec3 zp  = center + glm::vec3(0,0,size);
            glm::vec3 zn  = center + glm::vec3(0,0,-size);

            auto addTri = [&](glm::vec3 v1, glm::vec3 v2, glm::vec3 v3) {
                glm::vec3 n = glm::normalize(glm::cross(v2 - v1, v3 - v1));
                unsigned int idx = (unsigned int)leavesVerts.size();
                leavesVerts.push_back({v1, n, {0.5f, 1.0f}, params.foliageColor});
                leavesVerts.push_back({v2, n, {1.0f, 0.0f}, params.foliageColor});
                leavesVerts.push_back({v3, n, {0.0f, 0.0f}, params.foliageColor});
                leavesInds.push_back(idx); leavesInds.push_back(idx+1); leavesInds.push_back(idx+2);
            };
            addTri(top, xp, zp); addTri(top, zp, xn); addTri(top, xn, zn); addTri(top, zn, xp);
            addTri(bot, zp, xp); addTri(bot, xn, zp); addTri(bot, zn, xn); addTri(bot, xp, zn);
        }
    };

    struct BuildNode {
        int depth;
        glm::mat4 transform;
        float radius;
        float length;
    };
    std::vector<BuildNode> stack;
    stack.push_back({0, glm::mat4(1.0f), params.trunkRadius, params.trunkHeight});

    while (!stack.empty()) {
        BuildNode node = stack.back();
        stack.pop_back();

        // 1. Draw trunk cylinder
        addCylinder(node.transform, node.radius, node.length);

        if (node.depth < params.maxDepth) {
            // 2. Fork into children
            glm::mat4 endTransform = glm::translate(node.transform, glm::vec3(0.0f, node.length, 0.0f));
            float newRadius = node.radius * params.radiusScale;
            float newLength = node.length * params.lengthScale;
            
            float rotStep = 360.0f / params.branchesPerNode;
            float angleOffset = (node.depth % 2) * 45.0f; // twist the layers

            for (int i = 0; i < params.branchesPerNode; ++i) {
                glm::mat4 childMat = endTransform;
                childMat = glm::rotate(childMat, glm::radians(angleOffset + i * rotStep), glm::vec3(0.0f, 1.0f, 0.0f));
                childMat = glm::rotate(childMat, glm::radians(params.spreadAngle), glm::vec3(1.0f, 0.0f, 0.0f));
                stack.push_back({node.depth + 1, childMat, newRadius, newLength});
            }
        } else {
            // 3. Max depth reached, add foliage
            glm::mat4 endTransform = glm::translate(node.transform, glm::vec3(0.0f, node.length, 0.0f));
            addFoliage(endTransform);
        }
    }

    TreeMeshes result;
    result.trunk = uploadMesh(trunkVerts, trunkInds, params.trunkColor);
    result.foliage = uploadMesh(leavesVerts, leavesInds, params.foliageColor);
    return result;
}

// =============================================================
// 8. BEZIER TUBE GENERATOR
// =============================================================
ProceduralMesh generateBezierTube(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, 
                                  float radius, int segments, int radialSegments, glm::vec3 color)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    auto getBezierPoint = [&](float t) -> glm::vec3 {
        float u = 1.0f - t;
        float tt = t * t;
        float uu = u * u;
        float uuu = uu * u;
        float ttt = tt * t;
        
        glm::vec3 p = uuu * p0;
        p += 3.0f * uu * t * p1;
        p += 3.0f * u * tt * p2;
        p += ttt * p3;
        return p;
    };

    auto getBezierTangent = [&](float t) -> glm::vec3 {
        float u = 1.0f - t;
        glm::vec3 tan = 3.0f * u * u * (p1 - p0) +
                        6.0f * u * t * (p2 - p1) +
                        3.0f * t * t * (p3 - p2);
        return glm::normalize(tan);
    };

    // Calculate Parallel Transport frames along the curve
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> binormals;

    glm::vec3 lastNormal;
    
    for (int i = 0; i <= segments; ++i) {
        float t = (float)i / segments;
        glm::vec3 pt = getBezierPoint(t);
        glm::vec3 T = getBezierTangent(t);
        
        glm::vec3 N;
        if (i == 0) {
            glm::vec3 up = glm::vec3(0,1,0);     
            if (glm::abs(glm::dot(T, up)) > 0.99f) {
                up = glm::vec3(1,0,0);
            }
            glm::vec3 B = glm::normalize(glm::cross(T, up));
            N = glm::normalize(glm::cross(B, T));
        } else {
            glm::vec3 axis = glm::cross(tangents[i-1], T);
            float d = glm::clamp(glm::dot(tangents[i-1], T), -1.0f, 1.0f);
            float angle = std::acos(d);
            if (glm::length(axis) > 0.001f) {
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), angle, glm::normalize(axis));
                N = glm::vec3(rot * glm::vec4(lastNormal, 0.0f));
            } else {
                N = lastNormal;
            }
        }
        N = glm::normalize(N);
        glm::vec3 B = glm::normalize(glm::cross(T, N));
        
        points.push_back(pt);
        tangents.push_back(T);
        normals.push_back(N);
        binormals.push_back(B);
        lastNormal = N;
        
        // generate radial vertices
        for (int j = 0; j <= radialSegments; ++j) {
            float v = (float)j / radialSegments;
            float theta = v * 2.0f * M_PI;
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);
            
            glm::vec3 radialDir = N * cosT + B * sinT;
            glm::vec3 pos = pt + radialDir * radius;
            
            float u = (float)i / segments;
            vertices.push_back({ pos, radialDir, {u, v}, color });
        }
    }
    
    // Indices
    for (int i = 0; i < segments; ++i) {
        for (int j = 0; j < radialSegments; ++j) {
            unsigned int k1 = i * (radialSegments + 1) + j;
            unsigned int k2 = k1 + radialSegments + 1;
            
            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);
            
            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
    }

    return uploadMesh(vertices, indices, color);
}

ProceduralMesh generateSphere(float radius, int sectorCount, int stackCount) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float x, y, z, xy;                              
    float nx, ny, nz, lengthInv = 1.0f / radius;    
    float s, t;                                     
    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        y = radius * sinf(stackAngle);
        for(int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            z = xy * sinf(sectorAngle);
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            Vertex vertex;
            vertex.Position = glm::vec3(x, y, z);
            vertex.Normal = glm::vec3(nx, ny, nz);
            vertex.TexCoords = glm::vec2(s, t);
            vertex.Color = glm::vec3(0.8f);
            vertices.push_back(vertex);
        }
    }
    int k1, k2;
    for(int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     
        k2 = k1 + sectorCount + 1;      
        for(int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if(i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if(i != (stackCount-1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
    return uploadMesh(vertices, indices, glm::vec3(0.8f));
}

