#include <iostream>
#include <vector>
#include <random>
#include <cmath>

#include "Camera.h"
#include "Renderer.h"
#include "Heightmap.h"
#include "Geometry.h"
#include "DCEL.h"
#include "Delaunay.h"

#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

// Height map loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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

    Heightmap heightmap;

    if(!heightmap.load("assets/test_heightmap.png"))
        return -1;

    // std::vector<glm::vec3> points = heightmap.generateTestTerrain(30, 30);
    std::vector<glm::vec3> points = heightmap.importanceSample(500);

    DCEL dcel;
    Delaunay triangulator(dcel);
    triangulator.build(points);

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
        renderer.setViewPos(camera.Position);
        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
        
        processInput(window);
    }
    glfwTerminate();
    return 0;
}