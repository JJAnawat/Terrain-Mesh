#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 vertexPosition;
out vec3 vertexNormal;
out vec3 vertexColor;

void main() {
    gl_Position = proj * view * model * vec4(position, 1.0);
    vertexPosition = vec3(model * vec4(position, 1.0));
    vertexNormal = mat3(transpose(inverse(model))) * normal;
    vertexColor = color;
}
