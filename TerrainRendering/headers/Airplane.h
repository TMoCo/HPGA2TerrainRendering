//
// Plane class consisting of plane model, texture and a camera
//

#ifndef AIRPLANE_H
#define AIRPLANE_H

#include <vulkan_help/VulkanSetup.h>

#include <utils/Model.h>
#include <utils/Texture.h>
#include <utils/Camera.h>

// POD
class Airplane {

public:
	void createPlane(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, const glm::vec3& pos, const float& vel = 10.0f);

	void updatePosition(const float& deltaTime);

	Model model;
	Texture texture;
	Camera camera;

	glm::vec3 position;

	float velocity;
};


#endif // !AIRPLANE_H
