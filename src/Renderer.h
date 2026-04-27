#ifndef RENDERER_H
#define RENDERER_H

#include <string>
#include <glm/glm.hpp>

class Renderer{
    public:
        Renderer();
        ~Renderer();

        void uploadMesh(
            const std::vector<glm::vec3>& vertices, 
            const std::vector<glm::vec3>& normals,
            const std::vector<glm::vec3>& colors,
            const std::vector<unsigned int>& indices    
        );
        void draw();

        void setViewMatrix(const glm::mat4& view);
        void setProjMatrix(const glm::mat4& proj);
        void setModelMatrix(const glm::mat4& model);

    private:
        unsigned int shaderProgram;
        unsigned int VAO, VBO, normalVBO, colorVBO, EBO;
        size_t indexCount;

        glm::mat4 viewMatrix, projMatrix, modelMatrix;

        std::string loadShader(const std::string& filepath);
        unsigned int compileShader(const std::string& source, unsigned int type);
        void setUniformMatrix4fv(const std::string& name, const glm::mat4& matrix);
};

#endif