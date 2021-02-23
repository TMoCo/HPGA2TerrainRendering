//
// A convenience header file for constants etc. used in the application
//

#ifndef UTILS_H
#define UTILS_H

#include <stdint.h> // uint32_t

#include <vector> // vector container
#include <string> // string class
#include <optional> // optional wrapper
#include <ios> // streamsize

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>

#include <vulkan/vulkan_core.h> // vulkan core structs &c

// vectors, matrices ...
#include <glm/glm.hpp>

//////////////////////
//
// Constants
//
//////////////////////

// constants for window dimensions
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

const std::streamsize MAX_SIZE = 1048;

// strings for the vulkan instance
const std::string APP_NAME    = "Terrain rendering";
const std::string ENGINE_NAME = "No Engine";

// paths to the model and texture
const std::string MODEL_PATH   = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\plane.obj";
const std::string TEXTURE_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\plane.jpg";
const std::string TERRAIN_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\HeightMap.ppm";

// paths to the fragment and vertex shaders used
const std::string SHADER_VERT_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\vert.spv";
const std::string SHADER_FRAG_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\frag.spv";

// validation layers for debugging
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

// required device extensions
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// in flight frames number
const size_t MAX_FRAMES_IN_FLIGHT = 2;

// the ImGUI number of descriptor pools
const uint32_t IMGUI_POOL_NUM = 1000;

const glm::vec3 LIGHT_POS(0.0f, -30.0f, 50.0f);

//////////////////////
//
// Debug preprocessor
//
//////////////////////

//#define DEBUG // uncomment to remove validation layers and stop debug
#ifdef DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif

//#define VERBOSE
#ifdef VERBOSE
const bool enableVerboseValidation = true;
#else
const bool enableVerboseValidation = false;
#endif

//////////////////////
//
// Utility structs
//
//////////////////////

// a struct for the queue family index
struct QueueFamilyIndices {
    // queue family supporting drawing commands
    std::optional<uint32_t> graphicsFamily;
    // presentation of image to vulkan surface handled by the device
    std::optional<uint32_t> presentFamily;

    // returns true if the device supports the drawing commands AND the image can be presented to the surface
    inline bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
};

// a struct containing the details for support of a swap chain
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

// a POD struct containing the data for creating an image
struct CreateImageData {
    uint32_t              width       = 0;
    uint32_t              height      = 0;
    VkFormat              format      = VK_FORMAT_UNDEFINED;
    VkImageTiling         tiling      = VK_IMAGE_TILING_OPTIMAL;
    VkImageUsageFlags     usage       = VK_NULL_HANDLE;
    VkMemoryPropertyFlags properties  = VK_NULL_HANDLE;
    VkImage*              image       = nullptr;
    VkDeviceMemory*       imageMemory = nullptr;
};

// a POD struct containing the data for transitioning from one image layout to another
struct TransitionImageLayoutData {
    VkImage*      image             = nullptr;
    VkCommandPool renderCommandPool = VK_NULL_HANDLE;
    VkFormat      format            = VK_FORMAT_UNDEFINED;
    VkImageLayout oldLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageLayout newLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
};

//////////////////////
//
// Utility functions namespace
//
//////////////////////

namespace utils {

    //
    // Memory type 
    //

    uint32_t findMemoryType(const VkPhysicalDevice* physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    //
    // Begining and ending single use command buffers
    //

    VkCommandBuffer beginSingleTimeCommands(const VkDevice* device, const VkCommandPool& commandPool);

    void endSingleTimeCommands(const VkDevice* device, const VkQueue* queue, const VkCommandBuffer* commandBuffer, const VkCommandPool* commandPool);

    //
    // Image and image view creation
    //

    void createImage(const VkDevice* device, const VkPhysicalDevice* physicalDevice, const CreateImageData& info);

    VkImageView createImageView(const VkDevice* device, const VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags);

    //
    // Transitioning between image layouyts
    //

    bool hasStencilComponent(VkFormat format);

    void transitionImageLayout(const VkDevice* device, const VkQueue* graphicsQueue, const TransitionImageLayoutData& transitionData);

    //
    // Creating and copying buffers
    //

    void copyBufferToImage(const VkDevice* device, const VkQueue* queue, const VkCommandPool& renderCommandPool, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createBuffer(const VkDevice* device, const VkPhysicalDevice* physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
        VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    void copyBuffer(const VkDevice* device, const VkQueue* queue, const VkCommandPool& commandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
}

#endif // !UTILS_H
