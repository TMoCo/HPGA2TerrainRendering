//
// A class that contains the swap chain setup and data. It has a create function
// to facilitate the recreation when a window is resized. It contains all the variables
// that depend on the VkSwapChainKHR object
//

#ifndef VULKAN_SWAP_CHAIN_H
#define VULKAN_SWAP_CHAIN_H

#include <vulkan_help/VulkanSetup.h> // for referencing the device
#include <vulkan_help/DepthResource.h> // for referencing the depth resource

#include <vector> // vector container

#include <vulkan/vulkan_core.h>


class SwapChainData {
    //////////////////////
    //
    // MEMBER FUNCTIONS
    //
    //////////////////////

public:

    //
    // Initiate and cleanup the swap chain
    //

    void initSwapChainData(VulkanSetup* pVkSetup, VkDescriptorSetLayout* descriptorSetLayout);

    void cleanupSwapChainData();

private:

    //
    // swap chain creation and helper functions
    //

    void createSwapChain();

    SwapChainSupportDetails querySwapChainSupport();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //
    // Image and image views creation
    //

    void createSwapChainImageViews();

    //
    // Render pass creation
    //

    void createRenderPass();

    void createImGuiRenderPass();

    //
    // Pipeline creation
    //

    void createTerrainPipeline(VkDescriptorSetLayout* descriptorSetLayout);
    
    void createAirplanePipeline(VkDescriptorSetLayout* descriptorSetLayout);

    //
    // Command pool creation
    //

    //////////////////////
    //
    // MEMBER VARIABLES
    //
    //////////////////////

public:
    // a reference to the vulkan setup (instance, devices, surface)
    VulkanSetup* vkSetup;

    //
    // The swap chain
    //

    VkSwapchainKHR           swapChain;
    // the swap chain images
    std::vector<VkImage>     images;
    // a vector containing the views needed to use images in the render pipeline
    std::vector<VkImageView> imageViews;
    // the image format
    VkFormat                 imageFormat;
    // the extent of the swap chain
    VkExtent2D               extent;
    // swap chain details obtained when creating the swap chain
    SwapChainSupportDetails  supportDetails;

    //
    // Pipeline
    //

    // the geometry render pass
    VkRenderPass     renderPass;
    // the ImGui render pass
    VkRenderPass     imGuiRenderPass;
    // the layout of the graphics pipeline, for binding descriptor sets
    VkPipelineLayout graphicsPipelineLayout;
    // the graphics pipeline
    VkPipeline       graphicsPipeline;

    //
    // Flags
    //

    bool enableDepthTest = true; // default
};

#endif // !VULKAN_SWAP_CHAIN_H
