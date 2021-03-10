//
// Definition of the SwapChain class
//

#include <utils/Vertex.h>

#include <vulkan_help/SwapChainData.h>// include the class declaration
#include <vulkan_help/Shader.h>// include the shader struct 

// exceptions
#include <iostream>
#include <stdexcept>

//////////////////////
//
// INITIALISATION AND DESTRUCTION
//
//////////////////////

void SwapChainData::initSwapChainData(VulkanSetup* pVkSetup, VkDescriptorSetLayout* descriptorSetLayout) {
    // update the pointer to the setup data rather than passing as argument to functions
    vkSetup = pVkSetup;
    // create the swap chain
    createSwapChain();
    // then create the image views for the images created
    createSwapChainImageViews();
    // then the geometry render pass 
    createRenderPass();
    // and the ImGUI render pass
    createImGuiRenderPass();
    // followed by the graphics pipeline
    createTerrainPipeline(&descriptorSetLayout[0]);
    createAirplanePipeline(&descriptorSetLayout[1]);
}

void SwapChainData::cleanupSwapChainData() {
    // destroy pipeline and related data
    vkDestroyPipeline(vkSetup->device, terrainPipeline, nullptr);
    vkDestroyPipelineLayout(vkSetup->device, terrainPipelineLayout, nullptr);

    // destroy the render passes
    vkDestroyRenderPass(vkSetup->device, renderPass, nullptr);
    vkDestroyRenderPass(vkSetup->device, imGuiRenderPass, nullptr);

    // loop over the image views and destroy them. NB we don't destroy the images because they are implicilty created
    // and destroyed by the swap chain
    for (size_t i = 0; i < imageViews.size(); i++) {
        vkDestroyImageView(vkSetup->device, imageViews[i], nullptr);
    }

    // destroy the swap chain proper
    vkDestroySwapchainKHR(vkSetup->device, swapChain, nullptr);
}

//////////////////////
//
// The swap chain
//
//////////////////////

void SwapChainData::createSwapChain() {
    // create the swap chain by checking for swap chain support
    supportDetails = querySwapChainSupport();

    // set the swap chain properties using the above three methods for the format, presentation mode and capabilities
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(supportDetails.presentModes);
    VkExtent2D newExtent = chooseSwapExtent(supportDetails.capabilities);

    // the number of images we want to put in the swap chain, at least one more image than minimum so we don't have to wait for 
    // driver to complete internal operations before acquiring another image
    uint32_t imageCount = supportDetails.capabilities.minImageCount + 1;

    // also make sure not to exceed the maximum image count
    if (supportDetails.capabilities.maxImageCount > 0 && imageCount > supportDetails.capabilities.maxImageCount) {
        imageCount = supportDetails.capabilities.maxImageCount;
    }

    // start creating a structure for the swap chain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vkSetup->surface; // specify the surface the swap chain should be tied to
    // set details
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = newExtent;
    createInfo.imageArrayLayers = 1; // amount of layers each image consists of
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // bit field, specifies types of operations we'll use images in swap chain for

    // how to handle the swap chain images across multiple queue families (in case graphics queue is different to presentation queue)
    QueueFamilyIndices indices = QueueFamilyIndices::findQueueFamilies(vkSetup->physicalDevice, vkSetup->surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    // if the queues differ
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // image owned by one queue family, ownership must be transferred explicilty
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // images can be used accross queue families with no explicit transfer
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    // a certain transform to apply to the image
    createInfo.preTransform = supportDetails.capabilities.currentTransform;
    // specifiy if alpha channel should be blending with other windows
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode; // determined earlier
    createInfo.clipped = VK_TRUE; // ignore colour of obscured pixels
    // in case the swap chain is no longer optimal or invalid (if window was resized), need to recreate swap chain from scratch
    // and specify reference to old swap chain (ignore for now)
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    // finally create the swap chain
    if (vkCreateSwapchainKHR(vkSetup->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // get size of swap chain images
    vkGetSwapchainImagesKHR(vkSetup->device, swapChain, &imageCount, nullptr);
    // resize accordingly
    images.resize(imageCount);
    // pull the images
    vkGetSwapchainImagesKHR(vkSetup->device, swapChain, &imageCount, images.data());

    // save format and extent
    imageFormat = surfaceFormat.format;
    extent = newExtent;
}

SwapChainSupportDetails SwapChainData::querySwapChainSupport() {
    SwapChainSupportDetails details;
    // query the surface capabilities and store in a VkSurfaceCapabilities struct
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkSetup->physicalDevice, vkSetup->surface, &details.capabilities); // takes into account device and surface when determining capabilities

    // same as we have seen many times before
    uint32_t formatCount;
    // query the available formats, pass null ptr to just set the count
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkSetup->physicalDevice, vkSetup->surface, &formatCount, nullptr);

    // if there are formats
    if (formatCount != 0) {
        // then resize the vector accordingly
        details.formats.resize(formatCount);
        // and set details struct fromats vector with the data pointer
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkSetup->physicalDevice, vkSetup->surface, &formatCount, details.formats.data());
    }

    // exact same thing as format for presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkSetup->physicalDevice, vkSetup->surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkSetup->physicalDevice, vkSetup->surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR SwapChainData::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // VkSurfaceFormatKHR entry contains a format and colorSpace member
    // format is colour channels and type eg VK_FORMAT_B8G8R8A8_SRGB (8 bit uint BGRA channels, 32 bits per pixel)
    // colorSpace is the coulour space that indicates if SRGB is supported with VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (used to be VK_COLORSPACE_SRGB_NONLINEAR_KHR)

    // loop through available formats
    for (const auto& availableFormat : availableFormats) {
        // if the correct combination of desired format and colour space exists then return the format
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    // if above fails, we could rank available formats based on how "good" they are for our task, settle for first element for now 
    return availableFormats[0];
}

VkPresentModeKHR SwapChainData::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // presentation mode, can be one of four possible values:
    // VK_PRESENT_MODE_IMMEDIATE_KHR -> image submitted by app is sent straight to screen, may result in tearing
    // VK_PRESENT_MODE_FIFO_KHR -> swap chain is a queue where display takes an image from front when display is refreshed. Program inserts rendered images at back. 
    // If queue full, program has to wait. Most similar vsync. Moment display is refreshed is "vertical blank".
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR -> Mode only differs from previous if application is late and queue empty at last vertical blank. Instead of waiting for next vertical blank, 
    // image is transferred right away when it finally arrives, may result tearing.
    // VK_PRESENT_MODE_MAILBOX_KHR -> another variation of second mode. Instead of blocking the app when queue is full, images that are already queued are replaced with newer ones.
    // Can be used to implement triple buffering, which allows to avoid tearing with less latency issues than standard vsync using double buffering.

    for (const auto& availablePresentMode : availablePresentModes) {
        // use triple buffering if available
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChainData::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    // swap extent is the resolution of the swap chain images, almost alwawys = to window res we're drawing pixels in
    // match resolution by setting width and height in currentExtent member of VkSurfaceCapabilitiesKHR struct.
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        // get the dimensions of the window
        int width, height;
        glfwGetFramebufferSize(vkSetup->window, &width, &height);

        // prepare the struct with the height and width of the window
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        // clamp the values between allowed min and max extents by the surface
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

        return actualExtent;
    }
}

//////////////////////
//
// The swap chain image views
//
//////////////////////

void SwapChainData::createSwapChainImageViews() {
    // resize the vector of views to accomodate the images
    imageViews.resize(images.size());
    
    // loop to iterate through images and create a view for each
    for (size_t i = 0; i < images.size(); i++) {
        // helper function for creating image views with a specific format
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images[i]; // different image, should refer to the texture
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // image as 1/2/3D textures and cube maps
        viewInfo.format = imageFormat;// how the image data should be interpreted
        // lets us swizzle colour channels around (here there is no swizzle)
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // describe what the image purpose is
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // and what part of the image we want
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        // attemp to create the image view
        VkImageView imageView;
        if (vkCreateImageView(vkSetup->device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        // and return the image view
        imageViews[i] = imageView;
    }
}

//////////////////////
//
// The render pass
//
//////////////////////

void SwapChainData::createRenderPass() {
    // need to tell vulkan about framebuffer attachments used while rendering
    // how many colour and depth buffers, how many samples for each and how contents
    // handled though rendering operations... Wrapped in render pass object
    VkAttachmentDescription colorAttachment{}; // attachment 
    colorAttachment.format = imageFormat; // colour attachment format should match swap chain images format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // > 1 for multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // what to do with data in attachment pre rendering
    // VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
    // VK_ATTACHMENT_LOAD_OP_CLEAR : Clear the values to a constant at the start
    // VK_ATTACHMENT_LOAD_OP_DONT_CARE : Existing contents are undefined; we don't care about them
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // post rendering
    // VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memoryand can be read later
    // VK_ATTACHMENT_STORE_OP_DONT_CARE : Contents of the framebuffer will be undefined after the rendering
    // above ops for colour, below for stencil data... not currenyly in use so just ignore
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // textures and framebuffers are VkImages with certain pixel formats, but layout in memory can change based on image use
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout before render pass begins (we don't care, not guaranteed to be preserved)
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout to transition to post render pass (image should be ready for drawing over by imgui)
    // common layouts
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
    // VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : Images to be presented in the swap chain
    // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : Images to be used as destination for a memory copy operation

    // specify a depth attachment to the render pass
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = DepthResource::findDepthFormat(vkSetup); // the same format as the depth image itself
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // like the colour buffer, we don't care about the previous depth contents so use layout undefined
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    // a single render pass consists of multiple subpasses, which are subsequent rendering operations depending on content of
    // framebuffers on previous passes (eg post processing). Grouping subpasses into a single render pass lets Vulkan optimise
    // every subpass references 1 or more attachments (see above) with structs:
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0; // forst attachment so refer to attachment index 0
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // which layout we want the attachment to have during a subpass

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1; // the second attachment
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // be explicit that this is a graphics subpass (Vulkan supports compute subpasses)
    // specify the reference to the colour attachment 
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef; // other types of attachments can also be referenced
    subpass.pDepthStencilAttachment = &depthAttachmentRef; // only a single depth/stencil attachment, no sense in depth tests on multiple buffers

    // subpass dependencies control the image layout transitions. They specify memory and execution of dependencies between subpasses
    // there are implicit subpasses right before and after the render pass
    // There are two built-in dependencies that take care of the transition at the start of the render pass and at the end, but the former 
    // does not occur at the right time as it assumes that the transition occurs at the start of the pipeline, but we haven't acquired the image yet 
    // there are two ways to deal with the problem:
    // - change waitStages of the imageAvailableSemaphore (in the drawframe function) to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT -> ensures that the
    // render pass does not start until image is available
    // - make the render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage
    VkSubpassDependency dependency{};
    // indices of the dependency and dependent subpasses, dstSubpass > srcSubpass at all times to prevent cycles in dependency graph
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // special refers to implicit subpass before or after renderpass 
    dependency.dstSubpass = 0; // index 0 refers to our subpass, first and only one
    // specify the operations to wait on and stag when ops occur
    // need to wait for swap chain to finish reading, can be accomplished by waiting on the colour attachment output stage
    // need to make sure there are no conflicts between transitionning og the depth image and it being cleared as part of its load operation
    // The depth image is first accessed in the early fragment test pipeline stage and because we have a load operation that clears, we should specify the access mask for writes.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    // ops that should wait are in colour attachment stage and involve writing of the colour attachment
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // now create the render pass, can be created by filling the structure with references to arrays for multiple subpasses, attachments and dependencies
    std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment }; // store the attachments
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data(); // the colour attachment for the renderpass
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass; // the associated supass
    // specify the dependency
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    // explicitly create the renderpass
    if (vkCreateRenderPass(vkSetup->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void SwapChainData::createImGuiRenderPass() {
    VkAttachmentDescription attachment = {};
    attachment.format = imageFormat;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // the initial layout is the image of scene geometry
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    info.attachmentCount = 1;
    info.pAttachments = &attachment;
    info.subpassCount = 1;
    info.pSubpasses = &subpass;
    info.dependencyCount = 1;
    info.pDependencies = &dependency;

    if (vkCreateRenderPass(vkSetup->device, &info, nullptr, &imGuiRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("Could not create Dear ImGui's render pass");
    }
}

//////////////////////
//
// The graphics pipeline
//
//////////////////////

void SwapChainData::createTerrainPipeline(VkDescriptorSetLayout* descriptorSetLayout) {
    auto vertShaderCode = Shader::readFile(TERRAIN_SHADER_VERT_PATH);
    auto fragShaderCode = Shader::readFile(TERRAIN_SHADER_FRAG_PATH);


    // compiling and linking of shaders doesnt happen until the pipeline is created, they are also destroyed along
    // with the pipeline so we don't need them to be member variables of the class
    VkShaderModule vertShaderModule = Shader::createShaderModule(vkSetup, vertShaderCode);
    VkShaderModule fragShaderModule = Shader::createShaderModule(vkSetup, fragShaderCode);

    // need to assign shaders to stages in the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // for the vertex shader, we'll asign it to the vertex stage
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    // set the vertex shader
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // the entry point, or the function to invoke in the shader
    // vertShaderStageInfo.pSpecializationInfo : can specify values for shader constants. Can use a singleshader module 
    // whose behaviour can be configured at pipeline creation by specifying different values for the constants used 
    // better than at render time because compiler can optimise if statements dependent on these values. Watch this space

    // similar gist as vertex shader for fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // assign to the fragment stage
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main"; // also use main as the entry point

    // use this array for future reference
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // setup pipeline to accept vertex data. For the terrain, these are just heights as floats
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0; // index of binding in array of bindings
    bindingDescription.stride = sizeof(float); // bytes in one entry
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // how to move the next data

    VkVertexInputAttributeDescription attributeDescription{};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = VK_FORMAT_R32_SFLOAT; // a single float value
    attributeDescription.offset = 0;

    // format of the vertex data, describe the binding and the attributes
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // because vertex data is in the shader, we don't have to specify anything here. We would otherwise
    // need arrays of structs that describe the details for loading vertex data
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    // tells what kind of geometry to draw from the vertices
    // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
    // VK_PRIMITIVE_TOPOLOGY_LINE_LIST : line from every 2 vertices without reuse
    // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP : end vertex of every line used as start vertex for next line
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
    // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : second and third vertex of every triangle used as first two vertices of next triangle
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE; // can break up lines and triangles in strip using special id 0xFFF or 0xFFFFFFF

    // the region of the framebuffer of output that will be rendered to ... usually always (0,0) to (width,height)
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width; // don't use WIDTH and HEIGHT as the swap chain may have differing values
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f; // in range [0,1]
    viewport.maxDepth = 1.0f; // in range [0,1]

    // viewport describes transform from image to framebuffer, but scissor rectangles defiles wich regions pixels are actually stored
    // pixels outisde are ignored by the rasteriser. Here the scissor covers the entire framebuffer
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    // once viewport and scissor have been defined, they need to be combined. Can use multiple viewports and scissors on some GPUs
    // (requires enqbling a GPU feature, so changes the logical device creation)
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    // rateriser takes geometry and turns it into fragments. Also performs depth test, face culling and scissor test
    // can be configured to output wireframe, full polygon 
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE; // if true,  fragments outside of near and far are clamped rather than discarded
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, disables passing geometry to framebuffer, so disables output
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // like opengl, can also be VK_POLYGON_MODE_LINE (wireframe) or VK_POLYGON_MODE_POINT
    // NB any other mode than fill requires enabling a GPU feature
    rasterizer.lineWidth = 1.0f; // larger than 1.0f requires the wideLines GPU feature
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // type of face culling to use (disable, cull front, cull back, cull both)
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // vertex order for faces to be front facing (CW and CCW)
    rasterizer.depthBiasEnable = VK_FALSE; // alter the depth by adding a constant or based onthe fragment's slope
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    // multisampling is a way to perform antialiasing, less expensive than rendering a high res poly then donwscaling
    // also requires enabling a GPU feature (in logical device creation) so disable for now
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    // could also set depth and stencil testing, but not needed now so it can stay a nullptr (already set
    // thanks to the {} when creating the struct)

    // after fragment shader has returned a result, needs to be combined with what is already in framebuffer
    // can either mix old and new values or combine with bitwise operation
    VkPipelineColorBlendAttachmentState colorBlendAttachment{}; // contains the config per attached framebuffer
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE; // disable because we don't want colour blending

    // this struct references array of structures for all framebuffers and sets constants to use as blend factors
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1; // the previously declared attachment
    colorBlending.pAttachments = &colorBlendAttachment;

    // enable and configure depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    // spec if the depth should be compared to the depth buffer
    if (enableDepthTest)
        depthStencil.depthTestEnable = VK_TRUE;
    else
        depthStencil.depthTestEnable = VK_FALSE;

    // specifies if the new depth that pass the depth test should be written to the buffer
    depthStencil.depthWriteEnable = VK_TRUE;
    // pecifies the comparison that is performed to keep or discard fragments (here lower depth = closer so keep less)
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    // boundary on the depth test, only keep fragments within the specified depth range
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    // stencil buffer operations
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional


    // create the pipeline layout, where uniforms are specified, also push constants another way of passing dynamic values
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // refernece to the descriptor layout (uniforms)
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayout;

    if (vkCreatePipelineLayout(vkSetup->device, &pipelineLayoutInfo, nullptr, &terrainPipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // now use all the structs we have constructed to build the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // reference the array of shader stage structs
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    // reference the structures describing the fixed function pipeline
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencil;
    // the pipeline layout is a vulkan handle rather than a struct pointer
    pipelineInfo.layout = terrainPipelineLayout;
    // and a reference to the render pass
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; // index of desired sub pass where pipeline will be used

    if (vkCreateGraphicsPipelines(vkSetup->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &terrainPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // destroy the shader modules, as we don't need them once the shaders have been compiled
    vkDestroyShaderModule(vkSetup->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkSetup->device, vertShaderModule, nullptr);
}

void SwapChainData::createAirplanePipeline(VkDescriptorSetLayout* descriptorSetLayout) {
    auto vertShaderCode = Shader::readFile(AIRPLANE_SHADER_VERT_PATH);
    auto fragShaderCode = Shader::readFile(AIRPLANE_SHADER_FRAG_PATH);

    // shader compilation
    VkShaderModule vertShaderModule = Shader::createShaderModule(vkSetup, vertShaderCode);
    VkShaderModule fragShaderModule = Shader::createShaderModule(vkSetup, fragShaderCode);

    // pipeline shaders stages
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main"; // main function as entry point
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT; 
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main"; // also use main as the entry point

    // use this array for future reference
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // setup pipeline to accept vertex data
    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    // vertex data format (binding and attributes)
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE; 

    // region of framebuffer rendered to
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width  = (float)extent.width; // swapchain's extent
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f; // in range [0,1]
    viewport.maxDepth = 1.0f; // in range [0,1]

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable        = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE; 
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL; 
    rasterizer.lineWidth               = 1.0f; // > 1 requires an extension
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT; 
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE; // winding order
    rasterizer.depthBiasEnable         = VK_FALSE; 

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable  = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{}; 
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE; 

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.attachmentCount = 1; // the previously declared attachment
    colorBlending.pAttachments    = &colorBlendAttachment;

    // enable and configure depth testing
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    // spec if the depth should be compared to the depth buffer
    depthStencil.depthTestEnable       = VK_TRUE;
    depthStencil.depthWriteEnable      = VK_TRUE;
    depthStencil.depthCompareOp        = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    // stencil buffer operations
    depthStencil.stencilTestEnable     = VK_FALSE;

    // create pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // reference to descriptor layout (uniforms, textures)
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts    = descriptorSetLayout;

    if (vkCreatePipelineLayout(vkSetup->device, &pipelineLayoutInfo, nullptr, &airplanePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // build the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // reference the array of shader stage structs
    pipelineInfo.stageCount = static_cast<uint32_t>(sizeof(shaderStages) / sizeof(VkPipelineShaderStageCreateInfo));
    pipelineInfo.pStages = shaderStages;

    // reference the structures describing the fixed function pipeline
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDepthStencilState  = &depthStencil;
    pipelineInfo.layout              = airplanePipelineLayout;
    // and a reference to the render pass
    pipelineInfo.renderPass          = renderPass;
    pipelineInfo.subpass             = 0; // index of desired sub pass where pipeline will be used

    if (vkCreateGraphicsPipelines(vkSetup->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &airplanePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // destroy the shader modules, as we don't need them once the shaders have been compiled
    vkDestroyShaderModule(vkSetup->device, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkSetup->device, vertShaderModule, nullptr);
}

