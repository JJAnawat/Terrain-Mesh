#include "DCEL.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// Construction

int DCEL::add_vertex(const glm::vec3& pos) {
    int idx = vertices.size();
    vertices.push_back({pos, -1});
    return idx;
}

int DCEL::add_triangle(int v0, int v1, int v2) {
    int face_idx = faces.size();
    
    // Create 3 half-edges in CCW order: v0→v1, v1→v2, v2→v0
    int he0_idx = half_edges.size();
    int he1_idx = he0_idx + 1;
    int he2_idx = he0_idx + 2;
    
    // Half-edge 0: v0 → v1
    half_edges.push_back({
        v0,           // origin
        -1,           // twin (not set yet)
        he1_idx,      // next
        he2_idx,      // prev
        face_idx      // face
    });
    
    // Half-edge 1: v1 → v2
    half_edges.push_back({
        v1,           // origin
        -1,           // twin (not set yet)
        he2_idx,      // next
        he0_idx,      // prev
        face_idx      // face
    });
    
    // Half-edge 2: v2 → v0
    half_edges.push_back({
        v2,           // origin
        -1,           // twin (not set yet)
        he0_idx,      // next
        he1_idx,      // prev
        face_idx      // face
    });
    
    // Create the face
    faces.push_back({he0_idx, false});  // half_edge points to any of the 3 (we choose he0)
    
    // Update vertices to point to one of their incident half-edges
    if (vertices[v0].half_edge == -1) vertices[v0].half_edge = he0_idx;
    if (vertices[v1].half_edge == -1) vertices[v1].half_edge = he1_idx;
    if (vertices[v2].half_edge == -1) vertices[v2].half_edge = he2_idx;
    
    return face_idx;
}

void DCEL::link_twins(int he0, int he1) {
    // Validate
    if (he0 < 0 || he0 >= half_edges.size() || 
        he1 < 0 || he1 >= half_edges.size()) {
        std::cerr << "ERROR: Invalid half-edge indices in link_twins\n";
        return;
    }
    
    // Check that they form a valid pair: he0.origin == he1.next.origin
    int he0_origin = half_edges[he0].origin;
    int he0_dest = half_edges[half_edges[he0].next].origin;  // next's origin is this edge's destination
    int he1_origin = half_edges[he1].origin;
    int he1_dest = half_edges[half_edges[he1].next].origin;
    
    if (he0_origin != he1_dest || he0_dest != he1_origin) {
        std::cerr << "ERROR: Half-edges don't form a valid pair in link_twins\n";
        std::cerr << "  he0: " << he0_origin << " → " << he0_dest << "\n";
        std::cerr << "  he1: " << he1_origin << " → " << he1_dest << "\n";
        return;
    }
    
    half_edges[he0].twin = he1;
    half_edges[he1].twin = he0;
}

void DCEL::reserve_memory(int max_vertices){
    vertices.reserve(max_vertices);
    faces.reserve(max_vertices*2);
    half_edges.reserve(max_vertices*6);
}

// Traverse

std::array<int, 3> DCEL::face_vertices(int face_idx) const {
    // Get the three vertices of a triangle.
    const Face& f = faces[face_idx];
    int he0 = f.half_edge;
    int he1 = half_edges[he0].next;
    int he2 = half_edges[he1].next;
    
    return {
        half_edges[he0].origin,
        half_edges[he1].origin,
        half_edges[he2].origin
    };
}

std::array<int, 3> DCEL::face_half_edges(int face_idx) const {
    // Get the three half-edges of a triangle (in CCW order).
    const Face& f = faces[face_idx];
    int he0 = f.half_edge;
    int he1 = half_edges[he0].next;
    int he2 = half_edges[he1].next;
    return {he0, he1, he2};
}

std::array<std::pair<int, int>, 3> DCEL::face_edges(int face_idx) const {
    // Get the three edges of a triangle as (from, to) pairs.
    auto [v0, v1, v2] = face_vertices(face_idx);
    return {
        std::make_pair(v0, v1),
        std::make_pair(v1, v2),
        std::make_pair(v2, v0)
    };
}

std::vector<int> DCEL::vertex_half_edges(int vertex_idx) const {
    // keep only the beginning of the triangle (1 he)
    std::vector<int> result;
    if (vertices[vertex_idx].half_edge == -1) return result;  // isolated vertex
    
    int start = vertices[vertex_idx].half_edge;
    int current = start;
    
    do {
        result.push_back(current);
        
        current = half_edges[half_edges[current].prev].twin;
        
        if (current == -1) break;  // reached boundary of mesh
    } while (current != start && result.size() < 10000);  // safety limit
    
    return result;
}

std::vector<int> DCEL::vertex_neighbors(int vertex_idx) const {
    // Collect all vertices adjacent to this vertex.
    std::vector<int> neighbors;
    auto half_edges_v = vertex_half_edges(vertex_idx);
    
    for (int he : half_edges_v) {
        int next_he = half_edges[he].next;
        int neighbor = half_edges[next_he].origin;
        neighbors.push_back(neighbor);
    }
    
    return neighbors;
}

int DCEL::opposite_face(int half_edge_idx) const {
    // Get the face on the opposite side of this half-edge.
    int twin = half_edges[half_edge_idx].twin;
    if (twin == -1) return -1;
    return half_edges[twin].face;
}

// Geometric

glm::vec2 DCEL::circumcenter(int face_idx) const {
    // Compute the circumcenter of a triangle using the standard formula.
    auto [v0, v1, v2] = face_vertices(face_idx);
    
    glm::vec2 a = glm::vec2(vertices[v0].pos);
    glm::vec2 b = glm::vec2(vertices[v1].pos);
    glm::vec2 c = glm::vec2(vertices[v2].pos);
    
    // Formula: circumcenter of triangle abc
    float d = 2.0f * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::abs(d) < 1e-10f) {
        // Degenerate triangle
        return (a + b + c) / 3.0f;
    }
    
    float a_len2 = a.x * a.x + a.y * a.y;
    float b_len2 = b.x * b.x + b.y * b.y;
    float c_len2 = c.x * c.x + c.y * c.y;

    float ux = (a_len2 * (b.y - c.y) + b_len2 * (c.y - a.y) + c_len2 * (a.y - b.y)) / d;
    float uy = (a_len2 * (c.x - b.x) + b_len2 * (a.x - c.x) + c_len2 * (b.x - a.x)) / d;
    
    return {ux, uy};
}

float DCEL::circumradius(int face_idx) const {
    // Compute the circumradius of a triangle.
    auto [v0, v1, v2] = face_vertices(face_idx);
    
    glm::vec2 a = glm::vec2(vertices[v0].pos);
    glm::vec2 b = glm::vec2(vertices[v1].pos);
    glm::vec2 c = glm::vec2(vertices[v2].pos);
    
    float ab = glm::distance(a, b);
    float bc = glm::distance(b, c);
    float ca = glm::distance(c, a);
    
    float s = (ab + bc + ca) / 2.0f;  // semi-perimeter
    float area = std::sqrt(std::max(0.0f, s * (s - ab) * (s - bc) * (s - ca)));
    
    if (area < 1e-10f) return 1e10f;  // degenerate
    
    return (ab * bc * ca) / (4.0f * area);
}

float DCEL::shortest_edge(int face_idx) const {
    // Find the shortest edge of a triangle.
    auto [v0, v1, v2] = face_vertices(face_idx);
    
    float d01 = glm::distance(glm::vec2(vertices[v0].pos), glm::vec2(vertices[v1].pos));
    float d12 = glm::distance(glm::vec2(vertices[v1].pos), glm::vec2(vertices[v2].pos));
    float d20 = glm::distance(glm::vec2(vertices[v2].pos), glm::vec2(vertices[v0].pos));
    
    return std::min({d01, d12, d20});
}

float DCEL::in_circle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 p) {
    // In-circle test using determinant.
    // Returns positive if p is inside circumcircle(a,b,c), negative if outside.
    
    float ax = a.x, ay = a.y;
    float bx = b.x, by = b.y;
    float cx = c.x, cy = c.y;
    float px = p.x, py = p.y;

    float adx = ax - px, ady = ay - py;
    float bdx = bx - px, bdy = by - py;
    float cdx = cx - px, cdy = cy - py;
    
    float alift = adx*adx + ady*ady;
    float blift = bdx*bdx + bdy*bdy;
    float clift = cdx*cdx + cdy*cdy;
    
    float det = adx*(bdy*clift - blift*cdy) - ady*(bdx*clift - blift*cdx) + alift*(bdx*cdy - bdy*cdx);
    
    return det;
}

double DCEL::get_min_weight(glm::vec2 p, int v0, int v1, int v2) const {
    glm::vec2 a(vertices[v0].pos.x, vertices[v0].pos.z);
    glm::vec2 b(vertices[v1].pos.x, vertices[v1].pos.z);
    glm::vec2 c(vertices[v2].pos.x, vertices[v2].pos.z);
    
    auto edge_ccw = [](glm::vec2 p1, glm::vec2 p2, glm::vec2 pt) {
        return (double)(p2.x - p1.x) * (double)(pt.y - p1.y) - 
               (double)(p2.y - p1.y) * (double)(pt.x - p1.x);
    };
    
    double w0 = edge_ccw(a, b, p);
    double w1 = edge_ccw(b, c, p);
    double w2 = edge_ccw(c, a, p);
    
    return std::min({w0, w1, w2});
}

// Helper function to check if point p is inside triangle (v0, v1, v2)
bool DCEL::point_in_triangle(glm::vec2 p, int v0, int v1, int v2) const {
    auto edge_side = [&](int a_idx, int b_idx) {
        glm::vec2 a = glm::vec2(vertices[a_idx].pos.x, vertices[a_idx].pos.z);
        glm::vec2 b = glm::vec2(vertices[b_idx].pos.x, vertices[b_idx].pos.z);
        return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
    };
    // Returns true if p is to the left of (or on) all three directed edges
    return edge_side(v0, v1) >= -1e-5f && 
           edge_side(v1, v2) >= -1e-5f && 
           edge_side(v2, v0) >= -1e-5f;
}

// The master toggle function
int DCEL::locate_point(glm::vec2 p, int start_face) const {
    if (enable_dag && !dag.empty()) {
        return locate_point_dag(p);
    } else {
        return locate_point_walk(p, start_face);
    }
}

// O(log n) DAG Traversal
int DCEL::locate_point_dag(glm::vec2 p) const {
    int curr_node = 0; // Start at root (supertriangle)
    
    // Iterate down the tree until we hit an active leaf node
    while (!dag[curr_node].active) {
        bool found_child = false;
        for (int child_idx : dag[curr_node].children) {
            if (point_in_triangle(p, dag[child_idx].v0, dag[child_idx].v1, dag[child_idx].v2)) {
                curr_node = child_idx;
                found_child = true;
                break;
            }
        }
        // Fallback if precision issues cause point to slip through cracks
        if (!found_child) return dag[curr_node].children[0]; 
    }
    return dag[curr_node].dcel_face;
}

// O(1) ~ O(sqrt N) Directed Edge Walk
int DCEL::locate_point_walk(glm::vec2 p, int start_face) const {
    int curr_face = start_face;
    if (curr_face < 0 || curr_face >= faces.size() || faces[curr_face].is_outer) curr_face = 0;

    int max_steps = faces.size() + 100; 
    while (max_steps-- > 0) {
        auto [he0, he1, he2] = face_half_edges(curr_face);
        
        auto edge_side = [&](int he_idx) {
            glm::vec2 a = glm::vec2(vertices[half_edges[he_idx].origin].pos.x, vertices[half_edges[he_idx].origin].pos.z);
            glm::vec2 b = glm::vec2(vertices[half_edges[half_edges[he_idx].next].origin].pos.x, vertices[half_edges[half_edges[he_idx].next].origin].pos.z);
            return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
        };

        if (edge_side(he0) < -1e-7f) {
            int twin = half_edges[he0].twin;
            if (twin == -1) return curr_face;
            curr_face = half_edges[twin].face;
        } else if (edge_side(he1) < -1e-7f) {
            int twin = half_edges[he1].twin;
            if (twin == -1) return curr_face;
            curr_face = half_edges[twin].face;
        } else if (edge_side(he2) < -1e-7f) {
            int twin = half_edges[he2].twin;
            if (twin == -1) return curr_face;
            curr_face = half_edges[twin].face;
        } else {
            return curr_face; // Point is inside!
        }
    }
    return curr_face;
}

// Mesh

int DCEL::insert_point_in_face(int face_idx, glm::vec2 p, float z) {
    auto [v0, v1, v2] = face_vertices(face_idx);
    auto [he0, he1, he2] = face_half_edges(face_idx);

    int v_new = add_vertex(glm::vec3(p.x, z, p.y));

    // We reuse the original face_idx for the first new triangle (v0, v1, v_new)
    int f0 = face_idx; 
    
    // Create two new faces for the other two triangles
    int f1 = faces.size();
    faces.push_back({he1, false}); // Face (v1, v2, v_new) points to he1
    
    int f2 = faces.size();
    faces.push_back({he2, false}); // Face (v2, v0, v_new) points to he2

    // Update the original face's starting edge just to be perfectly safe
    faces[f0].half_edge = he0;

    // 4. Allocate 6 new half-edges (3 pairs of twins radiating from v_new)
    int start_he = half_edges.size();
    int h1n = start_he + 0; // v1 -> v_new
    int hn1 = start_he + 1; // v_new -> v1
    int h2n = start_he + 2; // v2 -> v_new
    int hn2 = start_he + 3; // v_new -> v2
    int h0n = start_he + 4; // v0 -> v_new
    int hn0 = start_he + 5; // v_new -> v0

    // Push dummy data to resize the vector, we will overwrite it immediately
    for (int i = 0; i < 6; ++i) {
        half_edges.push_back(HalfEdge{});
    }

    // 5. Wire everything together!
    // Format for reference: {origin, twin, next, prev, face}

    // --- Triangle 0: (v0, v1, v_new) ---
    // he0 (v0 -> v1) already exists and has the correct twin/origin. Just update its internal loop.
    half_edges[he0].face = f0;
    half_edges[he0].next = h1n;
    half_edges[he0].prev = hn0;

    half_edges[h1n] = {v1,    hn1, hn0, he0, f0};
    half_edges[hn0] = {v_new, h0n, he0, h1n, f0};

    // --- Triangle 1: (v1, v2, v_new) ---
    // he1 (v1 -> v2) already exists.
    half_edges[he1].face = f1;
    half_edges[he1].next = h2n;
    half_edges[he1].prev = hn1;

    half_edges[h2n] = {v2,    hn2, hn1, he1, f1};
    half_edges[hn1] = {v_new, h1n, he1, h2n, f1};

    // --- Triangle 2: (v2, v0, v_new) ---
    // he2 (v2 -> v0) already exists.
    half_edges[he2].face = f2;
    half_edges[he2].next = h0n;
    half_edges[he2].prev = hn2;

    half_edges[h0n] = {v0,    hn0, hn2, he2, f2};
    half_edges[hn2] = {v_new, h2n, he2, h0n, f2};

    vertices[v_new].half_edge = hn0; 

    // Assurance stuffs
    vertices[v0].half_edge = he0;
    vertices[v1].half_edge = he1;
    vertices[v2].half_edge = he2;
    
    if (enable_dag) {
        int old_dag_idx = face_to_dag[f0];
        dag[old_dag_idx].active = false;

        int n0 = dag.size();
        dag.push_back({v0, v1, v_new, {}, f0, true});
        int n1 = dag.size();
        dag.push_back({v1, v2, v_new, {}, f1, true});
        int n2 = dag.size();
        dag.push_back({v2, v0, v_new, {}, f2, true});

        face_to_dag[f0] = n0;
        face_to_dag[f1] = n1;
        face_to_dag[f2] = n2;

        dag[old_dag_idx].children.push_back(n0);
        dag[old_dag_idx].children.push_back(n1);
        dag[old_dag_idx].children.push_back(n2);
    }

    return v_new;
}

void DCEL::flip_edge(int half_edge_idx) {
    int e0 = half_edge_idx;
    int t0 = half_edges[e0].twin;

    if (t0 == -1) {
        std::cerr << "ERROR: Cannot flip boundary edge\n";
        return;
    }

    // 1. Gather the 6 half-edges involved in the two triangles
    // Triangle 1 (e.g., A -> B -> C -> A)
    int e1 = half_edges[e0].next;
    int e2 = half_edges[e1].next;

    // Triangle 2 (e.g., B -> A -> D -> B)
    int t1 = half_edges[t0].next;
    int t2 = half_edges[t1].next;

    // 2. Gather the 4 vertices based on the half-edge origins
    int v_A = half_edges[e0].origin;
    int v_B = half_edges[e1].origin;
    int v_C = half_edges[e2].origin;
    int v_D = half_edges[t2].origin;

    // 3. Gather the 2 faces
    int f_0 = half_edges[e0].face; // Originally ABC
    int f_1 = half_edges[t0].face; // Originally BAD

    // Rewrite

    // --- Repurpose f_0 to be triangle C -> A -> D ---
    faces[f_0].half_edge = e2; // Guarantee the face points to a valid edge

    // e2 (C -> A) stays mostly the same, just update neighbors
    half_edges[e2].next = t1;
    half_edges[e2].prev = t0;
    half_edges[e2].face = f_0; 

    // t1 (A -> D) moves into f_0
    half_edges[t1].next = t0;
    half_edges[t1].prev = e2;
    half_edges[t1].face = f_0;

    // t0 (was B -> A) becomes D -> C
    half_edges[t0].origin = v_D;
    half_edges[t0].next = e2;
    half_edges[t0].prev = t1;
    half_edges[t0].face = f_0;

    // --- Repurpose f_1 to be triangle C -> D -> B ---
    faces[f_1].half_edge = e1; // Guarantee the face points to a valid edge

    // e0 (was A -> B) becomes C -> D
    half_edges[e0].origin = v_C;
    half_edges[e0].next = t2;
    half_edges[e0].prev = e1;
    half_edges[e0].face = f_1;

    // t2 (D -> B) stays mostly the same, just update neighbors
    half_edges[t2].next = e1;
    half_edges[t2].prev = e0;
    half_edges[t2].face = f_1;

    // e1 (B -> C) moves into f_1
    half_edges[e1].next = e0;
    half_edges[e1].prev = t2;
    half_edges[e1].face = f_1;

    // Update vertex half edges
    vertices[v_A].half_edge = t1; // A -> D
    vertices[v_B].half_edge = e1; // B -> C
    vertices[v_C].half_edge = e2; // C -> A
    vertices[v_D].half_edge = t2; // D -> B

    if (enable_dag) {
        int old_dag_0 = face_to_dag[f_0];
        int old_dag_1 = face_to_dag[f_1];

        dag[old_dag_0].active = false;
        dag[old_dag_1].active = false;

        int n0 = dag.size();
        dag.push_back({v_C, v_A, v_D, {}, f_0, true});
        int n1 = dag.size();
        dag.push_back({v_C, v_D, v_B, {}, f_1, true});

        face_to_dag[f_0] = n0;
        face_to_dag[f_1] = n1;

        dag[old_dag_0].children.push_back(n0);
        dag[old_dag_0].children.push_back(n1);

        dag[old_dag_1].children.push_back(n0);
        dag[old_dag_1].children.push_back(n1);
    }
}

// Validation

bool DCEL::is_valid() const {
    // Check basic consistency.
    for (size_t i = 0; i < half_edges.size(); i++) {
        const auto& he = half_edges[i];
        
        // Check that origin vertex exists
        if (he.origin < 0 || he.origin >= vertices.size()) {
            std::cerr << "ERROR: Half-edge " << i << " has invalid origin\n";
            return false;
        }
        
        // Check that next/prev form a cycle
        if (he.next < 0 || he.next >= half_edges.size()) {
            std::cerr << "ERROR: Half-edge " << i << " has invalid next\n";
            return false;
        }
    }
    
    return true;
}

std::vector<std::array<int, 3>> DCEL::get_triangles() const {
    // Extract all triangles as vertex triples for rendering.
    std::vector<std::array<int, 3>> triangles;
    
    for (size_t i = 0; i < faces.size(); i++) {
        if (faces[i].is_outer) continue;
        auto verts = face_vertices(i);
        triangles.push_back(verts);
    }
    
    return triangles;
}

int DCEL::triangle_count() const {
    int count = 0;
    for (const auto& face : faces) {
        if (!face.is_outer) count++;
    }
    return count;
}

std::array<int, 3> DCEL::create_supertriangle(const std::vector<glm::vec2>& points) {
    // Create a huge triangle that encloses all points.
    
    // Find bounding box
    float min_x = 1e10f, max_x = -1e10f;
    float min_y = 1e10f, max_y = -1e10f;
    
    for (const auto& p : points) {
        min_x = std::min(min_x, p.x);
        max_x = std::max(max_x, p.x);
        min_y = std::min(min_y, p.y);
        max_y = std::max(max_y, p.y);
    }
    
    float dx = max_x - min_x;
    float dy = max_y - min_y;
    float dmax = std::max(dx, dy);
    float mid_x = (min_x + max_x) / 2.0f;
    float mid_y = (min_y + max_y) / 2.0f;
    
    // Create three vertices of a large triangle
    // (far outside the bounding box)
    glm::vec3 v0(mid_x - 20*dmax, 0.0f, mid_y - dmax);
    glm::vec3 v1(mid_x + 20*dmax, 0.0f, mid_y - dmax);
    glm::vec3 v2(mid_x,           0.0f, mid_y + 20*dmax);
    
    int idx0 = add_vertex(v0);
    int idx1 = add_vertex(v1);
    int idx2 = add_vertex(v2);

    int face_idx = add_triangle(idx0, idx1, idx2);

    if (enable_dag){
        dag.clear();
        face_to_dag.clear();
        face_to_dag.resize(100000000, -1);

        dag.push_back({idx0, idx1, idx2, {}, face_idx, true});
        face_to_dag[face_idx] = 0;
    }
    
    return {idx0, idx1, idx2};
}