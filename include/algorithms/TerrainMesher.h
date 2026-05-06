#pragma once
#include "data_structures/DCEL.h"
#include "algorithms/Delaunay.h"
#include "data_structures/Heightmap.h"
#include <queue>
#include <vector>
#include <glm/glm.hpp>

// Priority Queue Struct for Ruppert's Algorithm
struct RuppertTriangle {
    int face_idx;
    float priority;
    int v0, v1, v2;

    bool operator<(const RuppertTriangle& other) const {
        return priority < other.priority;
    }
};

// Priority Queue Struct for Garland's Algorithm
struct GarlandTriangle {
    int face_idx;
    float priority;
    glm::vec2 max_error_norm_pt; // The normalized [0,1] XZ coordinate of the worst error
    int v0, v1, v2;

    bool operator<(const GarlandTriangle& other) const {
        return priority < other.priority;
    }
};

class TerrainMesher {
public:
    TerrainMesher(DCEL& d, Delaunay& t, Heightmap& h);

    // The single public entry point for your background thread
    // algo: 0 = Baseline (Uniform), 1 = Ruppert, 2 = Garland
    void rebuild_mesh(const glm::vec3& camera_pos, float fov, int max_vertices, int algo, const std::vector<glm::vec3>& base_corners);

private:
    DCEL& dcel;
    Delaunay& triangulator;
    Heightmap& heightmap;

    // The algorithms
    void generate_baseline(int max_vertices, const std::vector<glm::vec3>& base_corners);
    void generate_sobel(int max_vertices, const std::vector<glm::vec3>& base_corners);
    void generate_ruppert(const glm::vec3& camera_pos, float fov, int max_vertices, const std::vector<glm::vec3>& base_corners);
    void generate_garland(const glm::vec3& camera_pos, float fov, int max_vertices, const std::vector<glm::vec3>& base_corners);

    // Ruppert-specific helpers
    float calculate_ruppert_priority(int face_idx, const glm::vec3& camera_pos, float fov);
    void queue_ruppert_triangles(int vertex_idx, const glm::vec3& camera_pos, float fov);
    std::priority_queue<RuppertTriangle> bad_triangles;

    // Garland-specific helpers
    GarlandTriangle calculate_garland_error(int face_idx, const glm::vec3& camera_pos, float fov);
    std::priority_queue<GarlandTriangle> error_queue;
};