//
// Camera class definition
//

#include <utils/Camera.h> // the camera class

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

// default constructor initialises the class variables
Camera::Camera() {
    position_ = glm::vec3(0.0f, 0.0f, 0.0f);
    // start with no rotation to the camera
    pitch_ = yaw_ = roll_ = 0.0f;
    updateCamera();
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(position_, position_ + orientation_.front, orientation_.up); // look at in front of the camera
}

glm::mat4 Camera::getViewMatrix(const glm::vec3& pos) {
    return glm::lookAt(position_, glm::normalize(pos - position_), orientation_.up); // look at a certain point
}

Orientation Camera::getOrientation() {
    return orientation_;
}

void Camera::processInput(CameraMovement camMove, float deltaTime) {
    pitch_ = roll_ = yaw_ = 0.0f; // reset pitch yaw and roll
    if (camMove == CameraMovement::PitchUp)
        pitch_ = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::PitchDown)
        pitch_ = -CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::RollRight)
        roll_ = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::RollLeft)
        roll_ = -CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::YawLeft)
        yaw_ = CAMERA_ANGLE * deltaTime;
    if (camMove == CameraMovement::YawRight)
        yaw_ = -CAMERA_ANGLE * deltaTime;
    // update the camera accordingly
    updateCamera();
}

void Camera::updateCamera() {
    // we need to update the camera's axes: get the rotation from the camera input
    glm::quat rotation = glm::angleAxis(glm::radians(yaw_), orientation_.up) * 
                         glm::angleAxis(glm::radians(pitch_), orientation_.right) *
                         glm::angleAxis(glm::radians(roll_), orientation_.front);
    // apply the rotation to the current orientation
    orientation_.applyRotation(rotation);
}