//
// A class for saving an orientation, could add more code later for lerping etc...
//

#ifndef ORIENTATION_H
#define ORIENTATION_H

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <utils/Utils.h>

class Orientation {
public:
	Orientation() : front(WORLD_FRONT), up(WORLD_UP), right(WORLD_RIGHT) {}

	inline void applyRotation(const glm::quat& rotation) {
		front = glm::normalize(rotation * front);
		up	  = glm::normalize(rotation * up);
		right = glm::cross(front, up);
	}

	inline void rotateToOrientation(const Orientation& targetOrientation) {
		// get the target orientation as a matrix
		glm::mat3 targetAsMatrix;
		targetAsMatrix[0] = targetOrientation.right; // x
		targetAsMatrix[1] = targetOrientation.up;    // y
		targetAsMatrix[2] = targetOrientation.front; // z

		// rotation to the other orientation is the inverse of the other orientation matrix (so transpose)
		glm::mat3 rotation = glm::transpose(targetAsMatrix);

		// rotate to new orientation
		front = glm::normalize(front * rotation);
		up	  = glm::normalize(up * rotation);
		right = glm::cross(front, up);
	}

	inline glm::mat4 toWorldSpaceRotation() {
		// return the orientation as a rotation in world space 
		glm::mat4 rotation(1.0f);
		rotation[0] = glm::vec4(right, 0.0f);
		rotation[1] = glm::vec4(up, 0.0f);
		rotation[2] = glm::vec4(front, 0.0f);
		return rotation;
	}
	
	// orientation axes (normalised vectors)
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;

};


#endif // !ORIENTATION_H