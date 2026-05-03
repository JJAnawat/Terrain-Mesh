#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glm/glm.hpp>
#include <vector>

inline std::vector<glm::vec3> computeVertexNormals(
    const std::vector<glm::vec3>& vertices,
    const std::vector<unsigned int>& indices) {
    
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));
    
    // For each triangle, compute face normal and accumulate at vertices
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        
        glm::vec3 v0 = vertices[i0];
        glm::vec3 v1 = vertices[i1];
        glm::vec3 v2 = vertices[i2];
        
        // Compute face normal
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 faceNormal = glm::cross(edge1, edge2);
        
        // Accumulate at each vertex of the triangle
        normals[i0] += faceNormal;
        normals[i1] += faceNormal;
        normals[i2] += faceNormal;
    }
    
    // Normalize accumulated normals
    for (auto& n : normals) {
        if (glm::length(n) > 0.0f) {
            n = glm::normalize(n);
        }
    }
    
    return normals;
}

inline std::vector<glm::vec3> generateWhiteColors(const std::vector<glm::vec3>& vertices) {
    return std::vector<glm::vec3>(vertices.size(), glm::vec3(1.0f, 1.0f, 1.0f));
}

inline std::vector<glm::vec3> generateElevationColors(const std::vector<glm::vec3>& vertices) {
    std::vector<glm::vec3> colors;
    colors.reserve(vertices.size());

    float min_y = std::numeric_limits<float>::max();
    float max_y = std::numeric_limits<float>::lowest();
    for (const auto& v : vertices) {
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }

    // Prevent division by 0
    if (max_y - min_y < 0.001f) {
        max_y = min_y + 1.0f;
    }

    glm::vec3 color_water(0.1f, 0.3f, 0.8f);   // Deep Blue
    glm::vec3 color_sand (0.8f, 0.7f, 0.4f);   // Sandy Yellow
    glm::vec3 color_grass(0.2f, 0.6f, 0.2f);   // Forest Green
    glm::vec3 color_rock (0.5f, 0.5f, 0.5f);   // Grey
    glm::vec3 color_snow (0.95f, 0.95f, 0.95f);// White

    // Map each vertex's height to a color
    for (const auto& v : vertices) {
        // Normalize height between 0.0 and 1.0
        float h = (v.y - min_y) / (max_y - min_y);
        glm::vec3 final_color;

        if (h < 0.15f) {
            // Water to Sand
            float t = h / 0.15f;
            final_color = glm::mix(color_water, color_sand, t);
        } 
        else if (h < 0.45f) {
            // Sand to Grass
            float t = (h - 0.15f) / 0.30f;
            final_color = glm::mix(color_sand, color_grass, t);
        } 
        else if (h < 0.75f) {
            // Grass to Rock
            float t = (h - 0.45f) / 0.30f;
            final_color = glm::mix(color_grass, color_rock, t);
        } 
        else {
            // Rock to Snow
            float t = (h - 0.75f) / 0.25f;
            final_color = glm::mix(color_rock, color_snow, t);
        }

        colors.push_back(final_color);
    }

    return colors;
}

#endif
