#include <iostream>
#include <vector>
#include <random>
#include <cmath>

#include "Camera.h"
#include "Renderer.h"
#include "Geometry.h"
#include "DCEL.h"
#include "Delaunay.h"

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// Height map loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// std::vector<glm::vec3> vertices = {
//     // Base quad (y = 0, on XY plane)
//     glm::vec3(-0.5f, 0.0f, -0.5f),  // 0: back-left
//     glm::vec3( 0.5f, 0.0f, -0.5f),  // 1: back-right
//     glm::vec3( 0.5f, 0.0f,  0.5f),  // 2: front-right
//     glm::vec3(-0.5f, 0.0f,  0.5f),  // 3: front-left
    
//     // Apex (pointing up along +Y)
//     glm::vec3( 0.0f,  0.88f,  0.0f)  // 4: apex
// };
 
// std::vector<unsigned int> indices = {
//     // Bottom face (z = -0.5)
//     0, 1, 2,
//     0, 2, 3,
    
//     // Front face (y = -0.5)
//     0, 4, 1,
    
//     // Right face (x = 0.5)
//     1, 4, 2,
    
//     // Back face (y = 0.5)
//     2, 4, 3,
    
//     // Left face (x = -0.5)
//     3, 4, 0
// };

Camera* g_camera = nullptr;
Renderer* g_renderer = nullptr;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if(g_camera)
        g_camera->onMouseButton(button, action);
}

void mouseMoveCallback(GLFWwindow* window, double x, double y) {
    if(g_camera)
        g_camera->onMouseMove(x, y);
}

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset) {
    if(g_camera)
        g_camera->onScroll(yOffset);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS){
        if(g_camera)
            g_camera->printState();
    }
}

// Delaunay test data
std::vector<glm::vec3> generateTestTerrain(int gridWidth, int gridDepth) {
    std::vector<glm::vec3> points;
    
    float widthExtent = 4.0f;
    float depthExtent = 4.0f;

    for (int x = 0; x < gridWidth; ++x) {
        for (int z = 0; z < gridDepth; ++z) {
            float px = (x / (float)(gridWidth - 1) - 0.5f) * widthExtent;
            float pz = (z / (float)(gridDepth - 1) - 0.5f) * depthExtent;
            
            float py = 0.5f * std::sin(px * 1.5f) * std::cos(pz * 1.5f)        // Base rolling hills
                     + 0.2f * std::sin(px * 3.5f + 1.0f) * std::cos(pz * 3.5f) // Mid-frequency details
                     + 0.1f * std::sin(px * 8.0f);                             // High-frequency ridges
            
            points.push_back(glm::vec3(px, py, pz));
        }
    }
    return points;
}

int main() {
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return -1; }

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Terrain Mesh", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    Camera camera;
    Renderer renderer;
    g_camera = &camera;
    g_renderer = &renderer;

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mouseMoveCallback);
    glfwSetScrollCallback(window, scrollCallback);

    // Testing Delaunay
    std::vector<glm::vec3> testPoints  = generateTestTerrain(30, 30);
    
    DCEL dcel;
    Delaunay triangulator(dcel);
    triangulator.build(testPoints);

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    for(const auto& v: dcel.vertices)
        vertices.push_back(v.pos);

    auto triangles = dcel.get_triangles();
    for(const auto& tri: triangles){
        if(tri[0] < 3 || tri[1] < 3 || tri[2] < 3)
            continue;
        
        indices.push_back(tri[0]);
        indices.push_back(tri[1]);
        indices.push_back(tri[2]);
    }

    std::vector<glm::vec3> normals = computeVertexNormals(vertices, indices);
    std::vector<glm::vec3> colors = generateWhiteColors(vertices.size());

    renderer.uploadMesh(vertices, normals, colors, indices);

    glm::mat4 model = glm::mat4(1.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        float aspect = (float)width / height;
        
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjMatrix(aspect);
        
        renderer.setViewMatrix(view);
        renderer.setModelMatrix(model);
        renderer.setProjMatrix(proj);
        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        processInput(window);
    }
    glfwTerminate();
    return 0;
}