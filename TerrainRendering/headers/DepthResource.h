//
// Class of depth resources for depth testing
//

#ifndef DEPTH_RESOURCE_H
#define DEPTH_RESOURCE_H

#include "VulkanSetup.h" // for referencing the device

#include <vulkan/vulkan_core.h>

class DepthResource {
    //////////////////////
    //
    // MEMBER FUNCTIONS
    //
    //////////////////////

public:

    //
    // Create and destroy the depth resource
    //
    
    void createDepthResource(const VulkanSetup* vkSetup, const VkExtent2D& extent, VkCommandPool commandPool);

    void cleanupDepthResource();

    //
    // Helper functions for creating the depth resource
    //

    static VkFormat findDepthFormat(const VulkanSetup* vkSetup);

    static VkFormat findSupportedFormat(const VulkanSetup* vkSetup, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    //////////////////////
    //
    // MEMBER VARIABLES
    //
    //////////////////////

public:
    VulkanSetup* VkSetup;
	
    // depth image
    VkImage depthImage;

    // the depth image's view for interfacing
    VkImageView depthImageView;

    // the memory containing the depth image data
    VkDeviceMemory depthImageMemory;
};


#endif // !DEPTH_RESOURCE_H