#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec3 vertexColor;

uniform vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 ambientColor = vec3(0.3, 0.3, 0.3);

out vec4 FragColor;

void main() {
    vec3 normal = normalize(vertexNormal);
    
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    vec3 finalColor = (ambientColor + diffuse * lightColor * 0.7) * vertexColor;
    
    FragColor = vec4(finalColor, 1.0);
}
