#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <glm/glm.hpp>

class Renderer{
    public:
        Renderer();
        ~Renderer();

        void uploadMesh(const float* vertices, int vertexCount, const unsigned int* indices, int indexCount);
        void draw();

        void setViewMatrix(const glm::mat4& view);
        void setProjMatrix(const glm::mat4& proj);
        void setModelMatrix(const glm::mat4& model);

    private:
        unsigned int shaderProgram;
        unsigned int VAO, VBO, EBO;
        int indexCount;

        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::mat4 modelMatrix;

        std::string loadShader(const std::string& filepath);
        unsigned int compileShader(const std::string& source, unsigned int type);
        void setUniformMatrix4fv(const std::string& name, const glm::mat4& matrix);
};

#endif