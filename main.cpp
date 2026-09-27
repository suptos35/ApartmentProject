#include "src/Shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "src/Geometry.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <glm/gtc/matrix_transform.hpp>
#include "src/Camera.h"
#include "src/Scene.h"
#include "src/ProceduralMesh.h"
#include "src/TextureLoader.h"
#include "src/MiniGolf.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 800;

// Camera
Camera camera(glm::vec3(0.0f, 2.0f, 10.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Render Mode
enum RenderMode { RENDER_WORLD, RENDER_TESTBED };
RenderMode currentMode = RENDER_TESTBED;

// Testbed: which procedural object to display (0-19)
int testbedObjectIndex = 0;
const int TESTBED_OBJECT_COUNT = 20;

// Axis Debugging
bool showAxes = true;

// Lighting States
bool isDay = true;
bool dirLightOn = true;
bool pointLightOn = true;
bool spotLightOn = false;
bool usePhong = true;
bool diffuseEnabled = true;
bool specularEnabled = true;

// Keys for tracking toggles without rapid auto-fire
bool oKeyPressed = false;
bool key1Pressed = false;
bool key2Pressed = false;
bool key3Pressed = false;
bool gKeyPressed = false;
bool key4Pressed = false;
bool key5Pressed = false;
bool mKeyPressed = false;
bool screenshotRequested = false;
bool wireframeMode = false;

std::string buildScreenshotFilename() {
    std::time_t now = std::time(nullptr);
    std::tm localTime = *std::localtime(&now);

    std::ostringstream filename;
    filename << "screenshot_" << std::put_time(&localTime, "%Y%m%d_%H%M%S") << ".png";
    return filename.str();
}

void saveScreenshot(GLFWwindow* window, const std::string& filename) {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

    if (framebufferWidth <= 0 || framebufferHeight <= 0) {
        std::cout << "[SCREENSHOT] Failed: invalid framebuffer size." << std::endl;
        return;
    }

    std::vector<unsigned char> pixels(static_cast<size_t>(3 * framebufferWidth * framebufferHeight));

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, framebufferWidth, framebufferHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    stbi_flip_vertically_on_write(1);
    int success = stbi_write_png(filename.c_str(), framebufferWidth, framebufferHeight, 3, pixels.data(), framebufferWidth * 3);

    if (success) {
        std::cout << "[SCREENSHOT] Saved " << filename << std::endl;
    } else {
        std::cout << "[SCREENSHOT] Failed to save " << filename << std::endl;
    }
}

// --- Interaction Registry ---
std::vector<InteractivePoint> interactivePoints;

void tryInteract() {
    float nearDist = 3.5f;
    InteractivePoint* nearest = nullptr;
    for (auto& pt : interactivePoints) {
        float dist = glm::length(pt.worldPos - camera.Position);
        if (dist < nearDist) { nearDist = dist; nearest = &pt; }
    }
    if (nearest) {
        *nearest->isOpen = !(*nearest->isOpen);
        std::cout << "[INTERACT] " << nearest->label
                  << " is now " << (*nearest->isOpen ? "OPEN" : "CLOSED") << std::endl;
    }
}

void processInput(GLFWwindow *window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);
        
    static bool eKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        if (!eKeyPressed) { tryInteract(); eKeyPressed = true; }
    } else { eKeyPressed = false; }

    static bool xKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (!xKeyPressed) {
            showAxes = !showAxes;
            xKeyPressed = true;
        }
    } else {
        xKeyPressed = false;
    }

    static bool tKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        if (!tKeyPressed) {
            currentMode = (currentMode == RENDER_TESTBED) ? RENDER_WORLD : RENDER_TESTBED;
            tKeyPressed = true;
        }
    } else {
        tKeyPressed = false;
    }

    // Cycle testbed objects with N key
    static bool nKeyPressed = false;
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        if (!nKeyPressed) {
            testbedObjectIndex = (testbedObjectIndex + 1) % TESTBED_OBJECT_COUNT;
            const char* names[] = { "Hemisphere Dome", "Surface of Revolution (Vase)",
                                    "Spiral Staircase", "Superellipsoid (Sofa Cushion)", "Torus (Ring Lamp)",
                                    "Parametric Sofa", "Card Table Gazebo",
                                    "Tall Pine Tree", "Wide Oak Tree", "Potted Shrub",
                                    "Twisted Bonsai", "Alien Spindly Tree", "Interactive Dolna Swing",
                                    "Bed Frame", "Table", "Chair", "Dining Set", "Ceramic Commode", "Shower Cubicle", "Mini-Golf Game" };
            std::cout << "[TESTBED] Showing: " << names[testbedObjectIndex] << std::endl;
            nKeyPressed = true;
        }
    } else {
        nKeyPressed = false;
    }

    // Lighting Inputs
    if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        if (!oKeyPressed) { isDay = !isDay; std::cout << "[SYSTEM] " << (isDay ? "DAY" : "NIGHT") << " MODE" << std::endl; oKeyPressed = true; }
    } else { oKeyPressed = false; }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        if (!key1Pressed) { dirLightOn = !dirLightOn; std::cout << "[LIGHT] DirLight: " << (dirLightOn ? "ON" : "OFF") << std::endl; key1Pressed = true; }
    } else { key1Pressed = false; }
    
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        if (!key2Pressed) { pointLightOn = !pointLightOn; std::cout << "[LIGHT] PointLight: " << (pointLightOn ? "ON" : "OFF") << std::endl; key2Pressed = true; }
    } else { key2Pressed = false; }

    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        if (!key3Pressed) { spotLightOn = !spotLightOn; std::cout << "[LIGHT] Roof Lamppost Lights: " << (spotLightOn ? "ON" : "OFF") << std::endl; key3Pressed = true; }
    } else { key3Pressed = false; }

    if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
        if (!gKeyPressed) {
            usePhong = !usePhong;
            std::cout << "[SHADER] " << (usePhong ? "PHONG" : "GOURAUD") << " MODE" << std::endl;
            gKeyPressed = true;
        }
    } else { gKeyPressed = false; }

    if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        if (!key4Pressed) {
            diffuseEnabled = !diffuseEnabled;
            std::cout << "[LIGHT] Diffuse: " << (diffuseEnabled ? "ON" : "OFF") << std::endl;
            key4Pressed = true;
        }
    } else { key4Pressed = false; }

    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        if (!key5Pressed) {
            specularEnabled = !specularEnabled;
            std::cout << "[LIGHT] Specular: " << (specularEnabled ? "ON" : "OFF") << std::endl;
            key5Pressed = true;
        }
    } else { key5Pressed = false; }

    static bool f12Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F12) == GLFW_PRESS) {
        if (!f12Pressed) {
            screenshotRequested = true;
            f12Pressed = true;
        }
    } else {
        f12Pressed = false;
    }

    static bool f11Pressed = false;
    if (glfwGetKey(window, GLFW_KEY_F11) == GLFW_PRESS) {
        if (!f11Pressed) {
            wireframeMode = !wireframeMode;
            std::cout << "[RENDER] Polygon mode: " << (wireframeMode ? "WIRE" : "FILL") << std::endl;
            f11Pressed = true;
        }
    } else {
        f11Pressed = false;
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Utility function for loading a 6-image cubemap (stubbed for now)
unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    // STUB: We will load images via stb_image here later.
    // For now we just return the ID for the pipeline structure.
    for (unsigned int i = 0; i < 6; i++) {
        // glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, ...);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textureID;
}

void generateSphere(float radius, int sectorCount, int stackCount, std::vector<Vertex>& vertices, std::vector<unsigned int>& indices) {
    float x, y, z, xy;                              
    float nx, ny, nz, lengthInv = 1.0f / radius;    
    float s, t;                                     
    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for(int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;
        xy = radius * cosf(stackAngle);
        z = radius * sinf(stackAngle);

        for(int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;
            x = xy * cosf(sectorAngle);
            y = xy * sinf(sectorAngle);
            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            s = (float)j / sectorCount;
            t = (float)i / stackCount;
            Vertex vertex;
            vertex.Position = glm::vec3(x, y, z);
            vertex.Normal = glm::vec3(nx, ny, nz);
            vertex.TexCoords = glm::vec2(s, t);
            vertex.Color = glm::vec3(0.6f);
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
}

int main() {
    glfwInit();
    // REQUIRED: Tell GLFW we are using 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Apartment World 3.3", nullptr, nullptr);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLAD before calling any OpenGL functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    Shader ourShader("shaders/core.vert", "shaders/core.frag");
    Shader gouraudShader("shaders/gouraud.vert", "shaders/gouraud.frag");
    Shader roomShader("shaders/room.vert", "shaders/room.frag");
    Shader skyboxShader("shaders/skybox.vert", "shaders/skybox.frag");

    const unsigned int phongProgramID = ourShader.ID;
    const unsigned int gouraudProgramID = gouraudShader.ID;

    // --- SKYBOX SETUP ---
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    // --- STARS SETUP ---
    std::vector<glm::vec3> stars;
    for (int i=0; i<1500; i++) {
        float x = (rand() % 2000 - 1000) / 10.0f;
        float y = (rand() % 1000) / 10.0f; // upper half
        float z = (rand() % 2000 - 1000) / 10.0f;
        if (y > 20.0f) stars.push_back(glm::vec3(x, y, z));
    }
    unsigned int starVAO, starVBO;
    glGenVertexArrays(1, &starVAO);
    glGenBuffers(1, &starVBO);
    glBindVertexArray(starVAO);
    glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, stars.size() * sizeof(glm::vec3), stars.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    std::cout << "[SYSTEM] Stars generated: " << stars.size() << std::endl;

    unsigned int cubeVAO = setupUnitCube();
    ProceduralMesh celestialSphere = generateSphere(1.0f, 36, 18);

    // std::vector<std::string> faces = { ... }; 
    // unsigned int cubemapTexture = loadCubemap(faces);

    unsigned int woodTex = loadTexture("assets/textures/wood2.jpg");
    unsigned int brickExtTex = loadTexture("assets/textures/brick1.jpg");
    unsigned int brickFloorTex = loadTexture("assets/textures/brick2.jpg");

    ourShader.use();
    ourShader.setInt("imageTexture", 0);
    gouraudShader.use();
    gouraudShader.setInt("imageTexture", 0);
    roomShader.use();
    roomShader.setInt("imageTexture", 0);

    RoomParams r1;
    r1.width = 6.0f; r1.height = 6.0f; r1.depth = 12.0f;
    r1.faces[0].color = glm::vec3(0.8f, 0.2f, 0.2f); // Red Front
    r1.faces[0].isVisible = true;
    r1.faces[0].holes.push_back({ 2.0f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room1 Front Door" });

    r1.faces[1].isVisible = false; // face[1] not visible (Back face)
    r1.faces[1].color = glm::vec3(0.2f, 0.8f, 0.2f); // Green Back

    r1.faces[2].color = glm::vec3(0.2f, 0.2f, 0.8f); // Blue Left
    r1.faces[2].isVisible = true;
    r1.faces[2].holes.push_back({ 2.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room1 Left Window 1" });
    r1.faces[2].holes.push_back({ 7.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room1 Left Window 2" });

    r1.faces[3].color = glm::vec3(0.8f, 0.8f, 0.2f); // Yellow Right
    r1.faces[3].isVisible = true;

    r1.faces[4].color = glm::vec3(0.2f, 0.8f, 0.8f); // Cyan Top
    r1.faces[4].isVisible = true;

    r1.faces[5].color = glm::vec3(0.8f, 0.2f, 0.8f); // Magenta Bottom
    r1.faces[5].isVisible = true;
    RoomMesh room1Mesh = generateRoomGeometry(r1);

    RoomParams r2;
    r2.width = 12.0f; r2.height = 6.0f; r2.depth = 6.0f;
    r2.faces[0].color = glm::vec3(1.0f, 0.5f, 0.0f); // Orange Front
    r2.faces[0].isVisible = true;
    r2.faces[0].holes.push_back({ 2.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room2 Front Window 1" });
    r2.faces[0].holes.push_back({ 7.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room2 Front Window 2" });

    r2.faces[1].color = glm::vec3(0.5f, 0.0f, 0.5f); // Purple Back
    r2.faces[1].isVisible = true;

    r2.faces[2].color = glm::vec3(0.0f, 0.5f, 0.5f); // Teal Left
    r2.faces[2].isVisible = true;

    r2.faces[3].isVisible = false; // Right not visible
    r2.faces[3].color = glm::vec3(1.0f, 0.8f, 0.8f); // Pink Right

    r2.faces[4].color = glm::vec3(0.9f, 0.9f, 0.9f); // White Top
    r2.faces[4].isVisible = true;

    r2.faces[5].color = glm::vec3(0.2f, 0.2f, 0.2f); // Dark Grey Bottom
    r2.faces[5].isVisible = true;
    RoomMesh room2Mesh = generateRoomGeometry(r2);

    RoomParams r3;
    r3.width = 23.0f; r3.height = 6.0f; r3.depth = 10.0f;
    r3.faces[0].color = glm::vec3(1.0f, 0.5f, 0.0f); // Orange Front
    

    r3.faces[1].color = glm::vec3(0.5f, 0.0f, 0.5f); // Purple Back
    

    r3.faces[2].color = glm::vec3(0.0f, 0.5f, 0.5f); // Teal Left
    

    
    r3.faces[3].color = glm::vec3(1.0f, 0.8f, 0.8f); // Pink Right

    r3.faces[4].color = glm::vec3(0.9f, 0.9f, 0.9f); // White Top
   

    r3.faces[5].color = glm::vec3(0.2f, 0.2f, 0.2f); // Dark Grey Bottom
    
    for (int i=0; i<6; ++i) {
        r3.faces[i].isVisible = true;
        
    }
    r3.faces[2].isVisible = true;
    r3.faces[3].isVisible = false;// face[2] Left not visible

    // face[0] Front: left edge 6x6 opening (EDGE_CONNECT) + door 2 units from right
    r3.faces[0].holes.push_back({ 0.0f, 0.0f, 6.0f, 6.0f, Hole::EDGE_CONNECT, true, "" });
    r3.faces[0].holes.push_back({ 18.5f, 0.0f, 2.5f, 3.0f, Hole::DOOR, false, "Room3 Front Door" });

    // face[1] Back: three equally spaced windows
    r3.faces[1].holes.push_back({ 2.75f, 2.0f, 4.0f, 2.0f, Hole::WINDOW, true, "Room3 Back Window 1" });
    r3.faces[1].holes.push_back({ 9.5f,  2.0f, 4.0f, 2.0f, Hole::WINDOW, true, "Room3 Back Window 2" });
    r3.faces[1].holes.push_back({ 16.25f,2.0f, 4.0f, 2.0f, Hole::WINDOW, true, "Room3 Back Window 3" });
    RoomMesh room3Mesh = generateRoomGeometry(r3);

    // ==========================================
    // [DEBUG_ID: ROOM_4]
    // h=6, w=6, d=22. Left face[2] has two edge-connected openings (10 wide from left, 6 wide from right)
    // ==========================================
    RoomParams r4;
    r4.width = 6.0f; r4.height = 6.0f; r4.depth = 22.0f;

    // face[0] Front (Z+)
    r4.faces[0].color = glm::vec3(0.9f, 0.2f, 0.2f); // Red
    r4.faces[0].isVisible = true;

    // face[1] Back (Z-)
    r4.faces[1].color = glm::vec3(0.2f, 0.8f, 0.2f); // Green
    r4.faces[1].isVisible = true;

    // face[2] Left (X-), depth=22 so UV U spans 0->22
    // Hole 1: 10 wide from left edge (height=6, full height) -> {0, 0, 10, 6}
    // Hole 2: 6 wide from right edge (height=6) -> starts at 22-6=16 -> {16, 0, 6, 6}
    r4.faces[2].color = glm::vec3(0.2f, 0.2f, 0.9f); // Blue
    r4.faces[2].isVisible = true;
    r4.faces[2].holes.push_back({ 0.0f, 0.0f, 10.0f, 6.0f, Hole::EDGE_CONNECT, true, "" });  // Left edge opening
    r4.faces[2].holes.push_back({ 16.0f, 0.0f, 6.0f, 6.0f, Hole::EDGE_CONNECT, true, "" }); // Right edge opening

    // face[3] Right (X+) - 2 windows, depth=22, equally spaced
    // Two windows 3 wide 2 tall. 22-6=16 space left. 16/3 gap = ~5.3 -> place at 5, 14
    r4.faces[3].color = glm::vec3(0.8f, 0.8f, 0.1f); // Yellow
    r4.faces[3].isVisible = true;
    r4.faces[3].holes.push_back({ 5.0f,  2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room4 Right Window 1" });
    r4.faces[3].holes.push_back({ 14.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room4 Right Window 2" });

    // face[4] Top
    r4.faces[4].color = glm::vec3(0.85f, 0.85f, 0.85f); // Light grey
    r4.faces[4].isVisible = true;

    // face[5] Bottom
    r4.faces[5].color = glm::vec3(0.15f, 0.15f, 0.15f); // Dark grey
    r4.faces[5].isVisible = true;
    RoomMesh room4Mesh = generateRoomGeometry(r4);

    // ==========================================
    // [DEBUG_ID: ROOM_5]
    // Second floor, +Z direction (Front half)
    // ==========================================
    RoomParams r5;
    r5.width = 4.0f; r5.height = 6.0f; r5.depth = 11.0f;
    r5.faces[0].color = glm::vec3(0.7f, 0.3f, 0.3f); // Dull red
    r5.faces[0].isVisible = true;

    // Shared middle wall - we'll make Room 5's back wall visible and Room 6's front invisible
    r5.faces[1].color = glm::vec3(0.6f, 0.6f, 0.6f); // Grey
    r5.faces[1].isVisible = true;
    r5.faces[1].holes.push_back({ 1.0f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room5 Back Door" });

    // Left wall with door
    r5.faces[2].color = glm::vec3(0.3f, 0.7f, 0.3f); // Dull green
    r5.faces[2].isVisible = true;
    r5.faces[2].holes.push_back({ 4.5f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room5 Left Door" });

    // Right wall with window
    r5.faces[3].color = glm::vec3(0.3f, 0.3f, 0.7f); // Dull blue
    r5.faces[3].isVisible = true;
    r5.faces[3].holes.push_back({ 4.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room5 Right Window" });

    r5.faces[4].color = glm::vec3(0.9f, 0.9f, 0.9f); // Ceiling
    r5.faces[4].isVisible = true;
    r5.faces[5].isVisible = false; // Floor invisible to use roof
    RoomMesh room5Mesh = generateRoomGeometry(r5);

    // ==========================================
    // [DEBUG_ID: ROOM_6]
    // Second floor, -Z direction (Back half)
    // ==========================================
    RoomParams r6;
    r6.width = 4.0f; r6.height = 6.0f; r6.depth = 11.0f;
    r6.faces[0].isVisible = false; // Shared wall from R5

    r6.faces[1].color = glm::vec3(0.7f, 0.5f, 0.2f); // Orange
    r6.faces[1].isVisible = true;

    r6.faces[2].color = glm::vec3(0.3f, 0.7f, 0.3f); // Dull green
    r6.faces[2].isVisible = true;

    r6.faces[3].color = glm::vec3(0.3f, 0.3f, 0.7f); // Dull blue
    r6.faces[3].isVisible = true;
    r6.faces[3].holes.push_back({ 4.0f, 2.0f, 3.0f, 2.0f, Hole::WINDOW, true, "Room6 Right Window" });

    r6.faces[4].color = glm::vec3(0.9f, 0.9f, 0.9f); // Ceiling
    r6.faces[4].isVisible = true;
    r6.faces[5].isVisible = false; // Floor invisible
    RoomMesh room6Mesh = generateRoomGeometry(r6);

    // ==========================================
    // [DEBUG_ID: ROOM_7]
    // Second floor, front side (large bedroom/living)
    // Roof: X[-10,19] Z[-22,0]. Rooms5/6: X[15,19]. Stair hole: X[-4,7] Z[-12,-9]
    // w=9, h=6, d=8 -> center (4.5, 9.001, -4.0) -> X[0,9], Z[-8,0] -- all free
    // ==========================================
    RoomParams r7;
    r7.width = 9.0f; r7.height = 6.0f; r7.depth = 8.0f;

    r7.faces[0].color = glm::vec3(0.75f, 0.65f, 0.55f); // Warm beige Front
    r7.faces[0].isVisible = true;
    r7.faces[0].holes.push_back({ 3.5f, 1.5f, 2.0f, 1.5f, Hole::WINDOW, true, "Room7 Front Window" });

    r7.faces[1].color = glm::vec3(0.60f, 0.55f, 0.70f); // Lavender Back
    r7.faces[1].isVisible = true;
    r7.faces[1].holes.push_back({ 3.5f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room7 Back Door" });

    r7.faces[2].color = glm::vec3(0.55f, 0.70f, 0.65f); // Teal Left
    r7.faces[2].isVisible = true;
    r7.faces[2].holes.push_back({ 1.5f, 1.5f, 2.0f, 1.5f, Hole::WINDOW, true, "Room7 Left Window" });

    r7.faces[3].color = glm::vec3(0.70f, 0.55f, 0.65f); // Mauve Right
    r7.faces[3].isVisible = true;
    r7.faces[3].holes.push_back({ 2.5f, 1.5f, 2.0f, 1.5f, Hole::WINDOW, true, "Room7 Right Window" });

    r7.faces[4].color = glm::vec3(0.92f, 0.92f, 0.92f); // Off-white Ceiling
    r7.faces[4].isVisible = true;
    r7.faces[5].isVisible = false; // Floor: roof slab beneath
    RoomMesh room7Mesh = generateRoomGeometry(r7);

    // ==========================================
    // [DEBUG_ID: ROOM_8]
    // Second floor, front side (small bathroom)
    // w=4, h=6, d=5 -> center (-5.0, 9.001, -2.5) -> X[-7,-3], Z[-5,0] -- all free
    // ==========================================
    RoomParams r8;
    r8.width = 4.0f; r8.height = 6.0f; r8.depth = 5.0f;

    r8.faces[0].color = glm::vec3(0.70f, 0.80f, 0.90f); // Light blue Front
    r8.faces[0].isVisible = true;
    r8.faces[0].holes.push_back({ 1.0f, 3.5f, 2.0f, 1.0f, Hole::WINDOW, true, "Room8 Front Window" });

    r8.faces[1].color = glm::vec3(0.70f, 0.80f, 0.90f); // Light blue Back
    r8.faces[1].isVisible = true;

    r8.faces[2].color = glm::vec3(0.75f, 0.82f, 0.90f); // Light blue Left
    r8.faces[2].isVisible = true;

    r8.faces[3].color = glm::vec3(0.75f, 0.82f, 0.90f); // Light blue Right (door to corridor)
    r8.faces[3].isVisible = true;
    r8.faces[3].holes.push_back({ 1.0f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room8 Bathroom Door" });

    r8.faces[4].color = glm::vec3(0.92f, 0.92f, 0.92f); // Off-white Ceiling
    r8.faces[4].isVisible = true;
    r8.faces[5].isVisible = false; // Floor: roof slab beneath
    RoomMesh room8Mesh = generateRoomGeometry(r8);

    // ==========================================
    // [DEBUG_ID: STAIR_HOLE_RAILINGS]
    // Stair hole: World X[-4,7], Z[-12,-9]
    // Railings sit ON second floor: bottom Y=6.001, h=1.0 -> center Y=6.501
    // Only the face that borders the walking area is shown + top cap
    // ==========================================
    glm::vec3 railingCol = glm::vec3(0.85f, 0.80f, 0.75f);

    // Front railing (at Z=-9 hole edge, floor-side center Z=-8.925)
    RoomParams railFrontDef;
    railFrontDef.width = 11.0f; railFrontDef.height = 1.0f; railFrontDef.depth = 0.15f;
    for (int i=0;i<6;i++){railFrontDef.faces[i].color=railingCol; railFrontDef.faces[i].isVisible=false;}
    railFrontDef.faces[0].isVisible = true; // Z+ (toward open floor)
    railFrontDef.faces[1].isVisible = true; // Z- (toward hole)
    railFrontDef.faces[4].isVisible = true; // Top cap
    RoomMesh railFrontMesh = generateRoomGeometry(railFrontDef);

    // Back railing (at Z=-12 hole edge, floor-side center Z=-12.075)
    RoomParams railBackDef;
    railBackDef.width = 11.0f; railBackDef.height = 1.0f; railBackDef.depth = 0.15f;
    for (int i=0;i<6;i++){railBackDef.faces[i].color=railingCol; railBackDef.faces[i].isVisible=false;}
    railBackDef.faces[0].isVisible = true;
    railBackDef.faces[1].isVisible = true;
    railBackDef.faces[4].isVisible = true;
    RoomMesh railBackMesh = generateRoomGeometry(railBackDef);

   

    // ==========================================
    // [DEBUG_ID: ROOM_9]
    // Second floor, small room on back edge (Z=-22)
    // w=8, h=6, d=4 -> center (-6.0, 9.001, -20.0) -> X[-10,-2], Z[-22,-18]
    // face[2](Left) at X=-10: forms left edge wall for back section
    // face[1](Back) at Z=-22: forms back edge wall for its section
    // ==========================================
    RoomParams r9;
    r9.width = 8.0f; r9.height = 6.0f; r9.depth = 4.0f;

    r9.faces[0].color = glm::vec3(0.60f, 0.70f, 0.80f); // Front (interior)
    r9.faces[0].isVisible = true;
    r9.faces[0].holes.push_back({ 3.0f, 0.0f, 2.0f, 3.0f, Hole::DOOR, false, "Room9 Front Door" });

    r9.faces[1].color = glm::vec3(0.55f, 0.65f, 0.75f); // Back (exterior)
    r9.faces[1].isVisible = true;
    r9.faces[1].holes.push_back({ 3.0f, 2.0f, 2.0f, 1.5f, Hole::WINDOW, true, "Room9 Back Window" });

    r9.faces[2].color = glm::vec3(0.60f, 0.70f, 0.80f); // Left (exterior at X=-10)
    r9.faces[2].isVisible = true;

    r9.faces[3].color = glm::vec3(0.60f, 0.70f, 0.80f); // Right (interior)
    r9.faces[3].isVisible = true;
    // Deleted Right Door to make it a dedicated private bathroom

    r9.faces[4].color = glm::vec3(0.92f, 0.92f, 0.92f); // Ceiling
    r9.faces[4].isVisible = true;
    r9.faces[5].isVisible = false; // Floor: roof slab beneath
    RoomMesh room9Mesh = generateRoomGeometry(r9);

    // ==========================================
    // [DEBUG_ID: PERIMETER_WALLS]
    // Second floor edge walls: h=6, floor Y=6.001 -> center Y=9.001
    //
    // Front edge (Z=0) -- empty segments not covered by rooms:
    //   Seg A: X[-10,-7] w=3, center X=-8.5, Z=-0.075
    //   Seg B: X[-3, 0] w=3, center X=-1.5, Z=-0.075  (reuses Seg A mesh)
    //   Seg C: X[ 9,15] w=6, center X=12.0, Z=-0.075
    //
    // Left edge (X=-10) -- Z[-18,0] (Room9 face[2] covers [-22,-18]):
    //   Single wall, depth=18, center X=-9.925, Z=-9.0, large window on face[3]
    //
    // Back edge (Z=-22) -- X[-2,15] (Room9 face[1] at [-10,-2], Room6 at [15,19]):
    //   Single wall, width=17, center X=6.5, Z=-21.925
    // ==========================================
    glm::vec3 wallCol = glm::vec3(0.75f, 0.72f, 0.68f);

    // Front perimeter wall Seg A (w=3) -- also reused for Seg B
    RoomParams wallFrontA;
    wallFrontA.width = 3.0f; wallFrontA.height = 6.0f; wallFrontA.depth = 0.15f;
    for (int i=0;i<6;i++){wallFrontA.faces[i].color=wallCol; wallFrontA.faces[i].isVisible=false;}
    wallFrontA.faces[0].isVisible = true; // Z+ exterior
    wallFrontA.faces[1].isVisible = true; // Z- interior
    wallFrontA.faces[4].isVisible = true; // Top
    RoomMesh wallFrontAMesh = generateRoomGeometry(wallFrontA);

    // Front perimeter wall Seg C (w=6)
    RoomParams wallFrontC;
    wallFrontC.width = 6.0f; wallFrontC.height = 6.0f; wallFrontC.depth = 0.15f;
    for (int i=0;i<6;i++){wallFrontC.faces[i].color=wallCol; wallFrontC.faces[i].isVisible=false;}
    wallFrontC.faces[0].isVisible = true;
    wallFrontC.faces[1].isVisible = true;
    wallFrontC.faces[4].isVisible = true;
    RoomMesh wallFrontCMesh = generateRoomGeometry(wallFrontC);

    // Left edge wall — exterior door (face[2]) + matching interior hole (face[3])
    // UV center for both faces at U=8-10 → world Z ≈ -9 (wall center)
    // Door: W=2, H=4.5, sits on floor (V=0) of the second-floor wall
    RoomParams wallLeft;
    wallLeft.width = 0.15f; wallLeft.height = 6.0f; wallLeft.depth = 18.0f;
    for (int i=0;i<6;i++){wallLeft.faces[i].color=wallCol; wallLeft.faces[i].isVisible=false;}
    wallLeft.faces[2].isVisible = true; // X- exterior
    wallLeft.faces[2].holes.push_back({ 8.0f, 0.0f, 2.0f, 4.5f, Hole::DOOR, false, "Floor1 Left Exterior Door" });
    wallLeft.faces[3].isVisible = true; // X+ interior
    wallLeft.faces[3].holes.push_back({ 8.0f, 0.0f, 2.0f, 4.5f, Hole::DOOR, false, "Floor1 Left Interior Door" });
    wallLeft.faces[4].isVisible = true; // Top
    RoomMesh wallLeftMesh = generateRoomGeometry(wallLeft);

    // Back edge wall (width=17)
    RoomParams wallBack;
    wallBack.width = 17.0f; wallBack.height = 6.0f; wallBack.depth = 0.15f;
    for (int i=0;i<6;i++){wallBack.faces[i].color=wallCol; wallBack.faces[i].isVisible=false;}
    wallBack.faces[0].isVisible = true; // Z+ interior
    wallBack.faces[1].isVisible = true; // Z- exterior
    wallBack.faces[4].isVisible = true; // Top
    RoomMesh wallBackMesh = generateRoomGeometry(wallBack);

    // ==========================================
    // [DEBUG_ID: LEFT_WALL_BALCONY]
    // Second-floor balcony attached to the exterior door on the left wall.
    // Extends from X=-10 (wall) to X=-14 (outside). Z=-7.5 to Z=-10.5.
    // Floor at Y=6.001. Spiral stair column is underneath at (-12.2, 0, -9).
    // ==========================================
    glm::vec3 balconyCol = glm::vec3(0.72f, 0.70f, 0.68f);

    // Floor slab: 4m(X) x 0.15m(H) x 3m(Z). Top face = walking surface at Y=6.001.
    RoomParams balconyFloorDef;
    balconyFloorDef.width = 4.0f; balconyFloorDef.height = 0.15f; balconyFloorDef.depth = 3.0f;
    for (int i=0;i<6;i++){balconyFloorDef.faces[i].color=balconyCol; balconyFloorDef.faces[i].isVisible=true;}
    balconyFloorDef.faces[3].isVisible = false; // X+ face toward building = hidden
    RoomMesh balconyFloorMesh = generateRoomGeometry(balconyFloorDef);

    // Left/right railings: 4m(X) x 0.9m(H) x 0.1m(Z). Placed on Z edges.
    RoomParams balconyRailSideDef;
    balconyRailSideDef.width = 4.0f; balconyRailSideDef.height = 0.9f; balconyRailSideDef.depth = 0.1f;
    for (int i=0;i<6;i++){balconyRailSideDef.faces[i].color=balconyCol; balconyRailSideDef.faces[i].isVisible=true;}
    RoomMesh balconyRailSideMesh = generateRoomGeometry(balconyRailSideDef);

    // Left railing (Z- edge, Z=-10.5) WAS REMOVED for the stair.
    // Right railing (Z+ edge, Z=-7.5)
    Transform tBalcRailR;
    tBalcRailR.position = glm::vec3(-12.0f, 6.451f, -7.475f);
    placeObject(roomShader, balconyRailSideMesh, tBalcRailR);
    
    // Front railing: 0.1m(X) x 0.9m(H) x 3m(Z). Placed on far X- edge.
    RoomParams balconyRailFrontDef;
    balconyRailFrontDef.width = 0.1f; balconyRailFrontDef.height = 0.9f; balconyRailFrontDef.depth = 3.0f;
    for (int i=0;i<6;i++){balconyRailFrontDef.faces[i].color=balconyCol; balconyRailFrontDef.faces[i].isVisible=true;}
    RoomMesh balconyRailFrontMesh = generateRoomGeometry(balconyRailFrontDef);
    unsigned int floorVAO = setupFloor();
    // (cubeVAO is initialized earlier)
    unsigned int axesVAO = setupAxes();

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // --- Register interactive holes (run once at startup) ---
    Transform t1_reg; t1_reg.position = glm::vec3(-7.0f, 3.0f, -6.0f);
    registerRoomInteractions(room1Mesh, t1_reg, interactivePoints);
    Transform t2_reg; t2_reg.position = glm::vec3(7.0f, 3.0f, -3.0f);
    registerRoomInteractions(room2Mesh, t2_reg, interactivePoints);
    Transform t3_reg; t3_reg.position = glm::vec3(1.5f, 3.0f, -17.0f);
    registerRoomInteractions(room3Mesh, t3_reg, interactivePoints);
    Transform t4_reg; t4_reg.position = glm::vec3(16.0f, 3.0f, -11.0f);
    registerRoomInteractions(room4Mesh, t4_reg, interactivePoints);
    Transform t5_reg; t5_reg.position = glm::vec3(17.0f, 9.001f, -5.5f);
    registerRoomInteractions(room5Mesh, t5_reg, interactivePoints);
    Transform t6_reg; t6_reg.position = glm::vec3(17.0f, 9.001f, -16.5f);
    registerRoomInteractions(room6Mesh, t6_reg, interactivePoints);
    Transform t7_reg; t7_reg.position = glm::vec3(4.5f, 9.001f, -4.0f);
    registerRoomInteractions(room7Mesh, t7_reg, interactivePoints);
    Transform t8_reg; t8_reg.position = glm::vec3(-5.0f, 9.001f, -2.5f);
    registerRoomInteractions(room8Mesh, t8_reg, interactivePoints);
    Transform t9_reg; t9_reg.position = glm::vec3(-6.0f, 9.001f, -20.0f);
    registerRoomInteractions(room9Mesh, t9_reg, interactivePoints);
    Transform tLW_reg; tLW_reg.position = glm::vec3(-9.925f, 9.001f, -9.0f);
    registerRoomInteractions(wallLeftMesh, tLW_reg, interactivePoints);
    std::cout << "[SYSTEM] Registered " << interactivePoints.size() << " interactive points." << std::endl;

    // ==========================================
    // [DEBUG_ID: TESTBED_PROCEDURAL_OBJECTS]
    // 5 parametric meshes cycled with N key in testbed mode.
    // ==========================================

    // hemisphere function — dome shape, crown at +Y
    ProceduralMesh testbedHemisphere = generateHemisphere(
        1.8f, 48, 24, glm::vec3(0.75f, 0.85f, 0.95f));

    // surface of revolution function — classic vase/plant-pot profile
    std::vector<glm::vec2> vaseProfile = {
        {0.00f, 0.00f}, // base center
        {0.40f, 0.00f}, // base rim
        {0.50f, 0.15f}, // base shoulder
        {0.30f, 0.50f}, // waist
        {0.45f, 0.90f}, // body
        {0.55f, 1.20f}, // widest body
        {0.48f, 1.50f}, // neck taper
        {0.35f, 1.70f}, // neck
        {0.42f, 1.85f}, // lip flare
        {0.00f, 1.85f}  // lip center
    };
    ProceduralMesh testbedVase = generateSurfaceOfRevolution(
        vaseProfile, 48, glm::vec3(0.65f, 0.40f, 0.25f));

    // spiral staircase function — 16 steps, 20-degree sweep per step
    ProceduralMesh testbedSpiralStair = generateSpiralStair(
        0.4f, 1.2f, 0.22f, 16, 20.0f, glm::vec3(0.70f, 0.58f, 0.42f));

    // superellipsoid function — sofa cushion shape (n=0.3, e=1.0)
    ProceduralMesh testbedCushion = generateSuperellipsoid(
        1.4f, 0.45f, 0.9f, 0.3f, 1.0f, 48, 32, glm::vec3(0.45f, 0.30f, 0.65f));

    // torus function — ring lamp / donut shape
    ProceduralMesh testbedTorus = generateTorus(
        1.0f, 0.32f, 64, 32, glm::vec3(1.0f, 0.88f, 0.40f));

    // ==========================================
    // SOFA (Composite object)
    // ==========================================
    SofaParams sofaDef;
    sofaDef.seats = 3;
    sofaDef.totalWidth = 3.0f;
    sofaDef.cushionColor = glm::vec3(0.3f, 0.2f, 0.5f); // Deep Purple
    sofaDef.frameColor = glm::vec3(0.85f, 0.85f, 0.85f); // White frame
    SofaMesh livingRoomSofa = generateSofaGeometry(sofaDef);

    std::cout << "[TESTBED] Press N to cycle objects. Press T to toggle world/testbed." << std::endl;
    std::cout << "[TESTBED] Showing: Hemisphere Dome" << std::endl;

    // ==========================================
    // GAZEBO SCENE (Round table + 4 chairs + dome on pillars + playing items)
    // Scalable: set tGazebo.scale = glm::vec3(s) to resize the entire scene.
    // ==========================================
    GazeboParams gazeboDef;
    gazeboDef.domeColor = glm::vec3(1.0f, 0.0f, 0.0f); // Red dome
    // (other defaults: warm wood table, dark wood chairs, metal pillars)
    GazeboMesh cardTableGazebo = generateGazeboGeometry(gazeboDef);

    // ==========================================
    // FRACTAL TREES (5 Variations)
    // ==========================================
    PottedTreeParams pineDef;
    pineDef.treeParams.trunkHeight = 1.2f; pineDef.treeParams.trunkRadius = 0.08f;
    pineDef.treeParams.maxDepth = 4; pineDef.treeParams.lengthScale = 0.85f;
    pineDef.treeParams.radiusScale = 0.7f; pineDef.treeParams.spreadAngle = 20.0f;
    pineDef.treeParams.branchesPerNode = 3; pineDef.treeParams.trunkColor = glm::vec3(0.35f, 0.25f, 0.15f);
    pineDef.treeParams.foliageColor = glm::vec3(0.1f, 0.4f, 0.15f); pineDef.treeParams.foliageType = 2;
    PottedTreeMesh pineTree = generatePottedTreeGeometry(pineDef);

    PottedTreeParams oakDef;
    oakDef.treeParams.trunkHeight = 0.8f; oakDef.treeParams.trunkRadius = 0.12f;
    oakDef.treeParams.maxDepth = 4; oakDef.treeParams.lengthScale = 0.75f;
    oakDef.treeParams.radiusScale = 0.6f; oakDef.treeParams.spreadAngle = 45.0f;
    oakDef.treeParams.branchesPerNode = 3; oakDef.treeParams.trunkColor = glm::vec3(0.4f, 0.3f, 0.2f);
    oakDef.treeParams.foliageColor = glm::vec3(0.3f, 0.6f, 0.2f); oakDef.treeParams.foliageType = 1;
    PottedTreeMesh oakTree = generatePottedTreeGeometry(oakDef);

    PottedTreeParams shrubDef;
    shrubDef.potHeight = 0.4f; shrubDef.potTopRadius = 0.4f;
    shrubDef.treeParams.trunkHeight = 0.3f; shrubDef.treeParams.trunkRadius = 0.05f;
    shrubDef.treeParams.maxDepth = 3; shrubDef.treeParams.lengthScale = 0.8f;
    shrubDef.treeParams.radiusScale = 0.6f; shrubDef.treeParams.spreadAngle = 60.0f;
    shrubDef.treeParams.branchesPerNode = 4; shrubDef.treeParams.trunkColor = glm::vec3(0.3f, 0.2f, 0.1f);
    shrubDef.treeParams.foliageColor = glm::vec3(0.4f, 0.7f, 0.3f); shrubDef.treeParams.foliageType = 1;
    PottedTreeMesh shrubTree = generatePottedTreeGeometry(shrubDef);

    PottedTreeParams bonsaiDef;
    bonsaiDef.potHeight = 0.2f; bonsaiDef.potBulge = 0.05f;
    bonsaiDef.treeParams.trunkHeight = 0.4f; bonsaiDef.treeParams.trunkRadius = 0.06f;
    bonsaiDef.treeParams.maxDepth = 5; bonsaiDef.treeParams.lengthScale = 0.7f;
    bonsaiDef.treeParams.radiusScale = 0.7f; bonsaiDef.treeParams.spreadAngle = 55.0f;
    bonsaiDef.treeParams.branchesPerNode = 2; bonsaiDef.treeParams.trunkColor = glm::vec3(0.2f, 0.15f, 0.1f);
    bonsaiDef.treeParams.foliageColor = glm::vec3(0.5f, 0.8f, 0.4f); bonsaiDef.treeParams.foliageType = 2;
    PottedTreeMesh bonsaiTree = generatePottedTreeGeometry(bonsaiDef);

    PottedTreeParams alienDef;
    alienDef.potColor = glm::vec3(0.2f, 0.2f, 0.3f);
    alienDef.treeParams.trunkHeight = 1.0f; alienDef.treeParams.trunkRadius = 0.1f;
    alienDef.treeParams.maxDepth = 4; alienDef.treeParams.lengthScale = 0.9f;
    alienDef.treeParams.radiusScale = 0.5f; alienDef.treeParams.spreadAngle = 25.0f;
    alienDef.treeParams.branchesPerNode = 4; alienDef.treeParams.trunkColor = glm::vec3(0.3f, 0.1f, 0.4f);
    alienDef.treeParams.foliageColor = glm::vec3(0.9f, 0.2f, 0.6f); alienDef.treeParams.foliageType = 1;
    PottedTreeMesh alienTree = generatePottedTreeGeometry(alienDef);

    LamppostParams lampDef;
    LamppostMesh worldLamppost = generateLamppostGeometry(lampDef);

    // ==========================================
    // [DEBUG_ID: EXTERIOR_SPIRAL_STAIR]
    // Spiral stair from ground (Y=0.001) to second-floor door (Y=6.001).
    // Center at X=-15.8, Z=-9.0. Outer radius is 1.8, so the right inner edge
    // of the stair reaches X = -14.0, perfectly docking against the balcony.
    // ==========================================
    ProceduralMesh leftExteriorStair = generateSpiralStair(
        0.5f,           // inner column radius
        1.3f,           // stair width (outer radius = 1.8)
        6.001f / 20.0f, // per-step height (total rise = 6.001 over 20 steps)
        20,             // number of steps
        18.0f,          // degrees per step (20 x 18 = 360 full revolution)
        glm::vec3(0.72f, 0.70f, 0.68f) // warm concrete
    );

    // --- Stair mesh (World) ---
    StairParams worldStairDef;
    worldStairDef.width    = 2.0f;     // Gap depth is 6, so 1/3 is 2.0
    worldStairDef.height   = 6.001f;   // Reaches roof height
    worldStairDef.depth    = 11.0f;    // Distance from X=-4 to X=7
    worldStairDef.numSteps = 14;       // approx 6.0 / 0.43
    worldStairDef.riserColor = glm::vec3(0.7f, 0.7f, 0.7f);
    worldStairDef.treadColor = glm::vec3(0.3f, 0.3f, 0.3f);
    worldStairDef.hasRailings = true;
    StairMesh worldStairMesh = generateStairGeometry(worldStairDef);

    // ==========================================
    // [DEBUG_ID: STAIR_UPPER]
    // Room6 placed at (17, 9.001, -16.5), w=4 -> left wall at X=15, Z-center=-16.5
    // Middle of Room6 left wall (xz): (15, -16.5)
    // Go 7 units -X from left wall: anchor X = 15-7 = 8, Z = -16.5
    // Stair travels in +X (rotation Y=-90), depth=7 -> arrives at X=15 (Room6 left wall)
    // Height: Floor2 Y=6.001 -> Room6 floor Y=9.001 + 0.001 buffer = 9.002 -> h=3.001
    // Width 2.5 in Z, centered at Z=-16.5 -> spans [-17.75, -15.25] (within Room6 Z [-22,-11])
    // numSteps=7: rise=3.001/7=0.429u, run=7/7=1.0u per step
    // ==========================================
    StairParams upperStairDef;
    upperStairDef.width    = 2.5f;
    upperStairDef.height   = 6.001f;   // Floor2 (6.001) -> Room5/6 floor (9.002)
    upperStairDef.depth    = 7.0f;
    upperStairDef.numSteps = 10;
    upperStairDef.riserColor  = glm::vec3(0.65f, 0.50f, 0.35f); // Warm wood tones
    upperStairDef.treadColor  = glm::vec3(0.45f, 0.35f, 0.25f);
    upperStairDef.hasRailings = true;
    upperStairDef.railHeight  = 1.0f;
    upperStairDef.railColor   = glm::vec3(0.80f, 0.75f, 0.70f);
    StairMesh upperStairMesh = generateStairGeometry(upperStairDef);

    // --- Roof Floor (Testbed and World) ---
    // Encompasses total width: X from -10 to 19 (Width=29, Center=4.5)
    // Encompasses total depth: Z from -22 to 0 (Depth=22, Center=-11)
    RoomParams roofDef;
    roofDef.width  = 29.0f;
    roofDef.height = 0.0f;  // Flat plane
    roofDef.depth  = 22.0f;
    for (int i=0; i<6; i++) roofDef.faces[i].isVisible = false;
    roofDef.faces[5].isVisible = true; // Bottom floor = the roof we walk on
    roofDef.faces[5].color = glm::vec3(0.55f, 0.55f, 0.55f); // Ash color

    // Hole accommodates the stairs. 
    // World X bounds = -4 to 7 (Length 11) -> U bounds = 6 to 17
    // World Z bounds = -12 to -9 (Length 3) -> V bounds = 10 to 13
    roofDef.faces[5].holes.push_back({ 6.0f, 10.0f, 11.0f, 3.0f, Hole::EDGE_CONNECT, true, "Roof Stair Hole" });
    RoomMesh roofMesh = generateRoomGeometry(roofDef);

    // --- Roof Floor 2 (Ceiling of Second Floor) ---
    // Same dimensions: W=29, D=22, Center=(4.5, 12.001, -11.0)
    RoomParams roof2Def;
    roof2Def.width  = 29.0f;
    roof2Def.height = 0.0f;  
    roof2Def.depth  = 22.0f;
    for (int i=0; i<6; i++) roof2Def.faces[i].isVisible = false;
    roof2Def.faces[5].isVisible = true; // Bottom floor facing down
    roof2Def.faces[5].color = glm::vec3(1.0f, 1.0f, 1.0f); 

    // upper stair lands at X=15. Right edge of hole is at X=15. Hole width=4 -> X spans 11 to 15.
    // X=11 maps to U = 11 - (X_Center - width/2) = 11 - (4.5 - 14.5) = 11 - (-10) = 21.0
    // upper stair is centered at Z=-16.5. Hole depth 4 -> bounds Z=[-18.5, -14.5].
    // Center_Z is -11.0. Z bounds [-22, 0]. Z=-18.5 maps to V = -18.5 - (-22) = 3.5.
    roof2Def.faces[5].holes.push_back({ 21.0f, 3.5f, 4.0f, 4.0f, Hole::EDGE_CONNECT, true, "Roof2 Stair Hole" });
    RoomMesh roof2Mesh = generateRoomGeometry(roof2Def);

    // ==========================================
    // [DEBUG_ID: ROOF2_RAILINGS]
    // Railings for the edges of roof 2 (perimeter) and the hole in roof 2.
    // ==========================================
    glm::vec3 roofRailCol = glm::vec3(0.85f, 0.80f, 0.75f); // Match other railings
    RoomParams r2pFront; r2pFront.width = 29.0f; r2pFront.height = 1.0f; r2pFront.depth = 0.15f;
    for(int i=0;i<6;i++){r2pFront.faces[i].color=roofRailCol; r2pFront.faces[i].isVisible=true;}
    RoomMesh roof2RailFrontBackMesh = generateRoomGeometry(r2pFront);

    RoomParams r2pLeft; r2pLeft.width = 0.15f; r2pLeft.height = 1.0f; r2pLeft.depth = 22.0f;
    for(int i=0;i<6;i++){r2pLeft.faces[i].color=roofRailCol; r2pLeft.faces[i].isVisible=true;}
    RoomMesh roof2RailLeftRightMesh = generateRoomGeometry(r2pLeft);

    RoomParams r2hFront; r2hFront.width = 4.15f; r2hFront.height = 1.0f; r2hFront.depth = 0.15f;
    for(int i=0;i<6;i++){r2hFront.faces[i].color=roofRailCol; r2hFront.faces[i].isVisible=true;}
    RoomMesh roof2HoleFrontBackMesh = generateRoomGeometry(r2hFront);

    RoomParams r2hLeft; r2hLeft.width = 0.15f; r2hLeft.height = 1.0f; r2hLeft.depth = 4.0f;
    for(int i=0;i<6;i++){r2hLeft.faces[i].color=roofRailCol; r2hLeft.faces[i].isVisible=true;}
    RoomMesh roof2HoleLeftMesh = generateRoomGeometry(r2hLeft);
    std::vector<glm::vec3> treePositions = {
        // Move pots to 12.30 to completely clear any geometry overlaps
        glm::vec3(-8.0f, 12.30f, -2.0f),  // front-left
        glm::vec3(-1.0f, 12.30f, -2.0f),  // front-mid-left
        glm::vec3(6.5f, 12.30f, -2.0f),   // front-mid-right
        glm::vec3(17.0f, 12.30f, -2.0f),  // front-right
        glm::vec3(17.0f, 12.30f, -11.0f), // mid-right
        glm::vec3(-8.0f, 12.30f, -20.0f), // back-left
        glm::vec3(-1.0f, 12.60f, -20.0f), // back-mid-left (hardcoded extra lift)
        glm::vec3(6.5f, 12.30f, -20.0f),  // back-mid-right
        glm::vec3(17.0f, 12.30f, -20.0f), // back-right
        glm::vec3(-8.0f, 12.30f, -6.5f),  // left-mid-front
        glm::vec3(17.0f, 12.30f, -15.5f)  // right-mid-back
    };
    std::vector<PottedTreeMesh*> worldTrees = {
        &pineTree, &oakTree, &shrubTree, &bonsaiTree, &alienTree,
        &oakTree, &bonsaiTree, &shrubTree,
        &alienTree, &pineTree, &oakTree
    };
    
    // Fixed layout for the 12 trees to prevent randomization issues

    // --- Dolna (Swing Set) Setup ---
    SwingParams swingDef;
    SwingMesh testbedSwing = generateSwingGeometry(swingDef);
    float swingAngle = 0.0f;
    float swingVel = 0.0f;
    bool isRidingSwing = false;
    bool pKeyPressed = false;
    bool rKeyPressed = false;
    Transform tSwingFrame;
    // Move to center of Roof 2: X=2.5, Z=-11.0. Elevation = 12.08 + 4.5 = 16.58
    tSwingFrame.position = glm::vec3(2.5f, 16.58f, -11.0f); 

    // --- Mini Golf Setup ---
    MiniGolfGame golfGame;
    std::cout << "[GOLF] Mini-golf game initialized. Use J/L to aim, M to shoot." << std::endl;

    // --- Interior Furniture Testbeds ---
    BedParams bedDef; BedMesh testbedBed = generateBedGeometry(bedDef);
    TableParams tableDef; TableMesh testbedTable = generateTableGeometry(tableDef);
    ChairParams chairDef; ChairMesh testbedChair = generateChairGeometry(chairDef);
    DiningSetParams diningDef; DiningSetMesh testbedDining = generateDiningSetGeometry(diningDef);
    CommodeParams commodeDef; CommodeMesh testbedCommode = generateCommodeGeometry(commodeDef);
    ShowerParams showerDef; ShowerMesh testbedShower = generateShowerGeometry(showerDef);

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        processInput(window);

        glPolygonMode(GL_FRONT_AND_BACK, wireframeMode ? GL_LINE : GL_FILL);

        // --- Pendulum Physics Loop ---
        float angularAccel = - (9.81f / testbedSwing.ropeLength) * sin(glm::radians(swingAngle));
        swingVel += angularAccel * deltaTime * 60.0f; // Scale up slightly to feel fluid
        swingVel *= 0.995f; // Friction/damping
        swingAngle += swingVel * deltaTime;

        // Swing Inputs
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            if (!pKeyPressed) { swingVel += 120.0f; pKeyPressed = true; }
        } else { pKeyPressed = false; }

        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            if (!rKeyPressed) { isRidingSwing = !isRidingSwing; rKeyPressed = true; }
        } else { rKeyPressed = false; }

        // Mini-Golf Inputs
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) {
            golfGame.handleInput(GLFW_KEY_J, GLFW_PRESS);
        }
        
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
            golfGame.handleInput(GLFW_KEY_L, GLFW_PRESS);
        }
        
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
            if (!mKeyPressed) {
                golfGame.handleInput(GLFW_KEY_M, GLFW_PRESS);
                mKeyPressed = true;
            }
        } else { mKeyPressed = false; }

        // Update mini-golf physics
        golfGame.update(deltaTime);

        // Camera Override (Ride the Swing)
        if (isRidingSwing && (currentMode == RENDER_WORLD || testbedObjectIndex == 12)) {
            glm::mat4 mSwing = (currentMode == RENDER_WORLD) ? tSwingFrame.toMatrix() : glm::mat4(1.0f);
            // In testbed mode tbTransform is rotated by time around Y, so ideally we match it:
            if (currentMode == RENDER_TESTBED) {
                mSwing = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime() * 20.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            mSwing = glm::rotate(mSwing, glm::radians(swingAngle), glm::vec3(1.0f, 0.0f, 0.0f));
            glm::vec3 worldSeatPos = glm::vec3(mSwing * glm::vec4(0.0f, -testbedSwing.ropeLength + 1.0f, 0.0f, 1.0f));
            camera.Position = worldSeatPos;
            camera.Pitch = swingAngle * 0.75f; // Look up as it swings forward!
            camera.ProcessMouseMovement(0.0f, 0.0f, true); // apply pitch override internally
        }

        ourShader.ID = usePhong ? phongProgramID : gouraudProgramID;

        glm::vec3 skyColor = isDay ? glm::vec3(0.5f, 0.7f, 0.9f) : glm::vec3(0.01f, 0.01f, 0.03f);
        glClearColor(skyColor.r, skyColor.g, skyColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        auto setupLighting = [&](Shader& shader) {
            shader.use();
            shader.setMat4("projection", projection);
            shader.setMat4("view", view);
            shader.setVec3("viewPos", camera.Position);
            shader.setFloat("materialShininess", 32.0f);
            shader.setBool("diffuseEnabled", diffuseEnabled);
            shader.setBool("specularEnabled", specularEnabled);
            
            // Directional Light: Sun or Moon
            if (dirLightOn) {
                if (isDay) {
                    shader.setVec3("dirLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
                    shader.setVec3("dirLight.ambient", glm::vec3(0.3f, 0.3f, 0.3f));
                    shader.setVec3("dirLight.diffuse", glm::vec3(0.9f, 0.8f, 0.7f)); // Bright sun
                    shader.setVec3("dirLight.specular", glm::vec3(0.5f, 0.5f, 0.5f));
                } else {
                    shader.setVec3("dirLight.direction", glm::vec3(0.2f, -1.0f, 0.3f)); // Opposite direction overhead
                    shader.setVec3("dirLight.ambient", glm::vec3(0.05f, 0.05f, 0.1f));
                    shader.setVec3("dirLight.diffuse", glm::vec3(0.2f, 0.2f, 0.35f)); // Dim moonlight
                    shader.setVec3("dirLight.specular", glm::vec3(0.3f, 0.3f, 0.3f));
                }
            } else {
                shader.setVec3("dirLight.ambient", glm::vec3(0.0f));
                shader.setVec3("dirLight.diffuse", glm::vec3(0.0f));
                shader.setVec3("dirLight.specular", glm::vec3(0.0f));
            }

            int pointIndex = 0;
            auto addPointLight = [&](const glm::vec3& position,
                                     const glm::vec3& ambient,
                                     const glm::vec3& diffuse,
                                     const glm::vec3& specular,
                                     float constant,
                                     float linear,
                                     float quadratic) {
                if (pointIndex >= 22) return;
                std::string base = "pointLights[" + std::to_string(pointIndex) + "].";
                shader.setVec3(base + "position", position);
                shader.setFloat(base + "constant", constant);
                shader.setFloat(base + "linear", linear);
                shader.setFloat(base + "quadratic", quadratic);
                shader.setVec3(base + "ambient", ambient);
                shader.setVec3(base + "diffuse", diffuse);
                shader.setVec3(base + "specular", specular);
                pointIndex++;
            };

            if (pointLightOn) {
                // ========================================================
                // === GAZEBO POINT LIGHTS (8 Ring Maths) ===
                // ========================================================
                float radius = 1.6f;
                for(int i = 0; i < 8; i++) {
                    float angle = i * glm::pi<float>() / 4.0f;
                    float x = -5.0f + radius * cos(angle);
                    float z = -5.0f + radius * sin(angle);
                    float y = 14.8f;
                    addPointLight(glm::vec3(x, y, z), glm::vec3(0.05f), glm::vec3(1.0f, 0.8f, 0.5f), glm::vec3(1.0f, 0.8f, 0.5f), 1.0f, 0.09f, 0.032f);
                }

                // ========================================================
                // === ROOM LIGHTS (R1-R9): back wall, fallback left wall ===
                // ========================================================
                auto addRoomLight = [&](const RoomParams& room, const glm::vec3& center) {
                    float y = center.y + (room.height * 0.5f) - 1.0f;
                    glm::vec3 pos;
                    if (room.faces[1].isVisible) {
                        pos = glm::vec3(center.x, y, center.z - (room.depth * 0.5f) + 0.10f);
                    } else {
                        pos = glm::vec3(center.x - (room.width * 0.5f) + 0.10f, y, center.z);
                    }
                    addPointLight(pos, glm::vec3(0.03f), glm::vec3(0.95f, 0.95f, 0.90f), glm::vec3(0.95f, 0.95f, 0.90f), 1.0f, 0.14f, 0.07f);
                };

                addRoomLight(r1, glm::vec3(-7.0f, 3.0f, -6.0f));
                addRoomLight(r2, glm::vec3(7.0f, 3.0f, -3.0f));
                addRoomLight(r3, glm::vec3(1.5f, 3.0f, -17.0f));
                addRoomLight(r4, glm::vec3(16.0f, 3.0f, -11.0f));
                addRoomLight(r5, glm::vec3(17.0f, 9.001f, -5.5f));
                addRoomLight(r6, glm::vec3(17.0f, 9.001f, -16.5f));
                addRoomLight(r7, glm::vec3(4.5f, 9.001f, -4.0f));
                addRoomLight(r8, glm::vec3(-5.0f, 9.001f, -2.5f));
                addRoomLight(r9, glm::vec3(-6.0f, 9.001f, -20.0f));
            }

            if (spotLightOn) {
                // Roof lamppost lights (all lampposts toggled by key 3)
                addPointLight(glm::vec3(-1.25f, 16.5f, -8.0f), glm::vec3(0.02f), glm::vec3(0.80f, 0.80f, 1.0f), glm::vec3(1.0f), 1.0f, 0.045f, 0.0075f);
                addPointLight(glm::vec3(4.5f, 16.5f, -2.0f),   glm::vec3(0.02f), glm::vec3(0.80f, 0.80f, 1.0f), glm::vec3(1.0f), 1.0f, 0.045f, 0.0075f);
                addPointLight(glm::vec3(4.5f, 16.5f, -20.0f),  glm::vec3(0.02f), glm::vec3(0.80f, 0.80f, 1.0f), glm::vec3(1.0f), 1.0f, 0.045f, 0.0075f);
                addPointLight(glm::vec3(-8.0f, 16.5f, -11.0f), glm::vec3(0.02f), glm::vec3(0.80f, 0.80f, 1.0f), glm::vec3(1.0f), 1.0f, 0.045f, 0.0075f);
                addPointLight(glm::vec3(17.0f, 16.5f, -11.0f), glm::vec3(0.02f), glm::vec3(0.80f, 0.80f, 1.0f), glm::vec3(1.0f), 1.0f, 0.045f, 0.0075f);
            }

            shader.setInt("activePointLights", pointIndex);

            // Spot light disabled; lamppost lighting is now handled through point lights above.
            shader.setBool("useSpotLight", false);
            shader.setVec3("spotLight.ambient", glm::vec3(0.0f));
            shader.setVec3("spotLight.diffuse", glm::vec3(0.0f));
            shader.setVec3("spotLight.specular", glm::vec3(0.0f));
        };

        setupLighting(ourShader);
        setupLighting(roomShader);
        
        ourShader.use();

        if (showAxes) {
            glm::mat4 model = glm::mat4(1.0f);
            ourShader.setMat4("model", model);
            ourShader.setBool("useVertexColor", false); // Axes use uniform color
            glBindVertexArray(axesVAO);
            ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.0f, 0.0f));
            glDrawArrays(GL_LINES, 0, 2);
            ourShader.setVec3("objectColor", glm::vec3(0.0f, 1.0f, 0.0f));
            glDrawArrays(GL_LINES, 2, 2);
            ourShader.setVec3("objectColor", glm::vec3(0.0f, 0.5f, 1.0f));
            glDrawArrays(GL_LINES, 4, 2);
        }

        if (currentMode == RENDER_WORLD) {
            // Setup generic floor ground mesh using our original core Shader
            ourShader.use();
            ourShader.setInt("textureMode", 1);
            ourShader.setFloat("uvScale", 5.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, brickFloorTex);

            BasicMesh groundMesh = { floorVAO, 6, glm::vec3(0.15f, 0.25f, 0.15f) };
            Transform groundTransform;
            groundTransform.position = glm::vec3(0.0f, -0.01f, 0.0f); // Offset slightly down to prevent Z-Fighting
            groundTransform.scale = glm::vec3(35.0f, 1.0f, 35.0f);
            placeObject(ourShader, groundMesh, groundTransform);

            ourShader.setInt("textureMode", 0); // Reset after floor

            // Setup Highway immediately above ground (Z = 10 units in front of origin)
            BasicMesh highwayMesh = { floorVAO, 6, glm::vec3(0.2f, 0.2f, 0.2f) }; // Dark gray asphalt
            Transform highwayTransform;
            // Place it at Z = +10, Y = 0.00 (slightly above ground's -0.01 to prevent fighting)
            highwayTransform.position = glm::vec3(0.0f, 0.00f, 13.0f);
            // Size: spanning 350 in X (35.0 scale on 10-unit plane), width in Z is now 18 units (3x wider)
            highwayTransform.scale = glm::vec3(35.0f, 1.0f, 1.8f);
            placeObject(ourShader, highwayMesh, highwayTransform);

            // Middle lane divider (white line)
            BasicMesh highwayLineMesh = { floorVAO, 6, glm::vec3(1.0f, 1.0f, 1.0f) };
            Transform highwayLineTransform;
            highwayLineTransform.position = glm::vec3(0.0f, 0.002f, 13.0f);
            highwayLineTransform.scale = glm::vec3(35.0f, 1.0f, 0.03f);
            placeObject(ourShader, highwayLineMesh, highwayLineTransform);

            // Switch to the specialized room shader to place multiple generated rooms
            roomShader.use();
            roomShader.setMat4("projection", projection);
            roomShader.setMat4("view", view);

            // Place Room 1
            Transform t1;
            t1.position = glm::vec3(-7.0f, 3.0f, -6.0f);
            placeObject(roomShader, room1Mesh, t1);

            // Place Room 2
            Transform t2;
            t2.position = glm::vec3(7.0f, 3.0f, -3.0f);
            placeObject(roomShader, room2Mesh, t2);

            // Place Room 3
            Transform t3;
            t3.position = glm::vec3(1.5f, 3.0f, -17.0f);
            placeObject(roomShader, room3Mesh, t3);

            // place object — Sofa in Room 3
            // Room3 back wall (Z-) at world Z=-22. Sofa faces +Z. Back at Z=-21.999.
            // Sofa center Z = -21.999 + 0.45 = -21.549. X = room center 1.5. Y = floor 0.
            ourShader.use(); ourShader.setMat4("projection", projection); ourShader.setMat4("view", view);
            ourShader.setInt("textureMode", 3);
            ourShader.setFloat("uvScale", 15.0f);
            Transform tSofaR3;
            tSofaR3.position = glm::vec3(1.5f, 0.0f, -21.549f); // faces +Z (default)
            placeObject(ourShader, livingRoomSofa, tSofaR3);
            ourShader.setInt("textureMode", 0);

            // ===================================
            // INTERIOR FURNITURE (Floor 1 Rooms)
            // ===================================
            // Room 3 Dining Set
            ourShader.setInt("textureMode", 1); 
            ourShader.setFloat("uvScale", 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTex);

            Transform t3Dining; t3Dining.position = glm::vec3(1.5f, 0.0f, -18.5f);
            placeObject(ourShader, testbedDining, t3Dining);
            ourShader.setInt("textureMode", 0);

            ourShader.setInt("textureMode", 1);
            ourShader.setInt("blendMode", 1);
            ourShader.setFloat("uvScale", 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTex);

            // Room 1 Bed, Table, Chair
            Transform t1Bed; t1Bed.position = glm::vec3(-8.0f, 0.0f, -10.0f); t1Bed.rotation.y = -90.0f;
            placeObject(ourShader, testbedBed, t1Bed);
            Transform t1Table; t1Table.position = glm::vec3(-5.0f, 0.0f, -8.0f);
            placeObject(ourShader, testbedTable, t1Table);
            Transform t1Chair; t1Chair.position = glm::vec3(-5.0f, 0.0f, -7.0f); t1Chair.rotation.y = 180.0f;
            placeObject(ourShader, testbedChair, t1Chair);

            // Room 2 Bed, Table, Chair
            Transform t2Bed; t2Bed.position = glm::vec3(10.0f, 0.0f, -4.0f);
            placeObject(ourShader, testbedBed, t2Bed);
            Transform t2Table; t2Table.position = glm::vec3(4.0f, 0.0f, -4.0f);
            placeObject(ourShader, testbedTable, t2Table);
            Transform t2Chair; t2Chair.position = glm::vec3(4.0f, 0.0f, -3.0f); t2Chair.rotation.y = 180.0f;
            placeObject(ourShader, testbedChair, t2Chair);

            // Room 4 Bed, Table, Chair
            Transform t4Bed; t4Bed.position = glm::vec3(17.0f, 0.0f, -13.0f); t4Bed.rotation.y = -90.0f;
            placeObject(ourShader, testbedBed, t4Bed);
            Transform t4Table; t4Table.position = glm::vec3(17.0f, 0.0f, -4.0f);
            placeObject(ourShader, testbedTable, t4Table);
            Transform t4Chair; t4Chair.position = glm::vec3(17.0f, 0.0f, -3.0f); t4Chair.rotation.y = 180.0f;
            placeObject(ourShader, testbedChair, t4Chair);

            roomShader.use(); roomShader.setMat4("projection", projection); roomShader.setMat4("view", view);
            Transform t4;
            t4.position = glm::vec3(16.0f, 3.0f, -11.0f);
            placeObject(roomShader, room4Mesh, t4);

            // Place Room 5
            Transform t5;
            t5.position = glm::vec3(17.0f, 9.001f, -5.5f);
            placeObject(roomShader, room5Mesh, t5);

            // Place Room 6
            Transform t6;
            t6.position = glm::vec3(17.0f, 9.001f, -16.5f);
            placeObject(roomShader, room6Mesh, t6);

            // Place Room 7 (large, front side of second floor)
            // X[0,9], Z[-8,0], center (4.5, 9.001, -4.0)
            Transform t7;
            t7.position = glm::vec3(4.5f, 9.001f, -4.0f);
            placeObject(roomShader, room7Mesh, t7);

            // Place Room 8 (bathroom, front side of second floor)
            // X[-7,-3], Z[-5,0], center (-5.0, 9.001, -2.5)
            Transform t8;
            t8.position = glm::vec3(-5.0f, 9.001f, -2.5f);
            placeObject(roomShader, room8Mesh, t8);

            // Place Room 9 (small back-edge room)
            // X[-10,-2], Z[-22,-18], center (-6.0, 9.001, -20.0)
            Transform t9;
            t9.position = glm::vec3(-6.0f, 9.001f, -20.0f);
            placeObject(roomShader, room9Mesh, t9);

            // ===================================
            // INTERIOR FURNITURE (Floor 2 Rooms)
            // ===================================
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            // Room 8 Commode (Solid White)
            ourShader.setInt("textureMode", 0);
            ourShader.setVec3("objectColor", glm::vec3(0.9f));
            ourShader.setBool("useVertexColor", false);

            Transform t8Commode; t8Commode.position = glm::vec3(-5.0f, 6.001f, -4.2f);
            placeObject(ourShader, testbedCommode, t8Commode);

            // Room 9 Shower & Commode
            Transform t9Shower; t9Shower.position = glm::vec3(-8.5f, 6.001f, -21.0f);
            placeObject(ourShader, testbedShower, t9Shower);
            Transform t9Commode; t9Commode.position = glm::vec3(-3.5f, 6.001f, -21.5f);
            placeObject(ourShader, testbedCommode, t9Commode);
            
            // Restore default mode for rest
            ourShader.setBool("useVertexColor", true);
            
            ourShader.setInt("textureMode", 1); // Wood for beds/tables
            ourShader.setInt("blendMode", 1);
            ourShader.setFloat("uvScale", 1.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTex);

            // Room 5 Bed, Table, Chair
            Transform t5Bed; t5Bed.position = glm::vec3(17.0f, 6.001f, -8.5f);
            placeObject(ourShader, testbedBed, t5Bed);
            Transform t5Table; t5Table.position = glm::vec3(17.5f, 6.001f, -2.5f);
            placeObject(ourShader, testbedTable, t5Table);
            Transform t5Chair; t5Chair.position = glm::vec3(17.5f, 6.001f, -1.5f);
            t5Chair.rotation.y = 180.0f; // face table
            placeObject(ourShader, testbedChair, t5Chair);

            // Room 6 Bed, Table, Chair
            Transform t6Bed; t6Bed.position = glm::vec3(17.0f, 6.001f, -20.0f);
            placeObject(ourShader, testbedBed, t6Bed);
            Transform t6Table; t6Table.position = glm::vec3(17.5f, 6.001f, -15.0f);
            placeObject(ourShader, testbedTable, t6Table);
            Transform t6Chair; t6Chair.position = glm::vec3(17.5f, 6.001f, -14.0f);
            t6Chair.rotation.y = 180.0f; // face table
            placeObject(ourShader, testbedChair, t6Chair);

            // Room 7 Bed, Chairs, Table
            Transform t7Bed; t7Bed.position = glm::vec3(2.0f, 6.001f, -6.5f);
            t7Bed.rotation.y = -90.0f; // sideways
            placeObject(ourShader, testbedBed, t7Bed);
            Transform t7Table; t7Table.position = glm::vec3(7.0f, 6.001f, -3.0f);
            placeObject(ourShader, testbedTable, t7Table);
            Transform t7Chair1; t7Chair1.position = glm::vec3(7.0f, 6.001f, -1.8f);
            t7Chair1.rotation.y = 180.0f;
            placeObject(ourShader, testbedChair, t7Chair1);
            Transform t7Chair2; t7Chair2.position = glm::vec3(7.0f, 6.001f, -4.2f);
            placeObject(ourShader, testbedChair, t7Chair2);

            roomShader.use();
            roomShader.setMat4("projection", projection);
            roomShader.setMat4("view", view);

            // Place stair hole railings (sit on second floor, center Y=6.501)
            // Front railing: Z=-9 hole edge, floor-side center Z=-8.925
            Transform tRailFront;
            tRailFront.position = glm::vec3(1.5f, 6.501f, -8.925f);
            placeObject(roomShader, railFrontMesh, tRailFront);
            // Back railing: Z=-12 hole edge, floor-side center Z=-12.075
            Transform tRailBack;
            tRailBack.position = glm::vec3(1.5f, 6.501f, -12.075f);
            placeObject(roomShader, railBackMesh, tRailBack);
           

            // Place perimeter walls (center Y=9.001, bottom sits at floor Y=6.001)
            roomShader.use();
            roomShader.setInt("textureMode", 1);
            roomShader.setFloat("uvScale", 3.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, brickExtTex);

            // Front Seg A: X[-10,-7], center X=-8.5, Z=-0.075
            Transform tWallFrontA;
            tWallFrontA.position = glm::vec3(-8.5f, 9.001f, -0.075f);
            placeObject(roomShader, wallFrontAMesh, tWallFrontA);
            // Front Seg B: X[-3,0], center X=-1.5, Z=-0.075 (reuse Seg A mesh)
            Transform tWallFrontB;
            tWallFrontB.position = glm::vec3(-1.5f, 9.001f, -0.075f);
            placeObject(roomShader, wallFrontAMesh, tWallFrontB);
            // Front Seg C: X[9,15], center X=12.0, Z=-0.075
            Transform tWallFrontC;
            tWallFrontC.position = glm::vec3(12.0f, 9.001f, -0.075f);
            placeObject(roomShader, wallFrontCMesh, tWallFrontC);
            // Left wall: X[-10,-9.85], Z[-18,0], center (-9.925, 9.001, -9.0)
            Transform tWallLeft;
            tWallLeft.position = glm::vec3(-9.925f, 9.001f, -9.0f);
            placeObject(roomShader, wallLeftMesh, tWallLeft);

            // place object — exterior spiral stair on left side of building
            // Center at -12.0, Z=-11.0. The pillar (r=0.5) just touches the
            // back edge (Z=-10.5) of the balcony.
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);
            Transform tExtStair;
            tExtStair.position = glm::vec3(-12.0f, 0.001f, -10.7f);
            placeObject(ourShader, leftExteriorStair, tExtStair);
            roomShader.use();
            roomShader.setMat4("projection", projection);
            roomShader.setMat4("view", view);

            // place object — left wall balcony
            // Floor slab: top face at Y=6.001, center at (-12, 5.926, -9)
            // Balcony spans X[-10,-14], Z[-7.5,-10.5].
            // Spiral stair column (center -12.2, 0, -9) is underneath.
            Transform tBalconyFloor;
            tBalconyFloor.position = glm::vec3(-12.0f, 5.926f, -9.0f);
            placeObject(roomShader, balconyFloorMesh, tBalconyFloor);
            // Back railing (Z- edge, Z=-10.5) removed for spiral stair connection!
            // Right railing (Z+ edge, Z=-7.5)
            Transform tBalcRailR;
            tBalcRailR.position = glm::vec3(-12.0f, 6.451f, -7.475f);
            placeObject(roomShader, balconyRailSideMesh, tBalcRailR);
            // Front railing (X- far edge, X=-14.025)
            Transform tBalcRailFront;
            tBalcRailFront.position = glm::vec3(-14.025f, 6.451f, -9.0f);
            placeObject(roomShader, balconyRailFrontMesh, tBalcRailFront);
            // Back wall: X[-2,15], center (6.5, 9.001, -21.925)
            Transform tWallBack;
            tWallBack.position = glm::vec3(6.5f, 9.001f, -21.925f);
            placeObject(roomShader, wallBackMesh, tWallBack);

            roomShader.setInt("textureMode", 0);

            // Place Roof Floor
            roomShader.setInt("textureMode", 0); // Solid color
            
            Transform tRoof;
            tRoof.position = glm::vec3(4.5f, 6.001f, -11.0f);
            placeObject(roomShader, roofMesh, tRoof);

            // Place Roof 2
            Transform tRoof2;
            tRoof2.position = glm::vec3(4.5f, 12.003f, -11.0f);
            placeObject(roomShader, roof2Mesh, tRoof2);

            // Place Gazebo on Roof 2
            // 5 units back from front edge (Z=0 -> Z=-5)
            // 5 units right of left edge (X=-10 -> X=-5)
            // Height sits exactly on roof 2 (Y=12.003)
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            ourShader.setInt("textureMode", 1); // Wood for Gazebo, Trees, Swing
            ourShader.setInt("textureMode", 4);
            ourShader.setBool("useVertexColor", false);
            
            // ========================================================
            // === SUN AND MOON POSITION AND SIZE ===
            // ========================================================
            if (isDay && dirLightOn) {
                // Sun Settings
                ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.9f, 0.5f));
                Transform tSun;
                tSun.position = glm::vec3(0.2f, 1.0f, 0.3f) * 60.0f; 
                tSun.scale = glm::vec3(2.0f); // Make sun much smaller (was 4.0)
                placeObject(ourShader, celestialSphere, tSun);
            } else if (!isDay && dirLightOn) {
                // Moon Settings
                ourShader.setVec3("objectColor", glm::vec3(0.9f, 0.9f, 0.95f));
                Transform tMoon;
                tMoon.position = glm::vec3(-0.2f, 1.0f, -0.3f) * 60.0f;
                tMoon.scale = glm::vec3(1.5f); // Make moon much smaller (was 3.0)
                placeObject(ourShader, celestialSphere, tMoon);
            }

            // ========================================================
            // === GAZEBO POINT LIGHT MODELS (8 Rings) ===
            // ========================================================
            if (pointLightOn) {
                ourShader.setVec3("objectColor", glm::vec3(1.0f, 0.8f, 0.5f)); 
                for(int i = 0; i < 8; i++) {
                    float angle = i * glm::pi<float>() / 4.0f;
                    Transform tLamp;
                    // Lowered Y to ~14.2f to touch bottom edge of dome. Radius 1.6f.
                    tLamp.position = glm::vec3(-5.0f + 1.6f * cos(angle), 14.2f, -5.0f + 1.6f * sin(angle));
                    tLamp.scale = glm::vec3(0.05f); // Tiny point lights!
                    placeObject(ourShader, celestialSphere, tLamp);
                }
            }

            // ========================================================
            // ========================================================
            // === SWING SPOT LIGHT MODEL ===
            // ========================================================
            // The physical lamppost mesh is always drawn!
            // Its light is mathematically detached and controlled by 'useSpotLight' uniform.
            Transform tPole;
            tPole.position = glm::vec3(-1.25f, 12.003f, -8.0f); // Base sits exactly on roof.
            placeObject(ourShader, worldLamppost, tPole);

            // Additional Roof2 lampposts: middle of each edge, 2 units inward
            Transform tPoleFront;
            tPoleFront.position = glm::vec3(4.5f, 12.003f, -2.0f);
            placeObject(ourShader, worldLamppost, tPoleFront);

            Transform tPoleBack;
            tPoleBack.position = glm::vec3(4.5f, 12.003f, -20.0f);
            placeObject(ourShader, worldLamppost, tPoleBack);

            Transform tPoleLeft;
            tPoleLeft.position = glm::vec3(-8.0f, 12.003f, -11.0f);
            placeObject(ourShader, worldLamppost, tPoleLeft);

            Transform tPoleRight;
            tPoleRight.position = glm::vec3(17.0f, 12.003f, -11.0f);
            placeObject(ourShader, worldLamppost, tPoleRight);

            ourShader.setBool("useVertexColor", true);
            ourShader.setInt("textureMode", 1); 
            ourShader.setInt("blendMode", 1);
            ourShader.setFloat("uvScale", 2.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTex);

            Transform tGazebo;
            tGazebo.position = glm::vec3(-5.0f, 12.003f, -5.0f);
            placeObject(ourShader, cardTableGazebo, tGazebo);
            
            // Place Randomized Potted Trees on Roof 2
            for (size_t i = 0; i < worldTrees.size(); i++) {
                Transform tTree;
                tTree.position = treePositions[i];
                placeObject(ourShader, *worldTrees[i], tTree);
            }
            
            // Place Swing prominently in the middle of Roof 2
            placeObject(ourShader, testbedSwing, tSwingFrame, swingAngle);

            roomShader.use();
            roomShader.setMat4("projection", projection);
            roomShader.setMat4("view", view);

            // place object - Roof 2 Perimeter Railings
            Transform tR2pFront; tR2pFront.position = glm::vec3(4.5f, 12.503f, -0.075f);
            placeObject(roomShader, roof2RailFrontBackMesh, tR2pFront);
            Transform tR2pBack; tR2pBack.position = glm::vec3(4.5f, 12.503f, -21.925f);
            placeObject(roomShader, roof2RailFrontBackMesh, tR2pBack);
            Transform tR2pLeft; tR2pLeft.position = glm::vec3(-9.925f, 12.503f, -11.0f);
            placeObject(roomShader, roof2RailLeftRightMesh, tR2pLeft);
            Transform tR2pRight; tR2pRight.position = glm::vec3(18.925f, 12.503f, -11.0f);
            placeObject(roomShader, roof2RailLeftRightMesh, tR2pRight);

            // place object - Roof 2 Hole Railings
            Transform tR2hFront; tR2hFront.position = glm::vec3(12.925f, 12.503f, -14.425f);
            placeObject(roomShader, roof2HoleFrontBackMesh, tR2hFront);
            Transform tR2hBack; tR2hBack.position = glm::vec3(12.925f, 12.503f, -18.575f);
            placeObject(roomShader, roof2HoleFrontBackMesh, tR2hBack);
            Transform tR2hLeft; tR2hLeft.position = glm::vec3(10.925f, 12.503f, -16.5f);
            placeObject(roomShader, roof2HoleLeftMesh, tR2hLeft);

            // Place World Stair
            // To render with basic solid colors, switch back to ourShader
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);
            Transform tStair;
            tStair.position = glm::vec3(7.0f, 0.0f, -10.0f); 
            tStair.rotation = glm::vec3(0.0f, 90.0f, 0.0f); // Rotated +90 so -Z becomes -X (right to left)
            placeObject(ourShader, worldStairMesh, tStair);

            // place object -- Upper stair
            // Starts at (8, 6.001, -16.5): 7 units -X from Room6 left wall
            // Rotation Y=-90 makes depth travel in +X -> arrives at Room6 left wall (X=15)
            Transform tUpperStair;
            tUpperStair.position = glm::vec3(8.0f, 6.001f, -16.5f);
            tUpperStair.rotation = glm::vec3(0.0f, -90.0f, 0.0f);
            placeObject(ourShader, upperStairMesh, tUpperStair);

        } else if (currentMode == RENDER_TESTBED) {
            // === TESTBED: One procedural object at a time, press N to cycle ===
            ourShader.use();
            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            ourShader.setInt("textureMode", 1); // Default testbed to wood
            ourShader.setInt("blendMode", 1);
            ourShader.setFloat("uvScale", 2.0f);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, woodTex);

            if (testbedObjectIndex == 0 || testbedObjectIndex == 3 || testbedObjectIndex == 5) {
                // Hemisphere, Cushion, Sofa -> Noise
                ourShader.setInt("textureMode", 3);
                ourShader.setFloat("uvScale", 15.0f);
                ourShader.setInt("blendMode", 0);
            } else if (testbedObjectIndex == 1 || testbedObjectIndex == 17 || testbedObjectIndex == 18) {
                // Vase, Commode, Shower -> Solid
                ourShader.setInt("textureMode", 0);
            }

            // Slowly rotate the displayed object around Y for easy inspection
            Transform tbTransform;
            tbTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
            tbTransform.rotation = glm::vec3(0.0f, (float)glfwGetTime() * 20.0f, 0.0f);

            // place object
            switch (testbedObjectIndex) {
                case 0: placeObject(ourShader, testbedHemisphere,  tbTransform); break;
                case 1: placeObject(ourShader, testbedVase,        tbTransform); break;
                case 2: placeObject(ourShader, testbedSpiralStair, tbTransform); break;
                case 3: placeObject(ourShader, testbedCushion,     tbTransform); break;
                case 4: placeObject(ourShader, testbedTorus,       tbTransform); break;
                case 5: placeObject(ourShader, livingRoomSofa,     tbTransform); break;
                case 6: placeObject(ourShader, cardTableGazebo,    tbTransform); break;
                case 7: placeObject(ourShader, pineTree,           tbTransform); break;
                case 8: placeObject(ourShader, oakTree,            tbTransform); break;
                case 9: placeObject(ourShader, shrubTree,          tbTransform); break;
                case 10: placeObject(ourShader, bonsaiTree,        tbTransform); break;
                case 11: placeObject(ourShader, alienTree,         tbTransform); break;
                case 12: placeObject(ourShader, testbedSwing,      tbTransform, swingAngle); break;
                case 13: placeObject(ourShader, testbedBed,        tbTransform); break;
                case 14: placeObject(ourShader, testbedTable,      tbTransform); break;
                case 15: placeObject(ourShader, testbedChair,      tbTransform); break;
                case 16: placeObject(ourShader, testbedDining,     tbTransform); break;
                case 17: placeObject(ourShader, testbedCommode,    tbTransform); break;
                case 18: placeObject(ourShader, testbedShower,     tbTransform); break;
                case 19: golfGame.render(ourShader, projection, view, glm::vec3(0.0f, 0.0f, 0.0f)); break;
            }
        }

        // --- DRAW STARS AT NIGHT ---
        if (!isDay) {
            ourShader.use();
            glm::mat4 starProjection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH / (float)HEIGHT, 0.1f, 1000.0f);
            ourShader.setMat4("projection", starProjection);
            glm::mat4 skyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
            ourShader.setMat4("view", skyView);
            ourShader.setMat4("model", glm::mat4(1.0f));
            ourShader.setInt("textureMode", 4);
            ourShader.setBool("useVertexColor", false);
            ourShader.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
            
            glBindVertexArray(starVAO);
            glPointSize(2.0f);
            glDrawArrays(GL_POINTS, 0, (GLsizei)stars.size());
            glBindVertexArray(0);
            
            ourShader.setBool("useVertexColor", true);
            ourShader.setInt("textureMode", 0);
        }

        // --- DRAW SKYBOX LAST ---
        glDepthFunc(GL_LEQUAL); // render skybox behind everything
        skyboxShader.use();
        glm::vec3 darkSky = isDay ? skyColor : glm::vec3(0.01f, 0.01f, 0.02f);
        skyboxShader.setVec3("objectColor", darkSky); 
        
        // Remove translation from the view matrix so skybox is static
        glm::mat4 skyView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", skyView);
        skyboxShader.setMat4("projection", projection);

        glBindVertexArray(skyboxVAO);
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        if (screenshotRequested) {
            saveScreenshot(window, buildScreenshotFilename());
            screenshotRequested = false;
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}