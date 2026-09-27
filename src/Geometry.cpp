#include "Geometry.h"

unsigned int setupFloor() {
    // 1. Define the vertices for a simple square (2 triangles)
    // Format: X, Y, Z, NX, NY, NZ, U, V
    float vertices[] = {
        // Position                Normal        TexCoords
        -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,  0.0f, 0.0f,  // Bottom Left
         5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,  1.0f, 0.0f,  // Bottom Right
         5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  // Top Right
         
         5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,  1.0f, 1.0f,  // Top Right
        -5.0f, 0.0f,  5.0f,   0.0f, 1.0f, 0.0f,  0.0f, 1.0f,  // Top Left
        -5.0f, 0.0f, -5.0f,   0.0f, 1.0f, 0.0f,  0.0f, 0.0f   // Bottom Left
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    // Bind the Vertex Array Object first, then bind and set vertex buffer(s)
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // TexCoords attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    return VAO;
}

// === [DEBUG_ID: PROTOTYPE_UNIT_CUBE] ===
unsigned int setupUnitCube() {
    float vertices[] = {
        // Back face
        -0.5f, 0.0f, -0.5f,  // Bottom-left
         0.5f, 0.0f, -0.5f,  // Bottom-right
         0.5f, 1.0f, -0.5f,  // Top-right
         0.5f, 1.0f, -0.5f,  // Top-right
        -0.5f, 1.0f, -0.5f,  // Top-left
        -0.5f, 0.0f, -0.5f,  // Bottom-left
        // Front face
        -0.5f, 0.0f,  0.5f,  
         0.5f, 0.0f,  0.5f,  
         0.5f, 1.0f,  0.5f,  
         0.5f, 1.0f,  0.5f,  
        -0.5f, 1.0f,  0.5f,  
        -0.5f, 0.0f,  0.5f,  
        // Left face
        -0.5f, 1.0f,  0.5f,  
        -0.5f, 1.0f, -0.5f,  
        -0.5f, 0.0f, -0.5f,  
        -0.5f, 0.0f, -0.5f,  
        -0.5f, 0.0f,  0.5f,  
        -0.5f, 1.0f,  0.5f,  
        // Right face
         0.5f, 1.0f,  0.5f,  
         0.5f, 1.0f, -0.5f,  
         0.5f, 0.0f, -0.5f,  
         0.5f, 0.0f, -0.5f,  
         0.5f, 0.0f,  0.5f,  
         0.5f, 1.0f,  0.5f,  
        // Bottom face
        -0.5f, 0.0f, -0.5f,  
         0.5f, 0.0f, -0.5f,  
         0.5f, 0.0f,  0.5f,  
         0.5f, 0.0f,  0.5f,  
        -0.5f, 0.0f,  0.5f,  
        -0.5f, 0.0f, -0.5f,  
        // Top face
        -0.5f, 1.0f, -0.5f,  
         0.5f, 1.0f, -0.5f,  
         0.5f, 1.0f,  0.5f,  
         0.5f, 1.0f,  0.5f,  
        -0.5f, 1.0f,  0.5f,  
        -0.5f, 1.0f, -0.5f
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}

// === [DEBUG_ID: GLOBAL_AXES] ===
unsigned int setupAxes() {
    float vertices[] = {
        0.0f, 0.0f, 0.0f,
        50.0f, 0.0f, 0.0f, // X Line

        0.0f, 0.0f, 0.0f,
        0.0f, 50.0f, 0.0f, // Y Line

        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 50.0f  // Z Line
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    return VAO;
}