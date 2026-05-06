#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Default camera values
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.005f;
const float ZOOM        =  45.0f;

// An abstract camera class that processes input, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float initAzimuth = 0.0f, float initElevation = 0.3f);
    
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float initAzimuth=0.0f, float initElevation = 0.3f);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix() const;

    glm::mat4 getProjMatrix(float aspectRatio) const;

    // processes input received from a mouse input system.
    void ProcessMouseMovement(float xoffset, float yoffset);

    // processes input received from a mouse scroll-wheel event.
    void ProcessMouseScroll(float yoffset);

    // GLFW callbacks
    void onMouseButton(int button, int action);
    void onMouseMove(double x, double y);
    void onScroll(double yOffset);

    void printState() const;

private:
    // orbit cam
    float azimuth;
    float elevation;
    float distance;

    glm::vec3 center;

    // Mouse state
    double lastMouseX, lastMouseY;
    bool isLeftDragging, isRightDragging;
    
    // Constants
    const float MIN_ELEVATION = glm::pi<float>() * 0.05f;  // Clamp to avoid going below terrain
    const float MAX_ELEVATION = glm::pi<float>() * 0.45f;

    glm::vec3 computeEyePosition() const;

    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};
#endif