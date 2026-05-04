#include "TerrainMesher.h"
#include <iostream>
#include <algorithm>

TerrainMesher::TerrainMesher(DCEL& d, Delaunay& t, Heightmap& h) 
    : dcel(d), triangulator(t), heightmap(h) {}

void TerrainMesher::rebuild_mesh(const glm::vec3& camera_pos, float fov, int max_vertices, int algo, const std::vector<glm::vec3>& base_corners) {
    // 1. Pre-allocate memory to prevent stutters
    dcel.vertices.clear();
    dcel.half_edges.clear();
    dcel.faces.clear();
    
    dcel.vertices.reserve(max_vertices);
    dcel.faces.reserve(max_vertices * 2);
    dcel.half_edges.reserve(max_vertices * 6);

    // 2. Route to the correct algorithm
    if (algo == 0) {
        generate_baseline(max_vertices, base_corners);
    } 
    else if (algo == 1){
        dcel.enable_dag=true;
        generate_baseline(max_vertices, base_corners);
    }
    else if (algo == 2) {
        generate_sobel(max_vertices, base_corners);
    }
    else if (algo == 3) {
        dcel.enable_dag=true;
        generate_sobel(max_vertices, base_corners);
    }
    else if (algo == 4) {
        generate_ruppert(camera_pos, fov, max_vertices, base_corners);
    } 
    else if (algo == 5) {
        generate_garland(camera_pos, fov, max_vertices, base_corners);
    }
}

// ==========================================
// BASELINE ALGORITHM (Uniform Random)
// ==========================================
void TerrainMesher::generate_baseline(int max_vertices, const std::vector<glm::vec3>& base_corners) {
    // Generate purely random uniform points
    int points_to_sample = max_vertices - base_corners.size();
    std::vector<glm::vec3> sampled_points = heightmap.uniformSample(points_to_sample);
    
    std::vector<glm::vec3> all_points = base_corners;

    all_points.insert(all_points.end(), sampled_points.begin(), sampled_points.end());

    // Build static mesh in one shot
    triangulator.build(all_points);
}

// ==========================================
// SOBEL Importance Sampling
// ==========================================
void TerrainMesher::generate_sobel(int max_vertices, const std::vector<glm::vec3>& base_corners) {
    int points_to_sample = max_vertices - base_corners.size();
    std::vector<glm::vec3> sampled_points = heightmap.importanceSample(points_to_sample);

    std::vector<glm::vec3> all_points = base_corners;

    all_points.insert(all_points.end(), sampled_points.begin(), sampled_points.end());

    // Build static mesh in one shot
    triangulator.build(all_points);
}

// ==========================================
// RUPPERT'S ALGORITHM
// ==========================================
void TerrainMesher::generate_ruppert(const glm::vec3& camera_pos, float fov, int max_vertices, const std::vector<glm::vec3>& base_corners) {
    // Clear queue
    bad_triangles = std::priority_queue<RuppertTriangle>();

    // std::vector<glm::vec3> sampled_points = heightmap.importanceSample(0); 
    std::vector<glm::vec3> all_points = base_corners;
    // all_points.insert(all_points.end(), sampled_points.begin(), sampled_points.end());

    triangulator.build(all_points);
    
    int points_inserted = all_points.size();
    float lod_threshold = -10000.0f; // Tweak this cutoff

    for (int i = 0; i < dcel.faces.size(); i++) {
        if (!dcel.faces[i].is_outer) {
            auto [v0, v1, v2] = dcel.face_vertices(i);
            float prio = calculate_ruppert_priority(i, camera_pos, fov);
            if(prio >= lod_threshold && !(v0 < 3 || v1 < 3 || v2 < 3))
                bad_triangles.push({i, calculate_ruppert_priority(i, camera_pos, fov), v0, v1, v2});
            }
        }
        
        
    while (!bad_triangles.empty() && points_inserted < max_vertices) {
        RuppertTriangle worst = bad_triangles.top();
        bad_triangles.pop();
        
        // Ghost check
        std::array<int, 3> curr_v = dcel.face_vertices(worst.face_idx);
        std::array<int, 3> queued_v = {worst.v0, worst.v1, worst.v2};
        std::sort(curr_v.begin(), curr_v.end());
        std::sort(queued_v.begin(), queued_v.end());
        if (curr_v != queued_v) continue;
        
        // if (worst.priority < lod_threshold) break;
        // std::cout << "Worst priority is: " << worst.priority << "\n"; // UNCOMMENT TO DEBUG
        // if (dcel.shortest_edge(worst.face_idx) < 1.0f) continue;

        glm::vec2 cc = dcel.circumcenter(worst.face_idx);

        float we = heightmap.widthExtent, de=heightmap.depthExtent, mh=heightmap.maxHeight, margin=0.001f;
        if(cc.x < -we+margin || cc.x > we-margin || cc.y < -de+margin || cc.y > de-margin)
            continue;

        bool too_close = false;
        float min_dist_sq = 0.0001f;

        for (int v_idx : curr_v) {
            glm::vec2 existing_pos(dcel.vertices[v_idx].pos.x, dcel.vertices[v_idx].pos.z);
            float dx = cc.x - existing_pos.x;
            float dy = cc.y - existing_pos.y;
            if ((dx * dx + dy * dy) < min_dist_sq) {
                too_close = true;
                break;
            }
        }
        
        // Convert world circumcenter to normalized for height interpolation
        float norm_x = (cc.x / we) + 0.5f;
        float norm_z = (cc.y / de) + 0.5f; // cc.y is Z in 3D
        float true_elevation = heightmap.bilinearInterpolate(norm_x, norm_z) * mh;
        
        int v_new = triangulator.insert_point(glm::vec3(cc.x, true_elevation, cc.y), worst.face_idx);
        if (v_new == -1) continue;
            
        points_inserted++;
        queue_ruppert_triangles(v_new, camera_pos, fov);
    }
}

float TerrainMesher::calculate_ruppert_priority(int face_idx, const glm::vec3& camera_pos, float fov) {
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

void TerrainMesher::queue_ruppert_triangles(int vertex_idx, const glm::vec3& camera_pos, float fov) {
    std::vector<int> incident_hes = dcel.vertex_half_edges(vertex_idx);

    for(int he: incident_hes){
        int f = dcel.half_edges[he].face;
        if(!dcel.faces[f].is_outer){
            auto [v0, v1, v2] = dcel.face_vertices(f);
            bad_triangles.push({f, calculate_ruppert_priority(f, camera_pos, fov), v0, v1, v2});
        }
    }
}

// ==========================================
// GARLAND'S ALGORITHM (Top-Down Greedy)
// ==========================================
void TerrainMesher::generate_garland(const glm::vec3& camera_pos, float fov, int max_vertices, const std::vector<glm::vec3>& base_corners) {
    error_queue = std::priority_queue<GarlandTriangle>();

    triangulator.build(base_corners);

    for (int i = 0; i < dcel.faces.size(); i++) {
        if (!dcel.faces[i].is_outer) {
            error_queue.push(calculate_garland_error(i, camera_pos, fov));
        }
    }

    int points_inserted = base_corners.size();
    float error_threshold = 0.5f; // Stop if the worst error is tiny
    
    while (!error_queue.empty() && points_inserted < max_vertices) {
        GarlandTriangle worst = error_queue.top();
        error_queue.pop();

        // Ghost check
        std::array<int, 3> curr_v = dcel.face_vertices(worst.face_idx);
        std::array<int, 3> queued_v = {worst.v0, worst.v1, worst.v2};
        std::sort(curr_v.begin(), curr_v.end());
        std::sort(queued_v.begin(), queued_v.end());
        if (curr_v != queued_v) continue;

        if (worst.max_error < error_threshold) break;

        float world_x = (worst.max_error_norm_pt.x - 0.5f) * heightmap.widthExtent;
        float world_z = (worst.max_error_norm_pt.y - 0.5f) * heightmap.depthExtent;
        float true_elevation = heightmap.bilinearInterpolate(worst.max_error_norm_pt.x, worst.max_error_norm_pt.y) * heightmap.maxHeight;

        int v_new = triangulator.insert_point(glm::vec3(world_x, true_elevation, world_z), worst.face_idx);
        if (v_new == -1) continue;
            
        points_inserted++;

        std::vector<int> incident_hes = dcel.vertex_half_edges(v_new);
        for (int he : incident_hes) {
            int f = dcel.half_edges[he].face;
            if (!dcel.faces[f].is_outer) {
                error_queue.push(calculate_garland_error(f, camera_pos, fov));
            }
        }
    }
}

GarlandTriangle TerrainMesher::calculate_garland_error(int face_idx, const glm::vec3& camera_pos, float fov) {
    auto [v0, v1, v2] = dcel.face_vertices(face_idx);
    glm::vec3 p0 = dcel.vertices[v0].pos;
    glm::vec3 p1 = dcel.vertices[v1].pos;
    glm::vec3 p2 = dcel.vertices[v2].pos;

    // 1. Find 2D Bounding Box in World Space (XZ plane)
    float min_x = std::min({p0.x, p1.x, p2.x});
    float max_x = std::max({p0.x, p1.x, p2.x});
    float min_z = std::min({p0.z, p1.z, p2.z});
    float max_z = std::max({p0.z, p1.z, p2.z});

    // 2. Convert to Normalized [0, 1] Space to match heightmap pixels
    float n_min_x = std::clamp((min_x / heightmap.widthExtent) + 0.5f, 0.0f, 1.0f);
    float n_max_x = std::clamp((max_x / heightmap.widthExtent) + 0.5f, 0.0f, 1.0f);
    float n_min_z = std::clamp((min_z / heightmap.depthExtent) + 0.5f, 0.0f, 1.0f);
    float n_max_z = std::clamp((max_z / heightmap.depthExtent) + 0.5f, 0.0f, 1.0f);

    int px_min = n_min_x * (heightmap.width - 1);
    int px_max = n_max_x * (heightmap.width - 1);
    int pz_min = n_min_z * (heightmap.height - 1);
    int pz_max = n_max_z * (heightmap.height - 1);

    // Barycentric denominator 
    float denom = (p1.z - p2.z) * (p0.x - p2.x) + (p2.x - p1.x) * (p0.z - p2.z);
    if (std::abs(denom) < 1e-6f) {
        return {face_idx, 0.0f, glm::vec2(0.5f, 0.5f), v0, v1, v2}; // Degenerate
    }

    float max_err = -1.0f;
    glm::vec2 worst_norm_pt(0.5f, 0.5f);
    glm::vec3 worst_world_pt(p0);

    // Optimization: If the triangle covers the whole map, skip some pixels to go faster
    int step = ((px_max - px_min) * (pz_max - pz_min) > 10000) ? 2 : 1;

    // 3. Scan the bounding box
    for (int z = pz_min; z <= pz_max; z += step) {
        for (int x = px_min; x <= px_max; x += step) {
            
            float nx = (float)x / (heightmap.width - 1);
            float nz = (float)z / (heightmap.height - 1);

            float wx = (nx - 0.5f) * heightmap.widthExtent;
            float wz = (nz - 0.5f) * heightmap.depthExtent;

            // Barycentric Test 
            float w0 = ((p1.z - p2.z) * (wx - p2.x) + (p2.x - p1.x) * (wz - p2.z)) / denom;
            float w1 = ((p2.z - p0.z) * (wx - p2.x) + (p0.x - p2.x) * (wz - p2.z)) / denom;
            float w2 = 1.0f - w0 - w1;

            // If weights are positive, the pixel is INSIDE the triangle
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) {
                float interp_y = w0 * p0.y + w1 * p1.y + w2 * p2.y;
                float true_y = heightmap.getHeight(x, z) * heightmap.maxHeight;
                
                float error = std::abs(true_y - interp_y);

                if (error > max_err) {
                    max_err = error;
                    worst_norm_pt = glm::vec2(nx, nz);
                    worst_world_pt = glm::vec3(wx, true_y, wz);
                }
            }
        }
    }

    float dist = glm::distance(camera_pos, worst_world_pt);
    float fov_rad = glm::radians(fov);
    float eff_dist = std::max(dist * std::tan(fov_rad / 2.0f), 0.1f);
    
    float priority = (max_err * 50.0f) / eff_dist; 

    return {face_idx, priority, worst_norm_pt, v0, v1, v2};
}