//
// The class containing the application definition
//

#ifndef TERRAIN_APPLICATION_H
#define TERRAIN_APPLICATION_H

#include <Airplane.h>

#include <utils/Texture.h> // the texture class
#include <utils/Model.h> // the model class
#include <utils/Vertex.h> // the vertex struct
#include <utils/Terrain.h> // terrain class

#include <vulkan_help/VulkanSetup.h> // include the vulkan setup class
#include <vulkan_help/SwapChainData.h> // the swap chain class
#include <vulkan_help/FramebufferData.h> // the framebuffer data class

// glfw window library
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// vectors, matrices ...
#include <glm/glm.hpp>

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>
// very handy containers of objects
#include <vector>
#include <array>
#include <string> // string for file name
#include <chrono> // time 

//
// Uniform structs
//

// POD for terrain uniforms
struct TerrainUBO {
    glm::vec4 lightPos;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 normal;
    float vertexStride;
    float heightScalar;
    int mapDim;
    float invMapDim;
};

// POD for airplane uniforms
struct AirplaneUBO {
    glm::vec4 lightPos;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 normal;
};

//
// The application
//

// program wrapped in class where vulkan objects are stored as private members
class TerrainApplication {

public:

    void run();

private:
    //--------------------------------------------------------------------//

    void initVulkan();

    //--------------------------------------------------------------------//

    void initImGui();

    void uploadFonts();

    //--------------------------------------------------------------------//
    // functions to initiate the private members
    // init glfw window
    void initWindow();

    //--------------------------------------------------------------------//

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSets();

    //--------------------------------------------------------------------//

    template<class T>
    void createUniformBuffer(BufferData* bufferData);

    void updateUniformBuffer(uint32_t currentImage);

    //--------------------------------------------------------------------//

    void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);

    void createCommandBuffers(std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool& commandPool);

    void recordGeometryCommandBuffer(size_t cmdBufferIndex);

    //--------------------------------------------------------------------//
    template<class T>
    void createVertexBuffer(std::vector<T>* vertices, BufferData* bufferData);
    void createIndexBuffer(std::vector<uint32_t>* indices, BufferData* bufferData);
    
    //--------------------------------------------------------------------//

    void recreateVulkanData();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    //--------------------------------------------------------------------//

    void createSyncObjects();

    //--------------------------------------------------------------------//

    // the main loop
    void mainLoop();

    //--------------------------------------------------------------------//
    
    void drawFrame();

    void setGUI();

    int processKeyInput();

    //--------------------------------------------------------------------//

    // destroys everything properly
    void cleanup();

    //--------------------------------------------------------------------//

// private members
private:
    // the window
    GLFWwindow* window;


    // the vulkan data (instance, surface, device)
    VulkanSetup     vkSetup;
    // the swap chain and related data
    SwapChainData   swapChainData;
    // the frame buffer and related data
    FramebufferData framebufferData;


    // the terrain object
    Terrain  terrain;
    // the airplane object
    Airplane airplane;

    // for... *drumroll* debugging
    Camera debugCamera;


    // scene vertex buffers
    BufferData bTerrainVertex;
    BufferData bAirplaneVertex;
    // scene index buffers
    BufferData bTerrainIndex;
    BufferData bAirplaneIndex;


    // uniform buffers
    BufferData bTerrainUniforms;
    BufferData bAirplaneUniforms;


    // descriptor data
    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<VkDescriptorSet> terrainDescriptorSets; // descriptor set handles
    std::vector<VkDescriptorSet> airplaneDescriptorSets; // descriptor set handles
    

    // command buffers
    VkCommandPool renderCommandPool;
    std::vector<VkCommandBuffer> renderCommandBuffers;

    // imgui command buffers
    VkCommandPool imGuiCommandPool;
    std::vector<VkCommandBuffer> imGuiCommandBuffers;


    // each frame has it's own semaphores
    // semaphores are for GPU-GPU synchronisation
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    // fences are for CPU-GPU synchronisation
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;


    // Variables changed by the UI
    float translateX = 0.0f;
    float translateY = 0.0f;
    float translateZ = 0.0f;

    float rotateX = 0.0f;
    float rotateY = 0.0f;
    float rotateZ = 0.0f;

    float scale = 1.0f;

    float vertexStride = 1.0f;
    float tolerance    = 0.5f;
    float heightScalar = 255.0f;

    int numChunks = 10;

    bool debugCameraState   = false; // on or off
    bool applyBinning       = true;
    bool shouldExit         = false;
    bool shouldLoadNewMap   = false;
    bool framebufferResized = false;
    bool onGPU              = true;

    int selectedMap = 2;
    int numMaps     = 0;
    
    std::string viewDir;

    // timer
    std::chrono::steady_clock::time_point prevTime;
    float deltaTime;


    // keep track of the current frame
    size_t currentFrame = 0;
    // the index of the image retrieved from the swap chain
    uint32_t imageIndex;
};

//
// Template function definitions
//

template<class T> // maybe a vector or vertex objects, or glm::vec3...
void TerrainApplication::createVertexBuffer(std::vector<T>* vertices, BufferData* bufferData) {
    // precompute buffer size
    VkDeviceSize bufferSize = sizeof(T) * vertices->size();

    // a staging buffer for mapping and copying 
    BufferData stagingBuffer;

    // buffer creation struct
    BufferCreateInfo createInfo{};
    createInfo.size = bufferSize;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createInfo.pBufferData = &stagingBuffer;

    // in host memory (cpu)
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, &createInfo);

    // map then copy in memory
    void* data;
    vkMapMemory(vkSetup.device, stagingBuffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, vertices->data(), (size_t)bufferSize);
    vkUnmapMemory(vkSetup.device, stagingBuffer.memory); // unmap the memory 

    // reuse the creation struct
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createInfo.pBufferData = bufferData;

    // in device memory (gpu)
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, &createInfo);

    // the struct used by VkCmdCopyBuffer
    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;

    // buffer copy struct
    BufferCopyInfo copyInfo{};
    copyInfo.pSrc = &stagingBuffer.buffer;
    copyInfo.pDst = &bufferData->buffer;
    copyInfo.copyRegion = copyRegion;

    utils::copyBuffer(&vkSetup.device, &vkSetup.graphicsQueue, renderCommandPool, &copyInfo);

    // cleanup after using the staging buffer
    stagingBuffer.cleanupBufferData(vkSetup.device);
}

template <class T>
void TerrainApplication::createUniformBuffer(BufferData* bufferData) {
    BufferCreateInfo createInfo{};
    createInfo.size = static_cast<VkDeviceSize>(swapChainData.images.size() * sizeof(T));
    createInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    // a large uniform buffer containing the data for each frame buffer
    createInfo.pBufferData = bufferData;
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, &createInfo);
}

#endif

