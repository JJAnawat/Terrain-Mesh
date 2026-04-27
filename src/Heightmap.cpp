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

    heightData.resize(width * height);
    for(int i=0;i<width * height; i++)
        heightData[i] = imageData[i] / 255.0f;

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

std::vector<glm::vec3> Heightmap::importanceSample(int numSamples) {
    // Normalize gradient to [0, 1]
    float maxGrad = *std::max_element(gradientMagnitude.begin(), gradientMagnitude.end());
    if (maxGrad == 0.0f) maxGrad = 1.0f;
    
    std::vector<float> probabilities(width * height);
    for (int i = 0; i < width * height; i++) {
        probabilities[i] = gradientMagnitude[i] / maxGrad;
    }
    
    // Build CDF
    std::vector<float> cdf(width * height + 1, 0.0f);
    for (int i = 0; i < width * height; i++) {
        cdf[i + 1] = cdf[i] + probabilities[i];
    }
    
    // Sample
    std::vector<glm::vec3> sampledPoints;
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    for (int s = 0; s < numSamples; s++) {
        float u = dist(rng) * cdf[width * height];
        
        int idx = std::lower_bound(cdf.begin(), cdf.end(), u) - cdf.begin() - 1;
        idx = std::max(0, std::min(idx, width * height - 1));
        
        int py = idx / width;
        int px = idx % width;
        
        // Jitter within pixel
        float jitterX = (dist(rng) - 0.5f) * 0.5f;
        float jitterY = (dist(rng) - 0.5f) * 0.5f;

        float normalizedX = (px + 0.5f + jitterX) / width;
        float normalizedZ = (py + 0.5f + jitterY) / height;

        float worldX = (normalizedX - 0.5f) * widthExtent;
        float worldZ = (normalizedZ - 0.5f) * depthExtent;
        
        float heightSample = bilinearInterpolate(normalizedX, normalizedZ);
        float worldY = heightSample * maxHeight;
        
        sampledPoints.push_back(glm::vec3(worldX, worldY, worldZ));
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