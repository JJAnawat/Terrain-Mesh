#include "Camera.h"
#include <iostream>
#include <cmath>
#include <glm/gtc/constants.hpp>

// constructor with vectors
Camera::Camera(glm::vec3 position, glm::vec3 up, float initAzimuth, float initElevation) 
    :   azimuth(initAzimuth), elevation(initElevation), distance(5.0f),
        center(position), WorldUp(up),
        lastMouseX(0), lastMouseY(0),
        isLeftDragging(false), isRightDragging(false),
        MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    updateCameraVectors();
}

// constructor with scalar values
Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float initAzimuth, float initElevation) 
    :   azimuth(initAzimuth), elevation(initElevation), distance(5.0f),
        center(posX, posY, posZ), WorldUp(upX, upY, upZ),
        lastMouseX(0), lastMouseY(0),
        isLeftDragging(false), isRightDragging(false),
        MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    updateCameraVectors();
}

glm::vec3 Camera::computeEyePosition() const {
    float x = distance * std::sin(azimuth) * std::cos(elevation);
    float y = distance * std::sin(elevation);
    float z = distance * std::cos(azimuth) * std::cos(elevation);
    return center + glm::vec3(x, y, z);
}

// calculates the front vector from the Camera's
void Camera::updateCameraVectors()
{  
    Position = computeEyePosition();
    Front = glm::normalize(center - Position);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, center, Up);
}

glm::mat4 Camera::getProjMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(Zoom), aspectRatio, 0.01f, 1000.0f);
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::ProcessMouseMovement(float xoffset, float yoffset){
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    azimuth -= xoffset;
    elevation += yoffset;
    elevation = glm::clamp(elevation, MIN_ELEVATION, MAX_ELEVATION);

    updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::onMouseButton(int button, int action){
    if (button == 0){ // left button
        isLeftDragging = (action == 1);
    }
    if (button == 1){ // right button
        isRightDragging = (action == 1);
    }
}

void Camera::onMouseMove(double x, double y){
    double deltaX = x - lastMouseX;
    double deltaY = y - lastMouseY;

    if(isLeftDragging){
        ProcessMouseMovement(deltaX, -deltaY);
    }

    if(isRightDragging){
        // Pan
        glm::vec3 panRight = Right;
        glm::vec3 panUp = Up;
        
        float panSpeed = distance * 0.005f;
        center += panRight * (float)deltaX * panSpeed;
        center += panUp * (float)(-deltaY) * panSpeed;

        updateCameraVectors();
    }
    
    lastMouseX = x;
    lastMouseY = y;
}

void Camera::onScroll(double yOffset) {
    ProcessMouseScroll(yOffset);
}

void Camera::printState() const {
    std::cout << "Camera: az=" << azimuth << " el=" << elevation 
              << " dist=" << distance << " center=(" << center.x << ","
              << center.y << "," << center.z << ")\n";
}