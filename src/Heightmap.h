#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>
#include <string>
#include <glm/glm.hpp>

class Heightmap {
    public:
        Heightmap(float we = 4.0f, float de = 4.0f, float mh=1.0f);
        ~Heightmap();

        bool load(const std::string& filepath);

        int getWidth() const { return width; }
        int getHeight() const { return height; }
        float getHeight(int x, int y) const;
        float getGradient(int x, int y) const;
        float bilinearInterpolate(float x, float y) const;
        std::vector<glm::vec3> getFourCorners() const;
        
        std::vector<glm::vec3> uniformSample(int numSamples);
        std::vector<glm::vec3> importanceSample(int numSamples);
        std::vector<glm::vec3> generateTestTerrain(int gridWidth, int gridDepth);

        float widthExtent = 4.0f, depthExtent = 4.0f, maxHeight = 1.0f;
        int width, height;

    private:

        std::vector<float> heightData;
        std::vector<float> gradientMagnitude;

        void computeSobelGradient();
};

#endif