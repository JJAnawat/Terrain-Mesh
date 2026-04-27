#include "delaunay.h"
#include <numeric>
#include <random>
#include <algorithm>

void Delaunay::build(const std::vector<glm::vec3>& points) {
    if (points.empty()) return;

    std::vector<glm::vec2> points2d;
    points2d.reserve(points.size());
    for (const auto& p : points) {
        points2d.push_back(glm::vec2(p.x, p.z));
    }

    dcel.create_supertriangle(points2d);

    // Create a randomized insertion order.
    std::vector<int> indices(points.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(indices.begin(), indices.end(), g);

    // Insert points one by one
    for (int i : indices) {
        insert_point(points[i]);
    }
}

int Delaunay::insert_point(const glm::vec3& p, int start_face) {
    glm::vec2 p2d(p.x, p.z);
    
    // locate point
    int face_idx = dcel.locate_point(p2d, start_face);
    if (face_idx == -1) return -1; // Should never happen while inside the supertriangle
    
    // Get the 3 outer half-edges of the triangle before split it
    auto [he0, he1, he2] = dcel.face_half_edges(face_idx);
    
    // Insert the point (1-to-3 split). 
    int v_new = dcel.insert_point_in_face(face_idx, p2d, p.y);
    
    // Restore the Delaunay property by checking the 3 edges opposite to the new vertex
    legalize_edge(v_new, he0);
    legalize_edge(v_new, he1);
    legalize_edge(v_new, he2);

    return v_new;
}

void Delaunay::legalize_edge(int pr, int he) {
    int twin = dcel.half_edges[he].twin;
    
    // If it's a boundary edge of the supertriangle, it's always legal
    if (twin == -1) return;
    
    // Triangle 1 (contains the new point pr): pi -> pj -> pr
    // Triangle 2 (opposite neighbor):         pj -> pi -> pk
    
    int pi = dcel.half_edges[he].origin;
    int pj = dcel.half_edges[dcel.half_edges[he].next].origin;
    int pk = dcel.half_edges[dcel.half_edges[twin].prev].origin;
    
    glm::vec2 p_r(dcel.vertices[pr].pos.x, dcel.vertices[pr].pos.z);
    glm::vec2 p_i(dcel.vertices[pi].pos.x, dcel.vertices[pi].pos.z);
    glm::vec2 p_j(dcel.vertices[pj].pos.x, dcel.vertices[pj].pos.z);
    glm::vec2 p_k(dcel.vertices[pk].pos.x, dcel.vertices[pk].pos.z);
    
    // In-Circle Test
    // Positive determinant means pk is strictly inside the circumcircle of pi, pj, pr.
    double adx = (double)p_i.x - p_k.x;
    double ady = (double)p_i.y - p_k.y;
    double bdx = (double)p_j.x - p_k.x;
    double bdy = (double)p_j.y - p_k.y;
    double cdx = (double)p_r.x - p_k.x;
    double cdy = (double)p_r.y - p_k.y;
    
    double alift = adx * adx + ady * ady;
    double blift = bdx * bdx + bdy * bdy;
    double clift = cdx * cdx + cdy * cdy;
    
    double det = adx * (bdy * clift - blift * cdy) 
               - ady * (bdx * clift - blift * cdx) 
               + alift * (bdx * cdy - bdy * cdx);
               
    if (det > 0.0) {
        int t_next = dcel.half_edges[twin].next; // Edge pi -> pk
        int t_prev = dcel.half_edges[twin].prev; // Edge pk -> pj
        
        dcel.flip_edge(he);
        
        // Recursively check the newly exposed outer edges
        legalize_edge(pr, t_prev);
        legalize_edge(pr, t_next);
    }
}