//
// The class containing the application definition
//

#ifndef TERRAIN_APPLICATION_H
#define TERRAIN_APPLICATION_H

#include <Texture.h> // the texture class
#include <Model.h> // the model class
#include <Vertex.h> // the vertex struct
#include <VulkanSetup.h> // include the vulkan setup class
#include <SwapChainData.h> // the swap chain class
#include <FramebufferData.h> // the framebuffer data class
#include <Terrain.h>

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
// string for file name
#include <string>
// value wrapper
#include <optional>

//
// Helper structs
//

// a struct containing uniforms
struct UniformBufferObject {
    // matrices for scene rendering
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    // flag for setting the colour to texture coordinates
    int uvToRgb; 
    int hasAmbient;
    int hasDiffuse;
    int hasSpecular;
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

    void renderUI();

    //--------------------------------------------------------------------//

    void createDescriptorSetLayout();

    void createDescriptorPool();

    void createDescriptorSets();

    void createUniformBuffers();

    //--------------------------------------------------------------------//

    void createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags);

    void createCommandBuffers(std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool& commandPool);

    void recordGeometryCommandBuffer();

    //--------------------------------------------------------------------//
    
    void createVertexBuffer();

    void createIndexBuffer();

    //--------------------------------------------------------------------//

    void recreateVulkanData();

    //--------------------------------------------------------------------//

    void createSyncObjects();

    //--------------------------------------------------------------------//

    // the main loop
    void mainLoop();

    void drawFrame();

    void updateUniformBuffer(uint32_t currentImage);

    //--------------------------------------------------------------------//

    // destroys everything properly
    void cleanup();

    //--------------------------------------------------------------------//

    // functions to initiate the private members
    // init glfw window
    void initWindow();

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

// private members
private:
    // the window
    GLFWwindow* window;

    // the vulkan data (instance, surface, device)
    VulkanSetup vkSetup;

    // the swap chain and related data
    SwapChainData swapChainData;

    // the frame buffer and related data
    FramebufferData framebufferData;

    // the terrain object
    Terrain terrain;

    // object data
    Model planeModel;

    // texture data
    Texture planeTexture;

    // vertex buffer
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // index buffer
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    // uniform buffers
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;


    // Variables changed by the UI
    float translateX = 0.0f;
    float translateY = 0.0f;
    float translateZ = 0.0f;

    float rotateX = 0.0f;
    float rotateY = 0.0f;
    float rotateZ = 0.0f;

    float zoom = 1.0f;

    bool centreModel = false;

    bool enableDepthTest = true;
    bool uvToRgb = false;
    bool enableAlbedo = true;
    bool enableDiffuse = true;
    bool enableSpecular = true;


    // layout used to specify fragment uniforms, still required even if not used
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorPool descriptorPool;
    VkDescriptorPool imGuiDescriptorPool;
    std::vector<VkDescriptorSet> descriptorSets; // descriptor set handles
   

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


    // keep track of the current frame
    size_t currentFrame = 0;
    // the index of the image retrieved from the swap chain
    uint32_t imageIndex;
    // resize window flag
    bool framebufferResized = false;
};

#endif

