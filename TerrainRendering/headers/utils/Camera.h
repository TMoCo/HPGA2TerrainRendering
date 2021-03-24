//
// Basic camera class
//

#ifndef CAMERA_H
#define CAMERA_H

#include <utils/Utils.h>
#include <utils/Orientation.h>

#include <glm/glm.hpp> // vectors, matrices
#include <glm/gtc/quaternion.hpp> // the quaternions

class Camera {

public: 
	Camera(glm::vec3 initPos = glm::vec3(0.0f), float initAngleSpeed = 20.0f, float initPosSpeed = 0.0f) :
		position_(initPos), angleChangeSpeed(initAngleSpeed), positionChangeSpeed(initPosSpeed) {
		// start with no rotation to the camera
		pitch_ = yaw_ = roll_ = 0.0f;
		updateCamera();
	}

	glm::mat4 getViewMatrix();

	glm::mat4 getViewMatrix(const glm::vec3& pos);

	Orientation getOrientation();

	void processInput(CameraMovement camMove, float deltaTime); // just keyborad input

	// update the camera's orientation
	void updateCamera();

	// camera position
	glm::vec3 position_;

	// camera's orientation
	Orientation orientation_;

	// start with no rotation to the camera
	float	  pitch_;
	float	  roll_;
	float	  yaw_;

	// the camera angle change speed
	float angleChangeSpeed;
	float positionChangeSpeed;
};


#endif // !CAMERA_H
