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
	Camera();

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

};


#endif // !CAMERA_H
