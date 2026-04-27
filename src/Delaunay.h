#pragma once
#include "DCEL.h"
#include <vector>
#include <glm/glm.hpp>

class Delaunay {
public:
    DCEL& dcel;
    
    // Pass in your existing DCEL instance
    Delaunay(DCEL& mesh) : dcel(mesh) {}
    
    // Build delaunay
    void build(const std::vector<glm::vec3>& points);
    
    // Insertion of a single point
    void insert_point(const glm::vec3& p);
    
private:
    // The core recursive function that flips edges to restore the Delaunay property
    void legalize_edge(int pr, int he);
};