#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <fstream>

#include "Camera.h"
#include "Renderer.h"
#include "Heightmap.h"
#include "TerrainMesher.h"
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

void exportMeshToCSV(const std::vector<glm::vec3>& verts, const std::vector<unsigned int>& indices) {
    // 1. Export Vertices
    std::ofstream vFile("C:/JJ/Year3/Comp-Geo/terrain-mesh/output/vertices.csv");
    vFile << "x,y,z\n";
    for (const auto& v : verts) {
        vFile << v.x << "," << v.y << "," << v.z << "\n";
    }
    vFile.close();

    // 2. Export Triangles (Indices)
    std::ofstream tFile("C:/JJ/Year3/Comp-Geo/terrain-mesh/output/triangles.csv");
    tFile << "v0,v1,v2\n";
    for (size_t i = 0; i < indices.size(); i += 3) {
        tFile << indices[i] << "," << indices[i+1] << "," << indices[i+2] << "\n";
    }
    tFile.close();
    
    std::cout << "Mesh exported to vertices.csv and triangles.csv successfully!\n";
}

bool wireframeMode = false;
std::mutex meshMutex;
bool meshReadyToUpload = false;

std::vector<glm::vec3> async_vertices;
std::vector<glm::vec3> async_normals;
std::vector<glm::vec3> async_colors;
std::vector<unsigned int> async_indices;

std::atomic<bool> isRunning{true};
std::atomic<bool> rebuildRequested{false};
glm::vec3 requestedCameraPos;
std::atomic<float> requestedZoom;

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Only toggle on the exact moment the key is pressed down
    if (key == GLFW_KEY_F && action == GLFW_PRESS) 
        wireframeMode = !wireframeMode;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
        if(g_camera)
            g_camera->printState();
    if (key == GLFW_KEY_C && action == GLFW_PRESS){
        std::lock_guard<std::mutex> lock(meshMutex);
        exportMeshToCSV(async_vertices, async_indices);
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
    glfwSetKeyCallback(window, keyCallback);

    Heightmap heightmap(4.0f, 4.0f, 1.0f);
    if(!heightmap.load("assets/test-terrain.png"))
        return -1;  

    DCEL dcel;
    Delaunay triangulator(dcel);
    TerrainMesher mesher(dcel, triangulator, heightmap);

    std::vector<glm::vec3> baseCorners = heightmap.getFourCorners();

    std::thread lodThread([&](){
        while(isRunning){
            if(rebuildRequested){
                glm::vec3 camPos = requestedCameraPos;
                float currentFov = requestedZoom;
                rebuildRequested = false;

                auto start_time = std::chrono::high_resolution_clock::now();
                mesher.rebuild_mesh(camPos, currentFov, 1000, 0, baseCorners); // Only algo 0-4 is working right now
                auto end_time = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> time_taken = end_time - start_time;

                std::cout << "Mesh generated in: " << time_taken.count() << " ms\n";

                std::vector<glm::vec3> local_vertices;
                std::vector<unsigned int> local_indices;
                
                for(const auto& v: dcel.vertices)
                    local_vertices.push_back(v.pos);
                
                float halfWidth = heightmap.widthExtent * 0.5f;
                float halfDepth = heightmap.depthExtent * 0.5f;
                
                float epsilon = 0.1f; 

                auto is_super_vertex = [&](const glm::vec3& p) {
                    return (p.x < -halfWidth - epsilon || p.x > halfWidth + epsilon ||
                            p.z < -halfDepth - epsilon || p.z > halfDepth + epsilon);
                };

                auto triangles = dcel.get_triangles();
                for(const auto& tri : triangles) {
                    glm::vec3 p0 = dcel.vertices[tri[0]].pos;
                    glm::vec3 p1 = dcel.vertices[tri[1]].pos;
                    glm::vec3 p2 = dcel.vertices[tri[2]].pos;

                    if(is_super_vertex(p0) || is_super_vertex(p1) || is_super_vertex(p2)) {
                        continue; 
                    }
                    
                    local_indices.push_back(tri[0]);
                    local_indices.push_back(tri[1]);
                    local_indices.push_back(tri[2]);
                }

                std::vector<glm::vec3> local_normals = computeVertexNormals(local_vertices, local_indices);
                std::vector<glm::vec3> local_colors = generateWhiteColors(local_vertices);

                {
                    std::lock_guard<std::mutex> lock(meshMutex);
                    async_vertices = std::move(local_vertices);
                    async_normals = std::move(local_normals);
                    async_colors = std::move(local_colors);
                    async_indices = std::move(local_indices);
                    meshReadyToUpload = true;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    glm::mat4 model = glm::mat4(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float lastZoom = camera.Zoom;
    float zoomFovThreshold = 1000.0f;
    rebuildRequested = true;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Draw as wireframe
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Draw as solid colored triangles
        }
        
        int width, height;
        glfwGetWindowSize(window, &width, &height);

        if(width ==0 || height == 0){
            glfwWaitEvents();
            continue;
        }

        float aspect = (float)width / height;

        // Request a mesh
        if(std::abs(camera.Zoom - lastZoom) > zoomFovThreshold && !rebuildRequested){
            requestedCameraPos = camera.Position;
            requestedZoom = camera.Zoom;
            rebuildRequested = true;
            lastZoom = camera.Zoom;
        }

        // Render
        {
            std::lock_guard<std::mutex> lock(meshMutex);
            if(meshReadyToUpload){
                renderer.uploadMesh(async_vertices, async_normals, async_colors, async_indices);
                meshReadyToUpload = false;
            }
        }
        
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 proj = camera.getProjMatrix(aspect);
        
        renderer.setViewMatrix(view);
        renderer.setModelMatrix(model);
        renderer.setProjMatrix(proj);
        renderer.setViewPos(camera.Position);
        renderer.draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    isRunning = false;
    lodThread.join();
    glfwTerminate();
    return 0;
}