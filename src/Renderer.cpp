#include "Renderer.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

Renderer::Renderer(){
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

    VAO = VBO = EBO = 0;
    indexCount = 0;
}

Renderer::~Renderer(){
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderProgram);
}

void Renderer::uploadMesh(const float* vertices, int vertexCount, const unsigned int* indices, int indexCount){
    if (!VAO){
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);
    
        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
        glGenBuffers(1, &EBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*) 0);
        glEnableVertexAttribArray(0);
        
        glBindVertexArray(0);
    }

    glBindBuffer(GL_COPY_READ_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 3 * sizeof(float), vertices, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_DYNAMIC_DRAW);

    this->indexCount = indexCount;
}

void Renderer::draw(){
    glUseProgram(shaderProgram);
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

