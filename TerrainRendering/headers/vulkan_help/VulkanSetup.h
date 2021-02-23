// 
// A helper class that creates an contains the vulkan data used in an application
// it aims to contain the vulkan structs that do not change during the lifetime of the
// application, such as the instance, the device, and the surface (although this is the case
// for single windowed applications, I assume support for multiple windows would require
// multiple surfaces). An application will have to use this class's member variables to
// run, the latter are NOT initiated when the object is created but when the setupVulkan function
// is called. A GLFW window needs to be initialised first and passed as an argument to the
// function so that vulkan can work with it. A reference of the window is kept as a pointer for convenience
//

#ifndef VULKAN_SETUP_H
#define VULKAN_SETUP_H

// constants and structs
#include "utils/Utils.h"

// vulkan definitions
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>

// glfw window library
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


class VulkanSetup {
    //////////////////////
    //
    // MEMBER FUNCTIONS
    //
    //////////////////////

public:

    //
    // Initiate and cleanup the setup
    //

    void initSetup(GLFWwindow* window);

    void cleanupSetup();

private:

    //
    // Vulkan instance setup
    //

	void createInstance();
	
	// enumerates the extensions required when creating a vulkan instance
	std::vector<const char*> getRequiredExtensions();

    //
    // Validation layers setup
    //

    // sets up the debug messenger when in debug mode
    void setupDebugMessenger();

    // finds where the debug messenger creator is located and calls it (if available)
    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger);

    // same as creator, but for destroying
    static void DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    bool checkValidationLayerSupport();

    //
    // Vulkan surface setup
    //

    void createSurface();
    
    //
    // Vulkan physical and logical device setup
    //

    void pickPhysicalDevice();

    bool isDeviceSuitable(VkPhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    bool checkDeviceExtensionSupport(VkPhysicalDevice device);

    void createLogicalDevice();
    
    //
    // Swap chain setup
    //

    void createSwapChain();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    //////////////////////
    //
    // MEMBER VARIABLES
    //
    //////////////////////

	// make them public for now
public:
    //
    // The window
    //

    // a reference to the GLFW window
    GLFWwindow* window;

    //
    // Instance
    //

	// vulkan instance struct
	VkInstance               instance;
    // a struct handle that manages debug callbacks
    VkDebugUtilsMessengerEXT debugMessenger;

    //
    // Surface
    //

    // the surface to render to
    VkSurfaceKHR surface;

    //
    // Device
    //

    // the physical device chosen for the application
    VkPhysicalDevice physicalDevice;
    // logical device that interfaces with the physical device
    VkDevice         device;
    // queue handle for interacting with the graphics queue, implicitly cleaned up by destroying devices
    VkQueue          graphicsQueue;
    // queue handle for interacting with the presentation queue
    VkQueue          presentQueue;

    //
    // Setup flag
    //

    bool setupComplete = false;

};

#endif // !VULKAN_SETUP_H

