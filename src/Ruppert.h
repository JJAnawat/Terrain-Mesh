#pragma once
#ifndef RUPPERT_H
#define RUPPERT_H

#include <glm/glm.hpp>
#include "DCEL.h"
#include "Delaunay.h"
#include "Heightmap.h"
#include <queue>
#include <functional>

struct QueuedTriangle {
    int face_idx;
    float priority;
    int v0, v1, v2;

    bool operator<(const QueuedTriangle& other) const {
        return priority < other.priority;
    }
};

class Ruppert {
    public:
        DCEL& dcel;
        Delaunay& triangulator;
        Heightmap& heightmap;

        float lod_threshold = 1.5f;
        int max_vertices = 20000;

        Ruppert(DCEL& mesh, Delaunay& tri, Heightmap& hm): 
            dcel(mesh), triangulator(tri), heightmap(hm)
        {};

        void rebuild_mesh(const glm::vec3& camera_pos, float fov, const std::vector<glm::vec3>& base_corners, int num_importance_sample);

    private:
        std::priority_queue<QueuedTriangle> bad_triangles;

        float calculate_priority(int face_idx, const glm::vec3& camera_pos, float fov);

        void queue_vertex_triangles(int vertex_idx, const glm::vec3& camera_pos, float fov);
};

#endif