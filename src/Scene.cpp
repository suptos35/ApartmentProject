#include "Scene.h"
#include <cmath>

void placeObject(Shader& shader, const BasicMesh& mesh, const Transform& transform) {
    shader.setMat4("model", transform.toMatrix());
    shader.setBool("useVertexColor", false);
    shader.setVec3("objectColor", mesh.color);
    glBindVertexArray(mesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
    glBindVertexArray(0);
}

void placeObject(Shader& shader, const RoomMesh& mesh, const Transform& transform) {
    shader.setMat4("model", transform.toMatrix());
    shader.setBool("useVertexColor", true);
    glBindVertexArray(mesh.vao);
    
    for (int i = 0; i < 6; ++i) {
        if (mesh.faceDraws[i].count > 0) {
            // we no longer rely on objectColor uniform, but update it just in case
            shader.setVec3("objectColor", mesh.faceDraws[i].color);
            int numHoles = mesh.faceDraws[i].holes.size();
            shader.setInt("numHoles", numHoles < 10 ? numHoles : 10);
            for (int h = 0; h < numHoles && h < 10; ++h) {
                const auto& hole = mesh.faceDraws[i].holes[h];
                shader.setVec4("holes[" + std::to_string(h) + "]", glm::vec4(hole.x, hole.y, hole.width, hole.height));
                shader.setInt("holeType[" + std::to_string(h) + "]", (int)hole.type);
                shader.setInt("holeOpen[" + std::to_string(h) + "]", hole.isOpen ? 1 : 0);
            }
            glDrawArrays(GL_TRIANGLES, mesh.faceDraws[i].first, mesh.faceDraws[i].count);
        }
    }
    glBindVertexArray(0);
}

void registerRoomInteractions(RoomMesh& mesh, const Transform& transform, std::vector<InteractivePoint>& out) {
    glm::mat4 model = transform.toMatrix();
    for (int i = 0; i < 6; ++i) {
        for (int h = 0; h < (int)mesh.faceDraws[i].holes.size(); ++h) {
            auto& hole = mesh.faceDraws[i].holes[h];
            if (hole.type == Hole::EDGE_CONNECT) continue;
            if (h < (int)mesh.faceDraws[i].holeCentersLocal.size()) {
                glm::vec3 localCenter = mesh.faceDraws[i].holeCentersLocal[h];
                glm::vec4 worldCenter = model * glm::vec4(localCenter, 1.0f);
                out.push_back({ glm::vec3(worldCenter), &hole.isOpen, hole.label });
            }
        }
    }
}

// ==========================================
// [DEBUG_ID: DYNAMIC_ROOM_GENERATOR]
// Local Anchor: centered at 0,0,0.
// Valid faces pushed out to form a box.
RoomMesh generateRoomGeometry(const RoomParams& params) {
    // new room geometry function
    RoomMesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    
    std::vector<Vertex> vertices;
    
    float w2 = params.width / 2.0f;
    float h2 = params.height / 2.0f;
    float d2 = params.depth / 2.0f;
    
    // Compute hole center in 3D local space based on face and UV coordinates.
    // Each face maps UV(u, v) onto 3D position differently.
    auto computeHoleCenter = [&](int faceIndex, const Hole& hole) -> glm::vec3 {
        float cx = hole.x + hole.width / 2.0f;
        float cy = hole.y + hole.height / 2.0f;
        switch (faceIndex) {
            case 0: return glm::vec3(cx - w2, cy - h2,  d2); // Front
            case 1: return glm::vec3(w2 - cx, cy - h2, -d2); // Back (U reversed)
            case 2: return glm::vec3(-w2, cy - h2, cx - d2); // Left (U -> Z)
            case 3: return glm::vec3( w2, cy - h2, d2 - cx); // Right (U reversed Z)
            case 4: return glm::vec3(cx - w2,  h2, d2 - cy); // Top
            case 5: return glm::vec3(cx - w2, -h2, cy - d2); // Bottom
            default: return glm::vec3(0.0f);
        }
    };

    auto addFace = [&](int faceIndex, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4, float w, float h) {
        if (!params.faces[faceIndex].isVisible) {
            mesh.faceDraws[faceIndex].count = 0;
            return;
        }
        int startVertexCount = vertices.size();
        
        glm::vec3 n = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        glm::vec3 c = params.faces[faceIndex].color;
        
        vertices.push_back({p1, n, {0.0f, 0.0f}, c});
        vertices.push_back({p2, n, {w, 0.0f},    c});
        vertices.push_back({p3, n, {w, h},       c});
        vertices.push_back({p1, n, {0.0f, 0.0f}, c});
        vertices.push_back({p3, n, {w, h},       c});
        vertices.push_back({p4, n, {0.0f, h},    c});
        
        mesh.faceDraws[faceIndex].first = startVertexCount;
        mesh.faceDraws[faceIndex].count = 6;
        mesh.faceDraws[faceIndex].color = c;
        mesh.faceDraws[faceIndex].holes = params.faces[faceIndex].holes;

        // Compute 3D local center for each hole
        for (const auto& hole : params.faces[faceIndex].holes) {
            mesh.faceDraws[faceIndex].holeCentersLocal.push_back(computeHoleCenter(faceIndex, hole));
        }
    };
    
    addFace(0, {-w2, -h2,  d2}, { w2, -h2,  d2}, { w2,  h2,  d2}, {-w2,  h2,  d2}, params.width, params.height);
    addFace(1, { w2, -h2, -d2}, {-w2, -h2, -d2}, {-w2,  h2, -d2}, { w2,  h2, -d2}, params.width, params.height);
    addFace(2, {-w2, -h2, -d2}, {-w2, -h2,  d2}, {-w2,  h2,  d2}, {-w2,  h2, -d2}, params.depth, params.height);
    addFace(3, { w2, -h2,  d2}, { w2, -h2, -d2}, { w2,  h2, -d2}, { w2,  h2,  d2}, params.depth, params.height);
    addFace(4, {-w2,  h2,  d2}, { w2,  h2,  d2}, { w2,  h2, -d2}, {-w2,  h2, -d2}, params.width, params.depth);
    addFace(5, {-w2, -h2, -d2}, { w2, -h2, -d2}, { w2, -h2,  d2}, {-w2, -h2,  d2}, params.width, params.depth);
    
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(3);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return mesh;
}

// ==========================================
// [DEBUG_ID: STAIR_GENERATOR]
// Local Anchor: bottom-center of the first step at (0,0,0)
// Stairs rise in +Y and extend into -Z.
// Layout in VAO: [all risers] [all treads] [railings if enabled]
// ==========================================
StairMesh generateStairGeometry(const StairParams& params) {
    // stair geometry function
    float stepH = params.height / params.numSteps;
    float stepD = params.depth  / params.numSteps;
    float hw    = params.width  / 2.0f;
    float rh    = params.railHeight;

    std::vector<Vertex> verts;

    auto addQuad = [&](glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 col) {
        glm::vec3 n = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        verts.push_back({p1, n, {0.0f, 0.0f}, col});
        verts.push_back({p2, n, {1.0f, 0.0f}, col});
        verts.push_back({p3, n, {1.0f, 1.0f}, col});
        verts.push_back({p1, n, {0.0f, 0.0f}, col});
        verts.push_back({p3, n, {1.0f, 1.0f}, col});
        verts.push_back({p4, n, {0.0f, 1.0f}, col});
    };

    // --- ALL RISERS ---
    int riserFirst = 0;
    for (int i = 0; i < params.numSteps; ++i) {
        float y0 = i * stepH;
        float z0 = -(i * stepD);
        addQuad({-hw, y0, z0}, {hw, y0, z0}, {hw, y0+stepH, z0}, {-hw, y0+stepH, z0}, params.riserColor);
    }
    int riserCount = (int)(verts.size()) - riserFirst;

    // --- ALL TREADS ---
    int treadFirst = (int)(verts.size());
    for (int i = 0; i < params.numSteps; ++i) {
        float y0 = i * stepH;
        float z0 = -(i * stepD);
        addQuad({-hw, y0+stepH, z0}, {hw, y0+stepH, z0}, {hw, y0+stepH, z0-stepD}, {-hw, y0+stepH, z0-stepD}, params.treadColor);
    }
    int treadCount = (int)(verts.size()) - treadFirst;

    // --- RAILINGS ---
    int railFirst = (int)(verts.size());
    int railCount = 0;
    if (params.hasRailings) {
        float topY = params.height;
        float botZ =  0.0f;
        float topZ = -params.depth;

        // LEFT side slant panel (X = -hw)
        addQuad(
            {-hw, 0.0f,    botZ},
            {-hw, rh,      botZ},
            {-hw, topY+rh, topZ},
            {-hw, topY,    topZ},
            params.railColor
        );
        // RIGHT side slant panel (X = +hw)
        addQuad(
            { hw, 0.0f,    botZ},
            { hw, topY,    topZ},
            { hw, topY+rh, topZ},
            { hw, rh,      botZ},
            params.railColor
        );

        float hcap = 0.08f; // handrail cap thickness
        float hcapX = 0.12f; // handrail cap width overhang

        // LEFT handrail top cap
        addQuad(
            {-hw-hcapX, rh,      botZ},
            {-hw+hcapX, rh,      botZ},
            {-hw+hcapX, topY+rh, topZ},
            {-hw-hcapX, topY+rh, topZ},
            params.railColor
        );
        // RIGHT handrail top cap
        addQuad(
            { hw-hcapX, rh,      botZ},
            { hw+hcapX, rh,      botZ},
            { hw+hcapX, topY+rh, topZ},
            { hw-hcapX, topY+rh, topZ},
            params.railColor
        );

        railCount = (int)(verts.size()) - railFirst;
    }

    // Upload
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);
    
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Position));
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    glEnableVertexAttribArray(2);
    // Color
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Color));
    glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return { vao, riserFirst, riserCount, treadFirst, treadCount, railFirst, railCount,
             params.riserColor, params.treadColor, params.railColor };
}

// place object
void placeObject(Shader& shader, const ProceduralMesh& mesh, const Transform& transform) {
    shader.setMat4("model", transform.toMatrix());
    shader.setBool("useVertexColor", true);
    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, (int)mesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void placeObject(Shader& shader, const StairMesh& mesh, const Transform& transform) {
    shader.setMat4("model", transform.toMatrix());
    shader.setBool("useVertexColor", true);
    glBindVertexArray(mesh.vao);
    // Risers
    shader.setVec3("objectColor", mesh.riserColor);
    glDrawArrays(GL_TRIANGLES, mesh.riserFirst, mesh.riserCount);
    // Treads
    shader.setVec3("objectColor", mesh.treadColor);
    glDrawArrays(GL_TRIANGLES, mesh.treadFirst, mesh.treadCount);
    // Railings
    if (mesh.railCount > 0) {
        shader.setVec3("objectColor", mesh.railColor);
        glDrawArrays(GL_TRIANGLES, mesh.railFirst, mesh.railCount);
    }
    glBindVertexArray(0);
}

// ==========================================
// [DEBUG_ID: SOFA_GENERATOR]
// Composite procedural mesh object.
// Generates the base parts once, assembled during placeObject.
// ==========================================
SofaMesh generateSofaGeometry(const SofaParams& params) {
    SofaMesh mesh;
    mesh.seats = params.seats;
    mesh.totalWidth = params.totalWidth;
    
    float armWidth = 0.2f;
    float totalDepth = 0.9f;
    float baseHeight = 0.2f;
    float seatHeight = 0.2f;
    float armHeight = 0.6f;
    float backHeight = 0.5f;
    
    float seatingWidth = params.totalWidth - (2.0f * armWidth);
    float seatW = seatingWidth / params.seats;
    
    // 1. Base Frame (long solid box)
    mesh.baseFrame = generateSuperellipsoid(seatingWidth/2.0f, baseHeight/2.0f, totalDepth/2.0f, 
                                            0.1f, 0.1f, 24, 16, params.frameColor);
                                            
    // 2. Armrests
    mesh.armRest = generateSuperellipsoid(armWidth/2.0f, armHeight/2.0f, totalDepth/2.0f, 
                                          0.1f, 0.1f, 24, 16, params.frameColor);
                                          
    // 3. Seat Cushion (wider, pillowy)
    mesh.seatCushion = generateSuperellipsoid((seatW - 0.02f)/2.0f, seatHeight/2.0f, (totalDepth - 0.2f)/2.0f, 
                                              0.3f, 0.8f, 24, 16, params.cushionColor);
                                              
    // 4. Back Cushion (taller, thinner)
    mesh.backCushion = generateSuperellipsoid((seatW - 0.02f)/2.0f, backHeight/2.0f, 0.1f, 
                                              0.3f, 0.8f, 24, 16, params.cushionColor);
                                              
    return mesh;
}

void placeObject(Shader& shader, const SofaMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();

    auto drawSubMesh = [&](const ProceduralMesh& part, glm::vec3 localPos, glm::vec3 localRot = glm::vec3(0.0f)) {
        glm::mat4 localMat = glm::translate(glm::mat4(1.0f), localPos);
        if (localRot.x != 0.0f) localMat = glm::rotate(localMat, glm::radians(localRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        if (localRot.y != 0.0f) localMat = glm::rotate(localMat, glm::radians(localRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        if (localRot.z != 0.0f) localMat = glm::rotate(localMat, glm::radians(localRot.z), glm::vec3(0.0f, 0.0f, 1.0f));
        
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    float armW = 0.2f;
    float baseH = 0.2f;
    float seatH = 0.2f;
    float armH = 0.6f;

    // 1. Base Frame
    drawSubMesh(mesh.baseFrame, glm::vec3(0.0f, baseH/2.0f, 0.0f));

    // 2. Armrests
    float armOffset = (mesh.totalWidth - armW) / 2.0f;
    drawSubMesh(mesh.armRest, glm::vec3(-armOffset, armH/2.0f, 0.0f));
    drawSubMesh(mesh.armRest, glm::vec3( armOffset, armH/2.0f, 0.0f));

    // 3. Cushions
    float seatW = (mesh.totalWidth - 2.0f * armW) / mesh.seats;
    float startX = -(mesh.totalWidth - 2.0f * armW) / 2.0f + seatW / 2.0f;

    for (int i = 0; i < mesh.seats; ++i) {
        float cx = startX + i * seatW;
        
        // Seat cushion (shifted slightly forward)
        // Seat depth = 0.7, centered at (0.9/2 - 0.7/2) = 0.45 - 0.35 = 0.1
        drawSubMesh(mesh.seatCushion, glm::vec3(cx, baseH + seatH/2.0f, 0.1f));
        
        // Back cushion (shifted slightly back, leaned back 8 degrees)
        // Seat top is at Y=0.40. Back cushion half-height is 0.25. 
        // Lowering center Y to 0.58 sinks the bottom edge into the seat for a connected look.
        drawSubMesh(mesh.backCushion, glm::vec3(cx, 0.58f, -0.32f), glm::vec3(8.0f, 0.0f, 0.0f));
    }
    glBindVertexArray(0);
}

// ==========================================
// [DEBUG_ID: GAZEBO_GENERATOR]
// Complete scene unit: round table + 4 chairs + playing items + dome on bars.
// Local anchor: scene center at (0, 0, 0), bottom at Y = 0.
//
// Dimensions:
//   Table  : radius 0.70, surface at Y=0.78
//   Pillars: 4x at radius 0.55 (between chairs), Y=0.78 to Y=1.80
//   Dome   : hemisphere radius 0.72, base at Y=1.80
//   Chairs : 4x at radius 0.90, angles 0/90/180/270 deg
// ==========================================
GazeboMesh generateGazeboGeometry(const GazeboParams& params) {
    GazeboMesh m;

    // ----- TABLE TOP (surface of revolution) -----
    // A flat disc with a slight rounded edge.
    // Profile goes from center (x=0) to outer rim, with a small lip.
    std::vector<glm::vec2> tableTopProfile = {
        {0.00f, 0.730f},   // center bottom
        {0.62f, 0.730f},   // flat underside
        {0.68f, 0.738f},   // bevel begin
        {0.70f, 0.753f},   // outer rim
        {0.70f, 0.775f},   // outer rim top
        {0.68f, 0.785f},   // bevel end
        {0.40f, 0.780f},   // top face inner
        {0.00f, 0.780f}    // top center
    };
    m.tableTop = generateSurfaceOfRevolution(tableTopProfile, 56, params.tableColor);

    // ----- TABLE PEDESTAL (surface of revolution) -----
    // Tapered stem with a flared foot and collar under the tabletop.
    std::vector<glm::vec2> pedestalProfile = {
        {0.00f, 0.000f},   // center bottom
        {0.22f, 0.000f},   // foot rim
        {0.22f, 0.030f},   // foot thickness
        {0.07f, 0.095f},   // foot-to-stem fillet
        {0.045f,0.650f},   // stem top (slight taper)
        {0.12f, 0.700f},   // top collar out
        {0.12f, 0.730f},   // top collar height
        {0.00f, 0.730f}    // center top
    };
    m.tablePedestal = generateSurfaceOfRevolution(pedestalProfile, 32, params.tableColor);

    // ----- DOME (hemisphere) -----
    // Large enough to cover the table (r=0.70) AND all chairs (r=0.78) with clearance.
    m.dome = generateHemisphere(1.5f, 56, 28, params.domeColor);

    // ----- PILLAR (4 drawn: thin square bar) -----
    // Pillar sizeY = 1.10 → spans from -1.10 to +1.10.
    // Center placed at Y = 1.10 → spans floor Y=0 to dome base Y=2.20.
    m.pillar = generateSuperellipsoid(0.030f, 1.100f, 0.030f,
                                      0.1f, 0.1f, 12, 8, params.pillarColor);

    // ----- CHAIR SEAT (flat pillow) -----
    m.chairSeat = generateSuperellipsoid(0.21f, 0.045f, 0.21f,
                                         0.3f, 0.4f, 28, 12, params.chairColor);

    // ----- CHAIR BACK (tall thin panel) -----
    m.chairBack = generateSuperellipsoid(0.21f, 0.230f, 0.030f,
                                         0.2f, 0.4f, 24, 12, params.chairColor);

    // ----- CHAIR LEG (thin round pillar) -----
    // sizeY = 0.215 → center at Y=0.215, spans Y=0 to 0.43.
    m.chairLeg = generateSuperellipsoid(0.022f, 0.215f, 0.022f,
                                        0.1f, 0.1f, 10, 6, params.tableColor);

    // ----- PLAYING CARD (very flat, slightly rounded rectangle) -----
    m.playCard = generateSuperellipsoid(0.068f, 0.004f, 0.095f,
                                        0.15f, 0.25f, 16, 8, params.cardColor);

    // ----- CHIP (small flat disc) -----
    m.chip = generateSuperellipsoid(0.038f, 0.012f, 0.038f,
                                    0.15f, 0.15f, 16, 8, params.chipColor);

    return m;
}

void placeObject(Shader& shader, const GazeboMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();

    // Helper: translate, then optionally rotate Y, then X (lean).
    // All positions are in local "gazebo" space (table center at origin).
    auto draw = [&](const ProceduralMesh& part,
                    glm::vec3 pos,
                    float rotYdeg = 0.0f,
                    float rotXdeg = 0.0f) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), pos);
        if (rotYdeg != 0.0f)
            m = glm::rotate(m, glm::radians(rotYdeg), glm::vec3(0.0f, 1.0f, 0.0f));
        if (rotXdeg != 0.0f)
            m = glm::rotate(m, glm::radians(rotXdeg), glm::vec3(1.0f, 0.0f, 0.0f));
        shader.setMat4("model", modelMat * m);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    // === TABLE ===
    draw(mesh.tableTop,      glm::vec3(0.0f, 0.0f, 0.0f));
    draw(mesh.tablePedestal, glm::vec3(0.0f, 0.0f, 0.0f));

    // === DOME (base at Y=2.20 — clears chair backs and covers everything) ===
    draw(mesh.dome, glm::vec3(0.0f, 2.20f, 0.0f));

    // === 4 PILLARS at 45, 135, 225, 315 deg (between chairs) ===
    // Floor Y=0 to dome Y=2.20 — bar center at Y=1.10, radius 1.25 (outside chairs)
    {
        const float pillarR  = 1.25f;
        const float pillarCY = 1.10f;
        for (int i = 0; i < 4; ++i) {
            float ang = glm::radians(45.0f + i * 90.0f);
            draw(mesh.pillar, glm::vec3(std::sin(ang) * pillarR, pillarCY, std::cos(ang) * pillarR));
        }
    }

    // === 4 CHAIRS at 0, 90, 180, 270 deg ===
    {
        // chair inner edge = chairR - seatHalfDepth(0.21) must clear table rim (r=0.70)
        // → min chairR = 0.91. Using 0.95 gives a small visible gap.
        const float chairR   = 0.95f;  // chairs sit just outside the table rim
        const float seatCY   = 0.45f;  // seat center height
        const float legCY    = 0.215f; // leg center height
        const float backCY   = 0.72f;  // backrest center height
        const float legLX    = 0.155f; // leg offset in chair local X
        const float legLZ    = 0.140f; // leg offset in chair local Z (toward/away table)

        for (int i = 0; i < 4; ++i) {
            float yawDeg = i * 90.0f;
            float yawRad = glm::radians(yawDeg);
            float s = std::sin(yawRad);
            float c = std::cos(yawRad);

            // Rotate a chair-local (lx, ly, lz) into gazebo space + chair base position.
            // Chair local: +X = lateral, +Z = toward table, -Z = behind sitter.
            auto L = [&](float lx, float ly, float lz) -> glm::vec3 {
                return glm::vec3(
                    chairR * s + c * lx + s * lz,   // world X
                    ly,
                    chairR * c - s * lx + c * lz    // world Z
                );
            };

            // Seat (symmetric, rotation for correctness)
            draw(mesh.chairSeat, L(0.0f, seatCY, 0.0f), yawDeg);

            // Backrest: behind sitter = chair local -Z direction.
            // Leans away from table (rotX = -12 in chair frame → rotX after rotY).
            draw(mesh.chairBack, L(0.0f, backCY, -0.185f), yawDeg, -12.0f);

            // 4 Legs per chair: at ±legLX, ±legLZ in chair space
            draw(mesh.chairLeg, L( legLX, legCY, legLZ));
            draw(mesh.chairLeg, L(-legLX, legCY, legLZ));
            draw(mesh.chairLeg, L( legLX, legCY, -legLZ));
            draw(mesh.chairLeg, L(-legLX, legCY, -legLZ));
        }
    }

    // === PLAYING CARDS on table surface (Y of card center = 0.784) ===
    {
        const float cardY = 0.784f;
        struct CP { float x, z, rotY; };
        CP cards[] = {
            { 0.18f,  0.12f,  22.0f },
            {-0.12f,  0.20f, -18.0f },
            { 0.22f, -0.18f,  55.0f },
            {-0.20f, -0.08f, -40.0f },
            { 0.05f,  0.30f,   8.0f },
        };
        for (auto& cp : cards)
            draw(mesh.playCard, glm::vec3(cp.x, cardY, cp.z), cp.rotY);
    }

    // === CHIPS scattered on table (Y = 0.792 — slightly above cards) ===
    {
        const float chipY = 0.792f;
        struct CP { float x, z; };
        CP chips[] = {
            { 0.38f,  0.10f },
            { 0.42f,  0.00f }, // stacked pair
            {-0.35f,  0.05f },
            { 0.08f, -0.42f },
            {-0.10f, -0.40f },
            { 0.32f, -0.30f },
        };
        for (auto& cp : chips)
            draw(mesh.chip, glm::vec3(cp.x, chipY, cp.z));
    }

    glBindVertexArray(0);
}

// ==========================================
// [DEBUG_ID: POTTED_TREE_GENERATOR]
// ==========================================
PottedTreeMesh generatePottedTreeGeometry(const PottedTreeParams& params) {
    PottedTreeMesh mesh;
    mesh.pot = generatePot(params.potBaseRadius, params.potTopRadius, params.potHeight, params.potBulge, 32, params.potColor);
    mesh.tree = generateFractalTree(params.treeParams);
    return mesh;
}

void placeObject(Shader& shader, const PottedTreeMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();

    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if (part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    // Draw Pot (at origin)
    drawSubMesh(mesh.pot, glm::mat4(1.0f));

    // Draw Tree (starts at origin so it rests inside the pot, emerging through the dirt level)
    drawSubMesh(mesh.tree.trunk, glm::mat4(1.0f));
    drawSubMesh(mesh.tree.foliage, glm::mat4(1.0f));

    glBindVertexArray(0);
}

// ==========================================
// [DEBUG_ID: LAMPPOST_GENERATOR]
// ==========================================
LamppostMesh generateLamppostGeometry(const LamppostParams& params) {
    LamppostMesh mesh;
    // Tweak to look like an architectural cylinder (n=0.2, e=1.0)
    mesh.pole = generateSuperellipsoid(params.poleRadius, params.height / 2.0f, params.poleRadius, 0.2f, 1.0f, 16, 8, params.poleColor);
    // Standard spherical bulb payload
    mesh.bulb = generateSphere(params.bulbRadius, 16, 8);
    return mesh;
}

void placeObject(Shader& shader, const LamppostMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();

    // Draw Pole
    // generator produced origin at 0,0,0, but height goes up to +/- height/2. Shift up!
    glm::mat4 poleMat = glm::translate(modelMat, glm::vec3(0.0f, 4.5f / 2.0f, 0.0f));
    shader.setMat4("model", poleMat);
    shader.setVec3("objectColor", mesh.pole.color);
    shader.setBool("useVertexColor", true);
    shader.setInt("textureMode", 0);
    glBindVertexArray(mesh.pole.vao);
    glDrawElements(GL_TRIANGLES, (int)mesh.pole.indexCount, GL_UNSIGNED_INT, 0);

    // Draw emissive bulb
    glm::mat4 bulbMat = glm::translate(modelMat, glm::vec3(0.0f, 4.5f, 0.0f));
    shader.setMat4("model", bulbMat);
    shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setBool("useVertexColor", false);
    shader.setInt("textureMode", 4); 
    glBindVertexArray(mesh.bulb.vao);
    glDrawElements(GL_TRIANGLES, (int)mesh.bulb.indexCount, GL_UNSIGNED_INT, 0);

    // restore
    shader.setBool("useVertexColor", true);
    glBindVertexArray(0);
}

// ==========================================
// [DEBUG_ID: SWING_SET_GENERATOR]
// ==========================================
SwingMesh generateSwingGeometry(const SwingParams& params) {
    SwingMesh mesh;
    mesh.ropeLength = params.ropeLength;
    mesh.frameWidth = params.frameWidth;
    mesh.frameHeight = params.frameHeight;
    mesh.seatWidth = params.seatWidth;

    // Frame Pillar (A-frame legs)
    mesh.framePillar = generateSuperellipsoid(0.08f, params.frameHeight/2.0f, 0.08f, 1.0f, 1.0f, 16, 16, params.frameColor);
    
    // Top bar
    mesh.frameBar = generateSuperellipsoid(params.frameWidth/2.0f + 0.4f, 0.1f, 0.1f, 1.0f, 1.0f, 16, 16, params.frameColor);

    // Bezier Rope (starts at Y=0, dips down to Y=-ropeLength)
    // We add slight curvature to mimic a natural hanging twisted rope
    glm::vec3 p0(0.0f, 0.0f, 0.0f);
    glm::vec3 p1(0.03f, -params.ropeLength * 0.3f, 0.05f); // slight curve
    glm::vec3 p2(-0.02f, -params.ropeLength * 0.6f, -0.02f);
    glm::vec3 p3(0.0f, -params.ropeLength, 0.0f);
    mesh.rope = generateBezierTube(p0, p1, p2, p3, 0.025f, 24, 8, params.ropeColor);

    // Seat (flat plank)
    mesh.seat = generateSuperellipsoid(params.seatWidth/2.0f, 0.05f, params.seatDepth/2.0f, 0.2f, 1.0f, 16, 16, params.seatColor);

    return mesh;
}

void placeObject(Shader& shader, const SwingMesh& mesh, const Transform& transform, float swingAngle) {
    glm::mat4 modelMat = transform.toMatrix();

    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if (part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    // 1. FRAME (Static)
    // Top Bar
    drawSubMesh(mesh.frameBar, glm::mat4(1.0f));
    
    // 4 Pillars (A-Frame configuration)
    float dx = mesh.frameWidth / 2.0f;
    
    auto leanPillar = [&](float px, float rotAngle) {
        // Anchor the pillar top to the top bar (px, 0, 0)
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(px, 0.0f, 0.0f));
        // Lean it outwards along the Z axis 
        m = glm::rotate(m, glm::radians(rotAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        // Drop the geometry so its center aligns with its visual bound
        m = glm::translate(m, glm::vec3(0.0f, -mesh.frameHeight / 2.0f, 0.0f));
        
        drawSubMesh(mesh.framePillar, m);
    };
    
    leanPillar(-dx,  15.0f); // Left Front
    leanPillar(-dx, -15.0f); // Left Back
    leanPillar( dx,  15.0f); // Right Front
    leanPillar( dx, -15.0f); // Right Back

    // 2. ASSEMBLY (Dynamic Pendulum)
    // Rotate the entire assembly around the local X axis at the pivot (0,0,0).
    glm::mat4 mSwing = glm::rotate(glm::mat4(1.0f), glm::radians(swingAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    
    float ropeAttachX = mesh.seatWidth/2.0f - 0.1f;
    
    // Ropes
    glm::mat4 mRopeL = glm::translate(mSwing, glm::vec3(-ropeAttachX, 0.0f, 0.0f));
    drawSubMesh(mesh.rope, mRopeL);

    glm::mat4 mRopeR = glm::translate(mSwing, glm::vec3(ropeAttachX, 0.0f, 0.0f));
    drawSubMesh(mesh.rope, mRopeR);

    // Seat
    glm::mat4 mSeat = glm::translate(mSwing, glm::vec3(0.0f, -mesh.ropeLength, 0.0f));
    drawSubMesh(mesh.seat, mSeat);

    glBindVertexArray(0);
}

// ==========================================
// BED GENERATOR
// ==========================================
BedMesh generateBedGeometry(const BedParams& params) {
    BedMesh mesh;
    // Frame
    mesh.frame = generateSuperellipsoid(params.width/2.0f, params.frameHeight/2.0f, params.length/2.0f, 0.05f, 0.05f, 24, 24, params.frameColor);
    // Mattress
    mesh.mattress = generateSuperellipsoid((params.width-0.1f)/2.0f, params.mattressHeight/2.0f, (params.length-0.1f)/2.0f, 0.2f, 0.2f, 24, 24, params.mattressColor);
    // Blanket (covers lower 2/3 of bed)
    mesh.blanket = generateSuperellipsoid((params.width+0.05f)/2.0f, (params.mattressHeight+0.02f)/2.0f, (params.length*0.6f)/2.0f, 0.2f, 0.2f, 24, 24, params.blanketColor);
    // Pillow 
    mesh.pillow = generateSuperellipsoid(0.4f, 0.08f, 0.25f, 0.4f, 0.8f, 24, 24, glm::vec3(0.95f, 0.95f, 0.95f));
    return mesh;
}

void placeObject(Shader& shader, const BedMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();
    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if (part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    drawSubMesh(mesh.frame, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.15f, 0.0f))); // Shift up by half height
    drawSubMesh(mesh.mattress, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f + 0.1f, 0.0f)));
    drawSubMesh(mesh.blanket, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f + 0.1f, 0.4f))); // shifted to foot
    
    // Two pillows at the head (Z = -0.8)
    drawSubMesh(mesh.pillow, glm::translate(glm::mat4(1.0f), glm::vec3(-0.4f, 0.3f + 0.2f, -0.8f))); 
    drawSubMesh(mesh.pillow, glm::translate(glm::mat4(1.0f), glm::vec3(0.4f, 0.3f + 0.2f, -0.8f))); 
    glBindVertexArray(0);
}

// ==========================================
// TABLE GENERATOR
// ==========================================
TableMesh generateTableGeometry(const TableParams& params) {
    TableMesh mesh;
    mesh.width = params.width; mesh.depth = params.depth; mesh.height = params.height;
    mesh.top = generateSuperellipsoid(params.width/2.0f, params.topThickness/2.0f, params.depth/2.0f, 0.1f, 0.1f, 24, 24, params.color);
    // Legs: thin cylinders using superellipsoid with n=1, e=0.1
    mesh.leg = generateSuperellipsoid(params.legRadius, params.height/2.0f, params.legRadius, 1.0f, 0.1f, 24, 24, params.color);
    return mesh;
}

void placeObject(Shader& shader, const TableMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();
    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if (part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    drawSubMesh(mesh.top, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, mesh.height, 0.0f)));
    
    float dx = mesh.width/2.0f - 0.1f;
    float dz = mesh.depth/2.0f - 0.1f;
    float dy = mesh.height/2.0f;
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3(-dx, dy, -dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3( dx, dy, -dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3(-dx, dy,  dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3( dx, dy,  dz)));
    glBindVertexArray(0);
}

// ==========================================
// CHAIR GENERATOR
// ==========================================
ChairMesh generateChairGeometry(const ChairParams& params) {
    ChairMesh mesh;
    mesh.seatHeight = params.seatHeight; mesh.backHeight = params.backHeight;
    mesh.width = params.width; mesh.depth = params.depth;
    
    mesh.seat = generateSuperellipsoid(params.width/2.0f, 0.03f, params.depth/2.0f, 0.1f, 0.1f, 24, 24, params.cushionColor);
    mesh.backrest = generateSuperellipsoid(params.width/2.0f, params.backHeight/2.0f, 0.03f, 0.1f, 0.1f, 24, 24, params.cushionColor);
    mesh.leg = generateSuperellipsoid(0.03f, params.seatHeight/2.0f, 0.03f, 1.0f, 0.1f, 24, 24, params.woodColor);
    return mesh;
}

void placeObject(Shader& shader, const ChairMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();
    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if(part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    drawSubMesh(mesh.seat, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, mesh.seatHeight, 0.0f)));
    drawSubMesh(mesh.backrest, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, mesh.seatHeight + mesh.backHeight/2.0f, -mesh.depth/2.0f + 0.03f)));
    
    float dx = mesh.width/2.0f - 0.05f;
    float dz = mesh.depth/2.0f - 0.05f;
    float dy = mesh.seatHeight/2.0f;
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3(-dx, dy, -dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3( dx, dy, -dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3(-dx, dy,  dz)));
    drawSubMesh(mesh.leg, glm::translate(glm::mat4(1.0f), glm::vec3( dx, dy,  dz)));
    glBindVertexArray(0);
}

// ==========================================
// DINING SET GENERATOR
// ==========================================
DiningSetMesh generateDiningSetGeometry(const DiningSetParams& params) {
    DiningSetMesh mesh;
    mesh.table = generateTableGeometry(params.tableDef);
    mesh.chair = generateChairGeometry(params.chairDef);
    mesh.numChairs = params.numChairs;
    return mesh;
}

void placeObject(Shader& shader, const DiningSetMesh& mesh, const Transform& transform) {
    // Draw table in center
    placeObject(shader, mesh.table, transform);
    
    // Draw 4 chairs around it
    float tableW = mesh.table.width;
    float tableD = mesh.table.depth;
    
    // Front side (facing -Z)
    Transform c1 = transform; c1.position += glm::vec3(-tableW/4.0f, 0.0f, tableD/2.0f + 0.3f);
    placeObject(shader, mesh.chair, c1);
    
    Transform c2 = transform; c2.position += glm::vec3(tableW/4.0f, 0.0f, tableD/2.0f + 0.3f);
    placeObject(shader, mesh.chair, c2);

    // Back side (facing +Z) -> rotate chairs 180 degrees around Y
    Transform c3 = transform; c3.position += glm::vec3(-tableW/4.0f, 0.0f, -tableD/2.0f - 0.3f);
    c3.rotation.y += 180.0f;
    placeObject(shader, mesh.chair, c3);

    Transform c4 = transform; c4.position += glm::vec3(tableW/4.0f, 0.0f, -tableD/2.0f - 0.3f);
    c4.rotation.y += 180.0f;
    placeObject(shader, mesh.chair, c4);
}

// ==========================================
// COMMODE GENERATOR
// ==========================================
CommodeMesh generateCommodeGeometry(const CommodeParams& params) {
    CommodeMesh mesh;
    // Bowl: surface of rev. Bottom radius 0.2, Top radius 0.4.
    std::vector<glm::vec2> profile;
    profile.push_back({0.0f, 0.0f});
    profile.push_back({0.2f, 0.0f});
    profile.push_back({0.2f, 0.2f});
    profile.push_back({0.4f, 0.5f});
    profile.push_back({0.4f, 0.6f}); //Rim
    profile.push_back({0.35f, 0.6f}); //Inner Rim
    profile.push_back({0.35f, 0.4f}); //Inner bowl
    profile.push_back({0.0f, 0.2f});
    mesh.bowl = generateSurfaceOfRevolution(profile, 32, params.porcelainColor);

    // Tank:
    mesh.tank = generateSuperellipsoid(0.35f, 0.4f, 0.15f, 0.1f, 0.1f, 24, 24, params.porcelainColor);
    
    // Lid/Seat:
    mesh.lid = generateSuperellipsoid(0.4f, 0.02f, 0.45f, 0.2f, 0.2f, 24, 24, glm::vec3(1.0f));
    return mesh;
}

void placeObject(Shader& shader, const CommodeMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();
    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if(part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    // Center is base. Bowl moves forward slightly.
    drawSubMesh(mesh.bowl, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.2f)));
    // Tank sits flush behind the bowl core.
    drawSubMesh(mesh.tank, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.4f, -0.2f))); 
    // Seat rests on top rim 
    drawSubMesh(mesh.lid, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.6f + 0.02f, 0.2f)));

    glBindVertexArray(0);
}

// ==========================================
// SHOWER GENERATOR
// ==========================================
ShowerMesh generateShowerGeometry(const ShowerParams& params) {
    ShowerMesh mesh;
    mesh.width = params.width; mesh.depth = params.depth; mesh.height = params.height;
    
    mesh.base = generateSuperellipsoid(params.width/2.0f, 0.05f, params.depth/2.0f, 0.1f, 0.1f, 24, 24, params.baseColor);
    mesh.glassWall = generateSuperellipsoid(params.width/2.0f, params.height/2.0f, 0.01f, 0.1f, 0.1f, 24, 24, params.glassColor);
    
    // Head pipe (Bezier)
    glm::vec3 p0(0, params.height - 0.2f, -params.depth/2.0f); // wall attach
    glm::vec3 p1(0, params.height - 0.1f, -params.depth/4.0f);
    glm::vec3 p2(0, params.height - 0.1f, 0.0f);
    glm::vec3 p3(0, params.height - 0.3f, 0.0f); // sprays down
    mesh.headPipe = generateBezierTube(p0, p1, p2, p3, 0.02f, 16, 8, glm::vec3(0.7f, 0.7f, 0.7f)); // chrome
    mesh.headNozzle = generateHemisphere(0.08f, 16, 8, glm::vec3(0.8f, 0.8f, 0.8f));
    
    return mesh;
}

void placeObject(Shader& shader, const ShowerMesh& mesh, const Transform& transform) {
    glm::mat4 modelMat = transform.toMatrix();
    auto drawSubMesh = [&](const ProceduralMesh& part, glm::mat4 localMat) {
        if(part.indexCount == 0) return;
        shader.setMat4("model", modelMat * localMat);
        shader.setVec3("objectColor", part.color);
        glBindVertexArray(part.vao);
        glDrawElements(GL_TRIANGLES, (int)part.indexCount, GL_UNSIGNED_INT, 0);
    };

    drawSubMesh(mesh.base, glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.05f, 0.0f)));
    
    // Left Glass Wall
    glm::mat4 LWall = glm::translate(glm::mat4(1.0f), glm::vec3(-mesh.width/2.0f + 0.01f, mesh.height/2.0f, 0.0f));
    LWall = glm::rotate(LWall, glm::radians(90.0f), glm::vec3(0,1,0));
    drawSubMesh(mesh.glassWall, LWall);
    // Right Glass Wall
    glm::mat4 RWall = glm::translate(glm::mat4(1.0f), glm::vec3(mesh.width/2.0f - 0.01f, mesh.height/2.0f, 0.0f));
    RWall = glm::rotate(RWall, glm::radians(90.0f), glm::vec3(0,1,0));
    drawSubMesh(mesh.glassWall, RWall);
    // Back Glass Wall
    glm::mat4 BWall = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, mesh.height/2.0f, -mesh.depth/2.0f + 0.01f));
    drawSubMesh(mesh.glassWall, BWall);

    // Showerhead
    drawSubMesh(mesh.headPipe, glm::mat4(1.0f));
    glm::mat4 nozzle = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, mesh.height - 0.3f, 0.0f));
    nozzle = glm::rotate(nozzle, glm::radians(180.0f), glm::vec3(1,0,0)); // Spray down
    drawSubMesh(mesh.headNozzle, nozzle);

    glBindVertexArray(0);
}
