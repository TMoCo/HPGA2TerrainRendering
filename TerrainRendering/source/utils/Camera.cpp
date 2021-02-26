//
// Camera class definition
//

#include <utils/Camera.h> // the camera class

#include <glm/gtx/quaternion.hpp>

// default constructor initialises the class variables
Camera::Camera() {
    // camera specific vectors
    position = glm::vec3(0.0f, 0.0f, 0.0f);
    // start with no rotation to the camera
    pitch    = 0.0f;
    roll     = 0.0f;
    yaw      = 0.0f;
    updateCamera();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position, position + front, up); // camera pos, view dir, camera up
}

glm::mat4 Camera::getRotation() {
    glm::mat4 rotation(1.0f);
    // the rotation matrix based on the camera's normalised local axes and global axes
    rotation[0] = glm::vec4(right, 0.0f);
    rotation[1] = glm::vec4(up, 0.0f);
    rotation[2] = glm::vec4(front, 0.0f);
    return rotation;
}

void Camera::processInput(CameraMovement camMove, float deltaTime) {
    pitch = roll = yaw = 0.0f; // reset pitch yaw and roll
    if (camMove == CameraMovement::PitchUp)
        pitch = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::PitchDown)
        pitch = -CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::RollLeft)
        roll = -CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::RollRight)
        roll = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::YawLeft)
        yaw = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::YawRight)
        yaw = -CAMERA_ANGLE * deltaTime;
    // update the camera accordingly
    updateCamera();
}

void Camera::updateCamera() {
    // change the camera's direction and up axes based on the pitch, yaw and roll aquired from user
    front = glm::normalize(glm::angleAxis(glm::radians(pitch), right) * glm::angleAxis(glm::radians(yaw), up) * front);
    up    = glm::normalize(glm::angleAxis(glm::radians(roll), front) * up);
    // compute the right axis using the newly computed up and front axes
    right = glm::normalize(glm::cross(front, up));
}