#include "Ruppert.h"
#include <iostream>
#include <algorithm>

void Ruppert::rebuild_mesh(
    const glm::vec3& camera_pos, 
    float fov,
    const std::vector<glm::vec3>& base_corners, 
    int num_importance_sample
){
    dcel.reserve_memory(max_vertices);

    dcel.vertices.clear();
    dcel.half_edges.clear();
    dcel.faces.clear();

    bad_triangles = std::priority_queue<QueuedTriangle>();

    std::vector<glm::vec3> sampled_points = heightmap.importanceSample(num_importance_sample);

    std::vector<glm::vec3> all_points = base_corners;
    all_points.insert(all_points.end(), sampled_points.begin(), sampled_points.end());

    triangulator.build(all_points);

    for(int i=0; i< dcel.faces.size(); i++){
        if(!dcel.faces[i].is_outer){
            auto [v0, v1, v2] = dcel.face_vertices(i);
            bad_triangles.push({i, calculate_priority(i, camera_pos, fov), v0, v1, v2});
        }
    }

    int points_inserted = all_points.size();
    
    while(!bad_triangles.empty() && points_inserted < max_vertices){
        QueuedTriangle worst = bad_triangles.top();
        bad_triangles.pop();
        
        std::array<int, 3> current_v = dcel.face_vertices(worst.face_idx);
        std::array<int, 3> queued_v = {worst.v0, worst.v1, worst.v2};

        std::sort(current_v.begin(), current_v.end());
        std::sort(queued_v.begin(), queued_v.end());

        if(current_v != queued_v)
            continue; // Ghost Triangle

        if(worst.priority < lod_threshold)
            break;

        // Do not refine for already small to prevent weird stuff
        float min_allowed_edge = 1.0f;
        if (dcel.shortest_edge(worst.face_idx) < min_allowed_edge) 
            continue;

        glm::vec2 cc = dcel.circumcenter(worst.face_idx);

        if (cc.x < 0.0f || cc.x > heightmap.getWidth() || cc.y < 0.0f || cc.y > heightmap.getHeight()) 
            continue; 

        float true_elevation = heightmap.bilinearInterpolate(cc.x, cc.y);
        
        int v_new = triangulator.insert_point(glm::vec3(cc.x, true_elevation, cc.y), worst.face_idx);
        if(v_new == -1)
            continue;

        points_inserted++;

        queue_vertex_triangles(v_new, camera_pos, fov);
    }
}

float Ruppert::calculate_priority(int face_idx, const glm::vec3& camera_pos, float fov){
    float cr = dcel.circumradius(face_idx);
    float se = dcel.shortest_edge(face_idx);
    float shape_factor = cr / (se + 1e-6f);

    glm::vec2 cc = dcel.circumcenter(face_idx);
    float true_elevation = heightmap.bilinearInterpolate(cc.x, cc.y);

    auto [v0, v1, v2] = dcel.face_vertices(face_idx);
    float y0 = dcel.vertices[v0].pos.y;
    float y1 = dcel.vertices[v1].pos.y;
    float y2 = dcel.vertices[v2].pos.y;

    float interp_elevation = (y0 + y1 + y2) / 3.0f;
    float error = std::abs(true_elevation - interp_elevation);

    glm::vec3 cc_3d(cc.x, true_elevation, cc.y);
    float dist = glm::distance(camera_pos, cc_3d);

    float fov_radians = glm::radians(fov);
    float eff_dist = dist * std::tan(fov_radians / 2.0f);

    eff_dist = std::max(eff_dist, 0.1f);

    float priority = (error * 50.0f / dist) + (shape_factor * 0.5f);

    return priority;
}

void Ruppert::queue_vertex_triangles(int vertex_idx, const glm::vec3& camera_pos, float fov){
    std::vector<int> incident_hes = dcel.vertex_half_edges(vertex_idx);

    for(int he: incident_hes){
        int f = dcel.half_edges[he].face;
        if(!dcel.faces[f].is_outer){
            auto [v0, v1, v2] = dcel.face_vertices(f);
            bad_triangles.push({f, calculate_priority(f, camera_pos, fov), v0, v1, v2});
        }
    }
}