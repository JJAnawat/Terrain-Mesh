#include "Renderer.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

Renderer::Renderer()
    :   viewMatrix(glm::mat4(1.0f)),
        projMatrix(glm::mat4(1.0f)),
        modelMatrix(glm::mat4(1.0f))
{
    shaderProgram = glCreateProgram();

    std::string vertSrc = loadShader("shaders/terrain.vert");
    std::string fragSrc = loadShader("shaders/terrain.frag");

    unsigned int vertexShader = compileShader(vertSrc, 1);
    unsigned int fragmentShader = compileShader(fragSrc, 2);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    VAO = VBO = normalVBO = EBO = 0;
    indexCount = 0;
}

Renderer::~Renderer(){
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &normalVBO);
    glDeleteBuffers(1, &colorVBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);
}

void Renderer::uploadMesh(
    const std::vector<glm::vec3>& vertices,
    const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec3>& colors,
    const std::vector<unsigned int>& indices){
        
    if (!VAO){
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
    
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glGenBuffers(1, &normalVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        
        glGenBuffers(1, &colorVBO);
        glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        // Position
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
        glEnableVertexAttribArray(0);
        
        // Normal
        glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
        glEnableVertexAttribArray(1);
        
        // Color
        glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*) 0);
        glEnableVertexAttribArray(2);
        
        glBindVertexArray(0);
    }

    // Position
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_DYNAMIC_DRAW);
    
    // Normals
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_DYNAMIC_DRAW);
    
    // Colors
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(glm::vec3), colors.data(), GL_DYNAMIC_DRAW);
    
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_DYNAMIC_DRAW);

    this->indexCount = indices.size();
}

void Renderer::setViewMatrix(const glm::mat4& view){
    viewMatrix = view;
}

void Renderer::setProjMatrix(const glm::mat4& proj){
    projMatrix = proj;
}

void Renderer::setModelMatrix(const glm::mat4& model){
    modelMatrix = model;
}

void Renderer::setViewPos(const glm::vec3& pos){
    viewPos = pos;
}

void Renderer::setUniformMatrix4fv(const std::string& name, const glm::mat4& matrix){
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    glUniformMatrix4fv(loc, 1, GL_FALSE, &matrix[0][0]);
}

void Renderer::setUniformVector3fv(const std::string& name, const glm::vec3& vec){
    GLint loc = glGetUniformLocation(shaderProgram, name.c_str());
    glUniform3fv(loc, 1, glm::value_ptr(vec));
}

void Renderer::draw(){
    glUseProgram(shaderProgram);

    setUniformMatrix4fv("view", viewMatrix);
    setUniformMatrix4fv("proj", projMatrix);
    setUniformMatrix4fv("model", modelMatrix);
    setUniformVector3fv("viewPos", viewPos);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
}

std::string Renderer::loadShader(const std::string& filepath){
    std::ifstream file(filepath);
    if(!file.is_open())
        throw std::runtime_error("Could not open shader: " + filepath);
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );  
}

unsigned int Renderer::compileShader(const std::string& source, unsigned int type){
    unsigned int shader;
    shader = glCreateShader((type==1 ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER));
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::string shaderType = (type == 1) ? "VERTEX" : "FRAGMENT";
        std::cerr << "ERROR::SHADER::" << shaderType << "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

