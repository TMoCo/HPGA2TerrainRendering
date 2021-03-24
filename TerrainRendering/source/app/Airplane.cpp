//
// Plane class definition
//

#include <Airplane.h>

void Airplane::createAirplane(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, const glm::vec3& position, const float& vel) {
	// set the plane's position and velocity
	velocity = vel;
	// load in the plane data (vertices, texture)
	model.loadModel(MODEL_PATH);
	texture.createTexture(pVkSetup, TEXTURE_PATH, commandPool, VK_FORMAT_R8G8B8A8_SRGB);
	// camera object is implicitly created by declaring it in class and using default constructor
	// still need to set the camera's position though!
	camera.position = position + glm::vec3(0.0f, 10.0f, 15.0f);
}

void Airplane::destroyAirplane() {
	texture.cleanupTexture();
}

void Airplane::updatePosition(const float& deltaTime) {
	glm::vec3 travelled = camera.orientation.front * deltaTime * velocity; // compute the distance travelled
	// update the camera and, and hence the airplane's, positions
	camera.position += travelled; 
}