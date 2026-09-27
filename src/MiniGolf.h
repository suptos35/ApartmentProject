#ifndef MINIGOLF_H
#define MINIGOLF_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shader.h"

struct MiniGolfGeometry {
    unsigned int vao, vbo, ebo;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    
    glm::vec3 groundColor = glm::vec3(0.2f, 0.7f, 0.3f); // grass green
    glm::vec3 wallColor = glm::vec3(0.5f, 0.3f, 0.1f);   // wood brown
    glm::vec3 holeColor = glm::vec3(0.05f, 0.05f, 0.05f); // black hole
};

class MiniGolfGame {
public:
    // Constructor & initialization
    MiniGolfGame();
    ~MiniGolfGame();
    
    // Generate course geometry (flat rectangular box with walls and hole)
    void generateCourseGeometry();
    
    // Physics & logic
    void update(float deltaTime);
    void handleInput(int key, int action);
    void render(Shader& shader, glm::mat4 projection, glm::mat4 view, glm::vec3 position = glm::vec3(5.0f, 0.0f, -16.5f));
    
    // State queries
    bool isGameActive() const { return inProgress; }
    float getAimAngle() const { return aimAngleDeg; }
    
private:
    // Game state
    glm::vec3 ballPos;
    glm::vec3 ballVelocity;
    float ballRadius = 0.15f;
    bool inProgress = false; // True when ball is moving
    
    // Course parameters
    float courseWidth = 8.0f;  // X extent
    float courseDepth = 10.0f; // Z extent
    float wallHeight = 0.3f;
    float wallThickness = 0.2f;
    float holeRadius = 0.4f;
    glm::vec3 holePos = glm::vec3(0.0f, 0.01f, 4.0f); // At the far end
    
    // Aiming
    float aimAngleDeg = 45.0f; // 0-360 degrees
    float shotPower = 8.0f;    // Fixed power
    
    // Physics constants
    float friction = 0.98f;
    float velocityThreshold = 0.01f; // Below this, stop the ball
    
    // Geometry
    MiniGolfGeometry courseGeo;
    
    // Helper functions
    void resetBall();
    void shootBall();
    void updatePhysics(float deltaTime);
    void checkWallCollision();
    void checkHoleCollision();
    void drawAimingLine(Shader& shader, glm::mat4 projection, glm::mat4 view);
};

#endif
