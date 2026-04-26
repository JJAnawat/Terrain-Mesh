#version 330 core
layout(location = 0) in vec3 position;   // read from VBO slot 0

void main() {
    gl_Position = vec4(position, 1.0);   // just pass it straight through
}