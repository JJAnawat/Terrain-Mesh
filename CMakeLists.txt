cmake_minimum_required(VERSION 3.20)
project(TerrainMesh)

set(CMAKE_CXX_STANDARD 17)

# vcpkg toolchain (set via cmake preset or command line)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

# stb_image is header-only, just drop it in src/
add_executable(TerrainMesh
    src/main.cpp
    src/Heightmap.cpp
    src/DCEL.cpp
    src/Delaunay.cpp
    src/Ruppert.cpp
    src/Renderer.cpp
)

target_link_libraries(TerrainMesh PRIVATE
    glfw
    glm::glm
    imgui::imgui
    glad::glad
)

# Copy shaders next to the executable
file(COPY shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets  DESTINATION ${CMAKE_BINARY_DIR})