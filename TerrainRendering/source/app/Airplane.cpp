//
// Plane class definition
//

#include <Airplane.h>

void Airplane::createPlane(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, const glm::vec3& position, const float& vel) {
	// set the plane's position and velocity
	velocity_ = vel;
	// load in the plane data (vertices, texture)
	model_.loadModel(MODEL_PATH);
	texture_.createTexture(pVkSetup, TEXTURE_PATH, commandPool);
	// camera object is implicitly created by declaring it in class and using default constructor
	// still need to set the camera's position though!
	camera_.position_ = position + glm::vec3(0.0f, 10.0f, 15.0f);
}

void Airplane::updatePosition(const float& deltaTime) {
	glm::vec3 travelled = camera_.orientation_.front * deltaTime * velocity_; // compute the distance travelled
	// update the camera and, and hence the airplane's, positions
	camera_.position_ += travelled; 
}