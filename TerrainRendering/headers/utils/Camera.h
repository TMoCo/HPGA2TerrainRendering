//
// Basic camera class
//

#ifndef CAMERA_H
#define CAMERA_H

#include <utils/Utils.h>

#include <glm/glm.hpp> // vectors, matrices
#include <glm/gtc/quaternion.hpp> // the quaternions

class Camera {

public: 
	Camera();

	glm::mat4 getViewMatrix();

	void processInput(CameraMovement camMove, float deltaTime); // just keyborad input
public: 
	// update the camera's orientation
	void updateCamera();

	// camera position
	glm::vec3 position;

	// camera axes
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

	// start with no rotation to the camera
	float	  pitch;
	float	  roll;
	float	  yaw;

};


#endif // !CAMERA_H
