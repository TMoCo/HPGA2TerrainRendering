//
// Plane class definition
//

#include <Airplane.h>

void Airplane::createPlane(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, const glm::vec3& pos, const float& vel) {
	// set the plane's position and velocity
	position = pos;
	velocity = vel;
	// load in the plane data (vertices, texture)
	model.loadModel(MODEL_PATH);
	texture.createTexture(pVkSetup, TEXTURE_PATH, commandPool);
	// camera object is implicitly created by declaring it in class and using default constructor
	// still need to set the camera's position though!
	camera.position = position + glm::vec3(0.0f, 0.0f, 10.0f); // a little behind the plane
	camera.front	= -WORLD_FRONT;
	camera.right	= WORLD_RIGHT; 
	camera.up		= WORLD_UP;
}

void Airplane::updatePosition(const float& deltaTime) {
	glm::vec3 travelled = camera.front * deltaTime * velocity; // compute the distance travelled
	// update the camera and airplane's positions
	camera.position += travelled; 
	position		+= travelled;
}