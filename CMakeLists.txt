cmake_minimum_required(VERSION 3.20)
project(TerrainMesh)

set(CMAKE_CXX_STANDARD 17)

# vcpkg toolchain (set via cmake preset or command line)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

include_directories(include include/algorithms include/data_structures include/rendering third_party)

# stb_image is header-only, just drop it in src/
add_executable(TerrainMesh
    src/main.cpp
    src/data_structures/Heightmap.cpp
    src/data_structures/DCEL.cpp
    src/algorithms/Delaunay.cpp
    src/rendering/Renderer.cpp
    src/rendering/Camera.cpp
    src/algorithms/TerrainMesher.cpp
)

target_link_libraries(TerrainMesh PRIVATE
    glfw
    glm::glm
    imgui::imgui
    glad::glad
)

# Post-build command to copy assets directory to the output directory
add_custom_command(TARGET TerrainMesh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/shaders
        $<TARGET_FILE_DIR:TerrainMesh>/shaders
)

add_custom_command(TARGET TerrainMesh POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:TerrainMesh>/assets
)