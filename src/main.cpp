#include <iostream>
#include "Camera.h"
#include "Renderer.h"
#include <GLFW/glfw3.h>

// Height map loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

float vertices[] = {
    // Base quad (z = -0.5)
    -0.5f, -0.5f, -0.5f,  // 0: bottom-left
     0.5f, -0.5f, -0.5f,  // 1: bottom-right
     0.5f,  0.5f, -0.5f,  // 2: top-right
    -0.5f,  0.5f, -0.5f,  // 3: top-left
    
    // Apex (top point, centered)
     0.0f,  0.0f,  0.5f   // 4: apex
};
 
unsigned int indices[] = {
    // Bottom face (z = -0.5)
    0, 1, 2,
    0, 2, 3,
    
    // Front face (y = -0.5)
    0, 4, 1,
    
    // Right face (x = 0.5)
    1, 4, 2,
    
    // Back face (y = 0.5)
    2, 4, 3,
    
    // Left face (x = -0.5)
    3, 4, 0
};

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
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS){
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

    renderer.uploadMesh(vertices, 5, indices, 18);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1280.0f/720.0f, 0.01f, 1000.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f), // Eye position
        glm::vec3(0.0f, 0.0f, 0.0f), // Look at
        glm::vec3(0.0f, 1.0f, 0.0f) // Up
    );
    glm::mat4 model = glm::mat4(1.0f);

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