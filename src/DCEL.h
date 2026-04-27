#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <array>
#include <optional>

// ============================================================================
// DCEL — Half-Edge Mesh Structure
// ============================================================================
// This is a **half-edge** (doubly-connected edge list) representation.
// Every undirected edge is split into two directed half-edges.
// Each half-edge belongs to exactly one face and points to its twin (opposite direction).

struct HalfEdge {
    int origin;      // vertex index this half-edge starts from
    int twin;        // index of the opposite half-edge (same geometric edge, opposite direction)
    int next;        // next half-edge around the same face (in CCW order)
    int prev;        // previous half-edge around the same face
    int face;        // face this half-edge bounds
};

struct Vertex {
    glm::vec3 pos;   // 3D position (x, y, z where z is elevation from heightmap)
    int half_edge;   // index of any half-edge emanating from this vertex
};

struct Face {
    int half_edge;   // index of any half-edge on the boundary of this face
    bool is_outer;   // true if this is the outer (infinite) face
};

class DCEL {
public:
    std::vector<Vertex> vertices;
    std::vector<HalfEdge> half_edges;
    std::vector<Face> faces;

    // ========================================================================
    // CONSTRUCTION & TOPOLOGY
    // ========================================================================

    /// Create a supertriangle that contains all points.
    /// Returns three vertex indices.
    std::array<int, 3> create_supertriangle(const std::vector<glm::vec2>& points);

    /// Add a single vertex to the mesh.
    /// Returns the vertex index.
    int add_vertex(const glm::vec3& pos);

    /// Add a face (triangle) given three vertex indices.
    /// Automatically creates 3 half-edges in CCW order and links them.
    /// Returns the face index.
    int add_triangle(int v0, int v1, int v2);

    /// Link two half-edges as twins (same geometric edge, opposite direction).
    /// Call this after triangles are created to stitch them together.
    void link_twins(int he0, int he1);

    void reserve_memory(int max_vertices);

    // ========================================================================
    // TRAVERSAL & QUERIES
    // ========================================================================

    /// Get all vertices of a face (triangle).
    std::array<int, 3> face_vertices(int face_idx) const;

    /// Get all half-edges of a face (in CCW order around the boundary).
    std::array<int, 3> face_half_edges(int face_idx) const;

    /// Get the three edges of a triangle (as pairs of vertex indices).
    /// Useful for rendering and geometric queries.
    std::array<std::pair<int, int>, 3> face_edges(int face_idx) const;

    /// Walk around a vertex, collecting all incident half-edges.
    /// Returns indices of half-edges radiating from this vertex (in CCW order).
    std::vector<int> vertex_half_edges(int vertex_idx) const;

    /// Walk around a vertex, collecting all adjacent vertices.
    std::vector<int> vertex_neighbors(int vertex_idx) const;

    /// Get the triangle opposite to the given half-edge (across its twin).
    /// Returns face index, or -1 if the twin has no valid face.
    int opposite_face(int half_edge_idx) const;

    // ========================================================================
    // GEOMETRIC QUERIES
    // ========================================================================

    /// Compute the circumcenter of a triangle.
    glm::vec2 circumcenter(int face_idx) const;

    /// Compute the circumradius of a triangle.
    float circumradius(int face_idx) const;

    /// Find the shortest edge of a triangle.
    float shortest_edge(int face_idx) const;

    /// In-circle test: is point p inside the circumcircle of triangle (a, b, c)?
    /// Uses the determinant method (sign of a 4×4 determinant).
    /// Returns: >0 if p is inside, <0 if outside, ~0 if on the circle.
    static float in_circle(glm::vec2 a, glm::vec2 b, glm::vec2 c, glm::vec2 p);

    double get_min_weight(glm::vec2 p, int v0, int v1, int v2) const;

    /// Locate which triangle contains point p (2D point location).
    /// Starts from a given face and walks the mesh until finding the containing triangle.
    /// Returns face index, or -1 if p is outside the mesh.
    int locate_point(glm::vec2 p, int start_face = 0) const;

    // ========================================================================
    // MESH MODIFICATION
    // ========================================================================

    /// Split a triangle by inserting a point inside it (3→6 triangles).
    /// The new vertex p is inserted at the centroid or circumcenter.
    /// Returns the index of the new vertex.
    int insert_point_in_face(int face_idx, glm::vec2 p, float z);

    /// Split an edge by inserting a point on it (affects 2 triangles → 4 triangles).
    /// The new vertex p lies on the geometric edge (half_edge_idx, half_edge.next).
    /// Returns the index of the new vertex.
    int insert_point_on_edge(int half_edge_idx, glm::vec2 p, float z);

    /// Flip an edge (swap the diagonal in a quad formed by two adjacent triangles).
    /// This is the core operation for restoring the Delaunay property.
    /// Precondition: the edge is internal (has a valid twin with a real face).
    /// Modifies connectivity but does NOT change vertex count.
    void flip_edge(int half_edge_idx);

    // ========================================================================
    // DEBUGGING & VALIDATION
    // ========================================================================

    /// Check mesh consistency: every half-edge has a valid twin, every face has valid edges.
    bool is_valid() const;

    /// Extract all triangles as (v0, v1, v2) vertex indices for rendering.
    std::vector<std::array<int, 3>> get_triangles() const;

    /// Count non-outer faces (interior triangles).
    int triangle_count() const;
};