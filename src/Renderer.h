#ifndef RENDERER_H
#define RENDERER_H

#include <string>

class Renderer{
    public:
        Renderer();
        ~Renderer();

        void uploadMesh(const float* vertices, int vertexCount, const unsigned int* indices, int indexCount);
        void draw();

    private:
        unsigned int shaderProgram;
        unsigned int VAO, VBO, EBO;
        int indexCount;

        std::string loadShader(const std::string& filepath);
        unsigned int compileShader(const std::string& source, unsigned int type);
};

#endif