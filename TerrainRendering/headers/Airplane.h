//
// Plane class consisting of plane model, texture and a camera
//

#ifndef AIRPLANE_H
#define AIRPLANE_H

#include <vulkan_help/VulkanSetup.h>

#include <utils/Model.h>
#include <utils/Texture.h>
#include <utils/Camera.h>

class Airplane {

public:
	void createAirplane(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, const glm::vec3& position, const float& vel = 10.0f);

	void destroyAirplane();

	void updatePosition(const float& deltaTime);

	// plane data
	Model	model_;
	Texture texture_;
	Camera	camera_;
	Orientation orientation_;

	float velocity_;
};


#endif // !AIRPLANE_H
