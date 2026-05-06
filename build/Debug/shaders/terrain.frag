#version 330 core

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec3 vertexColor;

uniform vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 ambientColor = vec3(0.3, 0.3, 0.3);
uniform vec3 viewPos;

out vec4 FragColor;

void main() {
    // Phone stuffs
    float diffuseReflection = 0.7;
    float specularReflection = 0.3;
    float specularShininess = 3.0;

    vec3 normal = normalize(vertexNormal);
    vec3 viewDir = normalize(viewPos - vertexPosition);
    
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    // Specular
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularShininess);
    vec3 specular = spec * lightColor * specularReflection;

    vec3 finalColor = (ambientColor + diffuse * lightColor * diffuseReflection + specular) * vertexColor;
    
    FragColor = vec4(finalColor, 1.0);
}
