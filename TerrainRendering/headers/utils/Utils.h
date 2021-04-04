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

//
// App constants
//

// constants for window dimensions
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

// strings for the vulkan instance
const std::string APP_NAME    = "Terrain rendering";
const std::string ENGINE_NAME = "No Engine";

// max size for reading a line 
const std::streamsize MAX_SIZE = 1048;

// world axes
const glm::vec3 WORLD_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
const glm::vec3 WORLD_UP    = glm::vec3(0.0f, 1.0f, 0.0f);
const glm::vec3 WORLD_FRONT = glm::vec3(0.0f, 0.0f, 1.0f);

// paths to the model and texture
const std::string MODEL_PATH   = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\plane.obj";
const std::string TEXTURE_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\plane.jpg";
// paths to height maps for terrain
const std::string TERRAIN_HEIGHTS_PATHS[3] = {
    "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\HeightMap.png",
    "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\JuliaHeightMap.png",
    "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\assets\\Mt_Ruapehu_Mt_Ngauruhoe.png" };

//
// constants for vulkan
//

const size_t N_DESCRIPTOR_LAYOUTS = 2;

// paths to the fragment and vertex shaders used

// vertex shaders
const std::string TERRAIN_SHADER_VERT_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\terrainVert.spv";
const std::string TERRAIN_GPU_SHADER_VERT_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\terrainGPUVert.spv";
const std::string AIRPLANE_SHADER_VERT_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\airplaneVert.spv";

// fragment shaders
const std::string TERRAIN_SHADER_FRAG_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\terrainFrag.spv";
const std::string AIRPLANE_SHADER_FRAG_PATH = "C:\\Users\\Tommy\\Documents\\COMP4\\5822HighPerformanceGraphics\\A2\\HPGA2TerrainRendering\\TerrainRendering\\source\\shaders\\airplaneFrag.spv";

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


//////////////////////
//
// Debug preprocessor
//
//////////////////////

#define DEBUG // uncomment to remove validation layers and stop debug
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
// Utility enums
//
//////////////////////

// an enum that abstracts away camera input processing
enum class CameraMovement : unsigned char {
    PitchUp   = 0x00,
    PitchDown = 0x10,
    RollLeft  = 0x20,
    RollRight = 0x30,
    YawLeft   = 0x40,
    YawRight  = 0x50,
    Right     = 0x60,
    Left      = 0x70,
    Forward   = 0x80,
    Backward  = 0x90,
    Upward    = 0xA0,
    Downward  = 0xB0
};

//////////////////////
//
// Utility structs
//
//////////////////////

// a struct that contains a vk buffer and its memory
struct BufferData {
    VkBuffer       buffer = nullptr;
    VkDeviceMemory memory = nullptr;

    inline void cleanupBufferData(const VkDevice& device) {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }
};

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

// a POD struct for creating a buffer
struct BufferCreateInfo {
    VkDeviceSize          size       = 0;
    VkBufferUsageFlags    usage      = 0;
    VkMemoryPropertyFlags properties = 0;
    BufferData*           pBufferData {};
};

// a POD struct for copying a buffer into another
struct BufferCopyInfo {
    VkBuffer*    pSrc       = nullptr;
    VkBuffer*    pDst       = nullptr;
    VkBufferCopy copyRegion {};
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

    void createBuffer(const VkDevice* device, const VkPhysicalDevice* physicalDevice, BufferCreateInfo* bufferCreateInfo);

    void copyBuffer(const VkDevice* device, const VkQueue* queue, const VkCommandPool& commandPool, BufferCopyInfo* bufferCopyInfo);
}

#endif // !UTILS_H
