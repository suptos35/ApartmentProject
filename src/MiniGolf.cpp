#include "MiniGolf.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

MiniGolfGame::MiniGolfGame() {
    resetBall();
    generateCourseGeometry();
}

MiniGolfGame::~MiniGolfGame() {
    if (courseGeo.vao != 0) {
        glDeleteBuffers(1, &courseGeo.vbo);
        glDeleteBuffers(1, &courseGeo.ebo);
        glDeleteVertexArrays(1, &courseGeo.vao);
    }
}

void MiniGolfGame::generateCourseGeometry() {
    // Build a simple flat course with walls
    // Ground plane + 4 walls + hole cylinder
    
    courseGeo.vertices.clear();
    courseGeo.indices.clear();
    
    float w = courseWidth / 2.0f;   // half-width
    float d = courseDepth / 2.0f;   // half-depth
    float h = wallHeight;
    float t = wallThickness;
    
    // Helper to add vertex (position only)
    auto addVert = [this](float x, float y, float z) {
        courseGeo.vertices.push_back(x);
        courseGeo.vertices.push_back(y);
        courseGeo.vertices.push_back(z);
    };
    
    // Helper to add quad (4 vertices -> 2 triangles)
    auto addQuad = [this](unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3) {
        unsigned int base = (unsigned int)courseGeo.indices.size();
        courseGeo.indices.push_back(i0);
        courseGeo.indices.push_back(i1);
        courseGeo.indices.push_back(i2);
        courseGeo.indices.push_back(i2);
        courseGeo.indices.push_back(i3);
        courseGeo.indices.push_back(i0);
    };
    
    unsigned int idx = 0;
    
    // ========== GROUND PLANE (flat green) ==========
    // Large rectangle at Y=0
    addVert(-w, 0.005f, -d);  idx++; // 0
    addVert( w, 0.005f, -d);  idx++; // 1
    addVert( w, 0.005f,  d);  idx++; // 2
    addVert(-w, 0.005f,  d);  idx++; // 3
    addQuad(0, 1, 2, 3);
    
    // ========== FRONT WALL (along -Z) ==========
    // Extended X to (-w - t) and (w + t) to seal the corners
    addVert(-w - t, 0.0f, -d);         idx++; // 4
    addVert( w + t, 0.0f, -d);         idx++; // 5
    addVert( w + t, h,    -d);         idx++; // 6
    addVert(-w - t, h,    -d);         idx++; // 7
    addQuad(4, 5, 6, 7);
    
    // ========== BACK WALL (along +Z, with hole) ==========
    float hx = holeRadius;
    
    // Left part of back wall (Extended left edge to seal corner)
    addVert(-w - t, 0.0f, d);          idx++; // 8
    addVert(-hx,    0.0f, d);          idx++; // 9
    addVert(-hx,    h,    d);          idx++; // 10
    addVert(-w - t, h,    d);          idx++; // 11
    addQuad(8, 9, 10, 11);
    
    // Right part of back wall (Extended right edge to seal corner)
    addVert( hx,    0.0f, d);          idx++; // 12
    addVert( w + t, 0.0f, d);          idx++; // 13
    addVert( w + t, h,    d);          idx++; // 14
    addVert( hx,    h,    d);          idx++; // 15
    addQuad(12, 13, 14, 15);
    
    // ========== LEFT WALL (along -X) ==========
    addVert(-w - t, 0.0f, -d);         idx++; // 16
    addVert(-w - t, 0.0f,  d);         idx++; // 17
    addVert(-w - t, h,     d);         idx++; // 18
    addVert(-w - t, h,    -d);         idx++; // 19
    addQuad(16, 17, 18, 19);
    
    // ========== RIGHT WALL (along +X) ==========
    addVert( w + t, 0.0f, -d);         idx++; // 20
    addVert( w + t, 0.0f,  d);         idx++; // 21
    addVert( w + t, h,     d);         idx++; // 22
    addVert( w + t, h,    -d);         idx++; // 23
    addQuad(20, 21, 22, 23);
    
    // ========== HOLE (dark cylinder at far end) ==========
    // Simple cylinder made of cone triangles
    int holeSegments = 8;
    unsigned int holeCenterTop = idx++;
    addVert(holePos.x, holePos.y + 0.05f, holePos.z);
    
    unsigned int holeRimStart = idx;
    for (int i = 0; i < holeSegments; i++) {
        float angle = 2.0f * 3.14159f * i / holeSegments;
        float x = holePos.x + holeRadius * std::cos(angle);
        float z = holePos.z + holeRadius * std::sin(angle);
        addVert(x, holePos.y, z);
        idx++;
    }
    
    // Connect cone to center
    for (int i = 0; i < holeSegments; i++) {
        unsigned int next = (i + 1) % holeSegments;
        courseGeo.indices.push_back(holeCenterTop);
        courseGeo.indices.push_back(holeRimStart + i);
        courseGeo.indices.push_back(holeRimStart + next);
    }
    
    // Setup VAO
    glGenVertexArrays(1, &courseGeo.vao);
    glGenBuffers(1, &courseGeo.vbo);
    glGenBuffers(1, &courseGeo.ebo);
    
    glBindVertexArray(courseGeo.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, courseGeo.vbo);
    glBufferData(GL_ARRAY_BUFFER, courseGeo.vertices.size() * sizeof(float),
                 courseGeo.vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, courseGeo.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, courseGeo.indices.size() * sizeof(unsigned int),
                 courseGeo.indices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void MiniGolfGame::resetBall() {
    ballPos = glm::vec3(-courseWidth / 3.0f, 0.15f, -courseDepth / 2.0f + 0.5f);
    ballVelocity = glm::vec3(0.0f);
    inProgress = false;
    aimAngleDeg = 45.0f;
}

void MiniGolfGame::shootBall() {
    if (inProgress) return; // Already moving
    
    float angleRad = glm::radians(aimAngleDeg);
    ballVelocity.x = shotPower * std::cos(angleRad);
    ballVelocity.z = shotPower * std::sin(angleRad);
    ballVelocity.y = 0.0f;
    inProgress = true;
    
    std::cout << "[GOLF] Shot! Angle: " << aimAngleDeg << "°, Speed: " << shotPower << std::endl;
}

void MiniGolfGame::updatePhysics(float deltaTime) {
    if (!inProgress) return;
    
    // Update position
    ballPos += ballVelocity * deltaTime;
    
    // Apply friction
    float speed = glm::length(ballVelocity);
    if (speed < velocityThreshold) {
        ballVelocity = glm::vec3(0.0f);
        inProgress = false;
    } else {
        ballVelocity *= friction;
    }
    
    // Keep ball on ground
    ballPos.y = ballRadius;
    
    // Check collisions
    checkWallCollision();
    checkHoleCollision();
}

void MiniGolfGame::checkWallCollision() {
    float w = courseWidth / 2.0f;
    float d = courseDepth / 2.0f;
    
    // Front wall (-Z direction)
    if (ballPos.z < -d + ballRadius) {
        ballPos.z = -d + ballRadius;
        ballVelocity.z = -ballVelocity.z * 0.8f;
    }
    
    // Back wall (+Z direction) - skip if near hole
    if (ballPos.z > d - ballRadius) {
        float distToHole = glm::distance(glm::vec2(ballPos.x, ballPos.z),
                                          glm::vec2(holePos.x, holePos.z));
        if (distToHole > holeRadius + ballRadius) {
            ballPos.z = d - ballRadius;
            ballVelocity.z = -ballVelocity.z * 0.8f;
        }
    }
    
    // Left wall (-X direction)
    if (ballPos.x < -w + ballRadius) {
        ballPos.x = -w + ballRadius;
        ballVelocity.x = -ballVelocity.x * 0.8f;
    }
    
    // Right wall (+X direction)
    if (ballPos.x > w - ballRadius) {
        ballPos.x = w - ballRadius;
        ballVelocity.x = -ballVelocity.x * 0.8f;
    }
}

void MiniGolfGame::checkHoleCollision() {
    glm::vec3 holeCenter = holePos;
    float dist = glm::distance(ballPos, holeCenter);
    
    if (dist < holeRadius) {
        std::cout << "[GOLF] Hole in one! Ball reset." << std::endl;
        resetBall();
    }
}

void MiniGolfGame::handleInput(int key, int action) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_J) {
            aimAngleDeg -= 5.0f;
            if (aimAngleDeg < 0.0f) aimAngleDeg += 360.0f;
            std::cout << "[GOLF] Aiming: " << aimAngleDeg << "°" << std::endl;
        } else if (key == GLFW_KEY_L) {
            aimAngleDeg += 5.0f;
            if (aimAngleDeg >= 360.0f) aimAngleDeg -= 360.0f;
            std::cout << "[GOLF] Aiming: " << aimAngleDeg << "°" << std::endl;
        } else if (key == GLFW_KEY_M) {
            shootBall();
        }
    }
}

void MiniGolfGame::update(float deltaTime) {
    updatePhysics(deltaTime);
}

void MiniGolfGame::render(Shader& shader, glm::mat4 projection, glm::mat4 view, glm::vec3 position) {
    shader.use();
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    
    // Draw course
    glm::mat4 courseModel = glm::mat4(1.0f);
    courseModel = glm::translate(courseModel, position); // Place at specified position
    shader.setMat4("model", courseModel);
    
    shader.setInt("textureMode", 0);
    shader.setBool("useVertexColor", false);
    
    glBindVertexArray(courseGeo.vao);
    
    // Draw ground (first quad)
    shader.setVec3("objectColor", courseGeo.groundColor);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);
    
    // Draw walls
    shader.setVec3("objectColor", courseGeo.wallColor);
    glDrawElements(GL_TRIANGLES, courseGeo.indices.size() - 6, GL_UNSIGNED_INT,
                   (void*)(6 * sizeof(unsigned int)));
    
    glBindVertexArray(0);
    
   // Draw ball (Using a generated mini-cube on the fly for self-containment)
   glm::mat4 ballModel = glm::translate(courseModel, ballPos);
   ballModel = glm::scale(ballModel, glm::vec3(ballRadius * 2.0f)); // Scale up slightly
   shader.setMat4("model", ballModel);
   shader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f)); // white ball
   
   // Quick 36-vertex cube definition for the ball
   float cubeVerts[] = {
       -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f,
       -0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f,-0.5f, 0.5f,
       -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f,-0.5f, -0.5f,-0.5f, 0.5f, -0.5f, 0.5f, 0.5f,
        0.5f, 0.5f, 0.5f,  0.5f, 0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f, 0.5f, 0.5f,
       -0.5f,-0.5f,-0.5f,  0.5f,-0.5f,-0.5f,  0.5f,-0.5f, 0.5f,  0.5f,-0.5f, 0.5f, -0.5f,-0.5f, 0.5f, -0.5f,-0.5f,-0.5f,
       -0.5f, 0.5f,-0.5f,  0.5f, 0.5f,-0.5f,  0.5f, 0.5f, 0.5f,  0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f,-0.5f
   };
   unsigned int bVAO, bVBO;
   glGenVertexArrays(1, &bVAO); glGenBuffers(1, &bVBO);
   glBindVertexArray(bVAO); glBindBuffer(GL_ARRAY_BUFFER, bVBO);
   glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVerts), cubeVerts, GL_STATIC_DRAW);
   glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
   glDrawArrays(GL_TRIANGLES, 0, 36);
   glDeleteBuffers(1, &bVBO); glDeleteVertexArrays(1, &bVAO);
    
    // Draw aiming line
    drawAimingLine(shader, projection, view);
}

void MiniGolfGame::drawAimingLine(Shader& shader, glm::mat4 projection, glm::mat4 view) {
    if (inProgress) return; // Don't show when moving
    
    float angleRad = glm::radians(aimAngleDeg);
    glm::vec3 aimDir = glm::vec3(std::cos(angleRad), 0.0f, std::sin(angleRad));
    
    // 1. The Aiming Ray (Red, pointing forward)
    glm::vec3 aimStart = ballPos;
    aimStart.y += 0.05f; // Lift above ground to prevent z-fighting
    glm::vec3 aimEnd = aimStart + aimDir * 2.0f; 
    
    // 2. The Golf Club / Stick (White, pointing backward)
    glm::vec3 clubStart = aimStart - (aimDir * 0.2f); // Slightly behind ball
    glm::vec3 clubEnd = clubStart - (aimDir * 0.8f);  // Length of the club
    
    std::vector<float> lineVerts = {
        // Aiming Ray (Vertices 0, 1)
        aimStart.x, aimStart.y, aimStart.z,
        aimEnd.x,   aimEnd.y,   aimEnd.z,
        // Golf Club (Vertices 2, 3)
        clubStart.x, clubStart.y, clubStart.z,
        clubEnd.x,   clubEnd.y,   clubEnd.z
    };
    
    unsigned int lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineVerts.size() * sizeof(float), lineVerts.data(), GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    
    // We do NOT translate this model matrix by the course position because 
    // ballPos is already in World Space due to how updatePhysics works currently.
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, -16.5f));
    
    shader.use();
    shader.setMat4("model", model);
    shader.setMat4("view", view);
    shader.setMat4("projection", projection);
    
    // Draw Aim Line
    shader.setVec3("objectColor", glm::vec3(1.0f, 0.1f, 0.1f)); // Bright Red
    glDrawArrays(GL_LINES, 0, 2);
    
    // Draw Club
    shader.setVec3("objectColor", glm::vec3(0.9f, 0.9f, 0.9f)); // White Stick
    glDrawArrays(GL_LINES, 2, 2);
    
    glDeleteBuffers(1, &lineVBO);
    glDeleteVertexArrays(1, &lineVAO);
}