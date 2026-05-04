#include "Heightmap.h"
#include <stb/stb_image.h>
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

Heightmap::Heightmap(float we, float de, float mh) : 
    width(0), height(0), 
    widthExtent(we), depthExtent(de), maxHeight(mh) {}

Heightmap::~Heightmap() {}

bool Heightmap::load(const std::string& filepath){
    int channels;
    unsigned char* imageData = stbi_load(
        filepath.c_str(),
        &width,
        &height,
        &channels,
        1
    );

    if(!imageData){
        std::cerr << "Failed  to load heightmap: " << filepath << "\n";
        return false;
    }

    unsigned char min_pixel = 255;
    unsigned char max_pixel = 0;
    for(int i = 0; i < width * height; i++) {
        min_pixel = std::min(min_pixel, imageData[i]);
        max_pixel = std::max(max_pixel, imageData[i]);
    }

    // Prevent divide-by-zero if the image is a solid color
    float range = (max_pixel > min_pixel) ? (float)(max_pixel - min_pixel) : 1.0f;

    heightData.resize(width * height);
    for(int i = 0; i < width * height; i++) {
        heightData[i] = (imageData[i] - min_pixel) / range;
        assert(heightData[i] >= 0 && heightData[i] <= 1);
    }

    stbi_image_free(imageData);

    computeSobelGradient();

    return true;
}

float Heightmap::getHeight(int x, int y) const {
    if(x<0 || x>=width || y<0 || y>=height)
        return 0.0f;
    return heightData[y*width + x];
}

float Heightmap::getGradient(int x,int y) const {
    if(x<0 || x>=width || y<0 || y>=height)
        return 0.0f;
    return gradientMagnitude[y*width + x];
}

void Heightmap::computeSobelGradient(){
    gradientMagnitude.assign(width * height, 0.0f);

    int sobelX[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int sobelY[3][3] = {
        {-1,-2,-1},
        { 0, 0, 0},
        { 1, 2, 1}
    };

    for(int y=1; y<height-1; y++){
        for(int x=1; x<width-1; x++){
            float gx=0.0f, gy=0.0f;
            for(int dy=-1;dy<=1;dy++){
                for(int dx=-1;dx<=1;dx++){
                    float val = heightData[(y+dy)*width + (x+dx)];
                    gx += sobelX[dy+1][dx+1]*val;
                    gy += sobelY[dy+1][dx+1]*val;
                }
            }

            gradientMagnitude[y * width + x] = std::sqrt(gx*gx + gy*gy);
        }
    }
}

float Heightmap::bilinearInterpolate(float x, float y) const {
    // x, y in [0, 1] range
    float px = x * (width - 1);
    float py = y * (height - 1);
    
    int x0 = (int)px;
    int y0 = (int)py;
    int x1 = std::min(x0 + 1, width - 1);
    int y1 = std::min(y0 + 1, height - 1);
    
    float fx = px - x0;
    float fy = py - y0;
    
    float h00 = getHeight(x0, y0);
    float h10 = getHeight(x1, y0);
    float h01 = getHeight(x0, y1);
    float h11 = getHeight(x1, y1);
    
    float h0 = h00 * (1 - fx) + h10 * fx;
    float h1 = h01 * (1 - fx) + h11 * fx;
    
    return h0 * (1 - fy) + h1 * fy;
}

std::vector<glm::vec3> Heightmap::getFourCorners() const {
    std::vector<glm::vec3> corners;
    
    // The four normalized (u, v) corners of the map
    float u_coords[4] = {0.0f, 1.0f, 1.0f, 0.0f};
    float v_coords[4] = {0.0f, 0.0f, 1.0f, 1.0f};

    for (int i = 0; i < 4; i++) {
        float u = u_coords[i];
        float v = v_coords[i];

        // 1. Convert normalized [0,1] to World Space [X, Z]
        float worldX = (u - 0.5f) * widthExtent;
        float worldZ = (v - 0.5f) * depthExtent;

        // 2. Sample true Y from the heightmap image
        // bilinearInterpolate returns 0.0 to 1.0, so multiply by maxHeight
        float worldY = bilinearInterpolate(u, v) * maxHeight;

        corners.push_back(glm::vec3(worldX, worldY, worldZ));
    }

    return corners;
}

std::vector<glm::vec3> Heightmap::uniformSample(int numSamples) {
    std::vector<glm::vec3> sampledPoints;
    
    int pointsPerAxis = std::round(std::sqrt(numSamples));
    
    if (pointsPerAxis < 2) {
        pointsPerAxis = 2; 
    }

    sampledPoints.reserve(pointsPerAxis * pointsPerAxis);

    for (int z = 0; z < pointsPerAxis; ++z) {
        for (int x = 0; x < pointsPerAxis; ++x) {
            
            float normalizedX = (float)x / (pointsPerAxis - 1);
            float normalizedZ = (float)z / (pointsPerAxis - 1);

            float worldX = (normalizedX - 0.5f) * widthExtent;
            float worldZ = (normalizedZ - 0.5f) * depthExtent;

            float heightSample = bilinearInterpolate(normalizedX, normalizedZ);
            float worldY = heightSample * maxHeight;

            sampledPoints.push_back(glm::vec3(worldX, worldY, worldZ));
        }
    }
    
    return sampledPoints;
}

std::vector<glm::vec3> Heightmap::importanceSample(int numSamples) {
    // 1. Normalize gradient and build CDF
    float maxGrad = *std::max_element(gradientMagnitude.begin(), gradientMagnitude.end());
    if (maxGrad == 0.0f) maxGrad = 1.0f;
    
    std::vector<float> probabilities(width * height);
    for (int i = 0; i < width * height; i++) {
        probabilities[i] = gradientMagnitude[i] / maxGrad;
    }
    
    std::vector<float> cdf(width * height + 1, 0.0f);
    for (int i = 0; i < width * height; i++) {
        cdf[i + 1] = cdf[i] + probabilities[i];
    }
    
    // 2. Sample without Replacement
    std::vector<glm::vec3> sampledPoints;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    std::vector<bool> visited(width * height, false);
    
    int actual_samples = 0;
    int max_attempts = numSamples * 20;
    int attempts = 0;

    while (actual_samples < numSamples && attempts < max_attempts) {
        attempts++;
        float u = dist(rng) * cdf[width * height];
        
        int idx = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin() - 1;
        idx = std::max(0, std::min(idx, width * height - 1));
        
        if (visited[idx]) continue; 
        
        // Mark as visited and increment our valid sample count
        visited[idx] = true;
        actual_samples++;

        // Convert to world coordinates
        int py = idx / width; 
        int px = idx % width; 

        float normalizedX = (px + 0.5f) / width;
        float normalizedZ = (py + 0.5f) / height;

        float worldX = (normalizedX - 0.5f) * widthExtent;
        float worldZ = (normalizedZ - 0.5f) * depthExtent;
        
        float heightSample = bilinearInterpolate(normalizedX, normalizedZ);
        float worldY = heightSample * maxHeight;
        
        sampledPoints.push_back(glm::vec3(worldX, worldY, worldZ));
    }
    
    if (actual_samples < numSamples) {
        std::cout << "Warning: Could only find " << actual_samples << " unique points!\n";
    }

    return sampledPoints;
}

std::vector<glm::vec3> Heightmap::generateTestTerrain(int gridWidth, int gridDepth) {
    std::vector<glm::vec3> points;
    
    for (int x = 0; x < gridWidth; ++x) {
        for (int z = 0; z < gridDepth; ++z) {
            float px = (x / (float)(gridWidth - 1) - 0.5f) * widthExtent;
            float pz = (z / (float)(gridDepth - 1) - 0.5f) * depthExtent;
            
            float py = 0.5f * std::sin(px * 1.5f) * std::cos(pz * 1.5f)        // Base rolling hills
                     + 0.2f * std::sin(px * 3.5f + 1.0f) * std::cos(pz * 3.5f) // Mid-frequency details
                     + 0.1f * std::sin(px * 8.0f);                             // High-frequency ridges
            
            points.push_back(glm::vec3(px, py, pz));
        }
    }
    return points;
}