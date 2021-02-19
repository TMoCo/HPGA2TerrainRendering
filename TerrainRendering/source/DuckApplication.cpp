// include class definition
#include <DuckApplication.h>

// include constants
#include <Utils.h>

// transformations
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // because OpenGL uses depth range -1.0 - 1.0 and Vulkan uses 0.0 - 1.0
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// time 
#include <chrono>

// min, max
#include <algorithm>

// file (shader) loading
#include <fstream>

// UINT32_MAX
#include <cstdint>

// set for queues
#include <set>


// ImGui includes for a nice gui
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

//////////////////////
//
// Run the application
//
//////////////////////

void DuckApplication::run() {
    // initialise a glfw window
    initWindow();

    // initialise vulkan
    initVulkan();

    // initialise imgui
    initImGui();

    // run the main loop
    mainLoop();

    // clean up before exiting
    cleanup();
}

//////////////////////
//
// Initialise vulkan
//
//////////////////////

void DuckApplication::initVulkan() {
    //
    // STEP 1: create the vulkan core 
    //

    vkSetup.initSetup(window);

    //
    // STEP 2: create the descriptor set layout(s) and command pool(s)
    //

    // create the descriptor set layout and render command pool BEFORE the swap chain
    // these do not change over the lifetime of the application
    createDescriptorSetLayout();
    createCommandPool(&renderCommandPool, 0);
    createCommandPool(&imGuiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    //
    // STEP 3: Swap chain and frame buffers
    //

    // create the swap chain
    swapChainData.initSwapChainData(&vkSetup, &descriptorSetLayout);
    // create the frame buffers
    framebufferData.initFramebufferData(&vkSetup, &swapChainData, renderCommandPool);

    //
    // STEP 4: Create the application's data (models, textures...)
    //

    // textures can go in a separate class
    duckTexture.createTexture(&vkSetup, TEXTURE_PATH, renderCommandPool);

    // model can go in a separate class
    duckModel.loadModel(MODEL_PATH);

    //
    // STEP 5: create the vulkan data for accessing and using the app's data
    //

    // these depend on the size of the framebuffer, so create them after 
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers(&renderCommandBuffers, renderCommandPool);
    createCommandBuffers(&imGuiCommandBuffers, imGuiCommandPool);
    // record the rendering command buffer once it has been created and the model has been loaded
    recordGemoetryCommandBuffer();

    //
    // STEP 6: setup synchronisation
    //
    createSyncObjects();
}

//////////////////////
//
// ImGui Initialisation
//
//////////////////////

void DuckApplication::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkSetup.instance;
    init_info.PhysicalDevice = vkSetup.physicalDevice;
    init_info.Device = vkSetup.device;
    init_info.QueueFamily = QueueFamilyIndices::findQueueFamilies(vkSetup.physicalDevice, vkSetup.surface).graphicsFamily.value();
    init_info.Queue = vkSetup.graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = swapChainData.supportDetails.capabilities.minImageCount + 1;
    init_info.ImageCount = static_cast<uint32_t>(swapChainData.images.size());

    // the imgui render pass
    ImGui_ImplVulkan_Init(&init_info, swapChainData.imGuiRenderPass);

    // upload the ImGui fonts
    uploadFonts();
}

void DuckApplication::uploadFonts() {
    VkCommandBuffer commandbuffer = utils::beginSingleTimeCommands(&vkSetup.device, imGuiCommandPool);
    ImGui_ImplVulkan_CreateFontsTexture(commandbuffer);
    utils::endSingleTimeCommands(&vkSetup.device, &vkSetup.graphicsQueue, &commandbuffer, &imGuiCommandPool);
}

//////////////////////
//
// Initialise GLFW
//
//////////////////////

void DuckApplication::initWindow() {
    // initialise glfw library
    glfwInit();

    // set parameters
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // initially for opengl, so tell it not to create opengl context
    //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // disable resizing for now

    // create the window, 4th param refs a monitor, 5th param is opengl related
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    // store an arbitrary pointer to a glfwwindow because glfw does not know how to call member function from a ptr to an instance of this class
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback); // tell the window what the call back for resizing is
}

//////////////////////
//
// Descriptors
//
//////////////////////

void DuckApplication::createDescriptorSetLayout() {
    // provide details about every descriptor binding used in the shaders
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // this is a uniform descriptor
    // specify binding used
    uboLayoutBinding.binding = 0; // the first descriptor
    uboLayoutBinding.descriptorCount = 1; // single uniform buffer object so just 1, could be used to specify a transform for each bone in a skeletal animation
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // in which shader stage is the descriptor going to be referenced
    uboLayoutBinding.pImmutableSamplers = nullptr; // relevant to image sampling related descriptors

    // same as above but for a texture sampler rather than for uniforms
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // this is a sampler descriptor
    samplerLayoutBinding.binding = 1; // the second descriptor
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; ;// the shader stage we wan the descriptor to be used in, ie the fragment shader stage
    // can use the texture sampler in the vertex stage as part of a height map to deform the vertices in a grid
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // put the descriptors in an array
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    
    // descriptor set bindings combined into a descriptor set layour object, created the same way as other vk objects by filling a struct in
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());; // number of bindings
    layoutInfo.pBindings = bindings.data(); // pointer to the bindings

    if (vkCreateDescriptorSetLayout(vkSetup.device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void DuckApplication::createDescriptorPool() {
    // descriptor layout describes descriptors that can be bound. Create a descriptor set for each buffer. We need 
    // to create a descriptor pool to get the descriptor set (much like the command pool for command queues)

    // from the ImGUI example function, the pool sizes have a descriptor count of 1000
    // we also need to allocate one pool for each frame for our descriptors (uniform and texture sampler)
    uint32_t swapChainImageCount = static_cast<uint32_t>(swapChainData.images.size());
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_POOL_NUM + swapChainImageCount },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMGUI_POOL_NUM + swapChainImageCount},
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, IMGUI_POOL_NUM }
    };

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    // the maximum number of descriptor sets that may be allocated
    poolInfo.maxSets = IMGUI_POOL_NUM * static_cast<uint32_t>(swapChainData.images.size());
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize)); // max nb of individual descriptor types
    poolInfo.pPoolSizes = poolSizes; // the descriptors

    // create the descirptor pool
    if (vkCreateDescriptorPool(vkSetup.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void DuckApplication::createDescriptorSets() {
    // create the descriptor set
    std::vector<VkDescriptorSetLayout> layouts(swapChainData.images.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    // specify the descriptor pool to allocate from
    allocInfo.descriptorPool = descriptorPool;
    // the number of descriptors to allocate
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainData.images.size());
    // a pointer to the descriptor layout to base them on
    allocInfo.pSetLayouts = layouts.data();

    // resize the descriptor set container to accomodate for the descriptor sets, as many as there are frames
    descriptorSets.resize(swapChainData.images.size());

    // now attempt to create them. We don't need to explicitly clear the descriptor sets because they will be freed
    // when the desciptor set is destroyed. The function may fail if the pool is not sufieciently large, but succeed other times 
    // if the driver can solve the problem internally... so sometimes the driver will let us get away with an allocation
    // outside of the limits of the desciptor pool, and other times fail! Goes without saying that this is different for every machine...
    if (vkAllocateDescriptorSets(vkSetup.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // loop over the created descriptor sets to configure them
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        // the buffer and the region of it that contain the data for the descriptor
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i]; // contents of buffer for image i
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject); // here this is the size of the whole buffer, we can use VK_WHOLE_SIZE instead

        // bind the actual image and sampler to the descriptors in the descriptor set
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = duckTexture.textureImageView;
        imageInfo.sampler = duckTexture.textureSampler;

        // the struct configuring the descriptor set
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        // the uniform buffer
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i]; // wich set to update
        descriptorWrites[0].dstBinding = 0; // uniform buffer has binding 0
        descriptorWrites[0].dstArrayElement = 0; // descriptors can be arrays, only one element so first index
        // type of descriptor again
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1; // can update multiple descriptors at once starting at dstArrayElement, descriptorCount specifies how many elements

        descriptorWrites[0].pBufferInfo = &bufferInfo; // for descriptors that use buffer data
        descriptorWrites[0].pImageInfo = nullptr; // for image data
        descriptorWrites[0].pTexelBufferView = nullptr; // desciptors refering to buffer views

        // the texture sampler
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1; // sampler has binding 1
        descriptorWrites[1].dstArrayElement = 0; // only one element so index 0
        // type of descriptor again
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;

        descriptorWrites[1].pBufferInfo = nullptr; // for descriptors that use buffer data
        descriptorWrites[1].pImageInfo = &imageInfo; // for image data
        descriptorWrites[1].pTexelBufferView = nullptr; // desciptors refering to buffer views

        // update according to the configuration
        vkUpdateDescriptorSets(vkSetup.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

//////////////////////
// 
// Uniforms
//
//////////////////////

void DuckApplication::createUniformBuffers() {
    // specify what the size of the buffer is
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // resize the uniform buffer to be as big as the swap chain, each image has its own set of uniforms
    uniformBuffers.resize(swapChainData.images.size());
    uniformBuffersMemory.resize(swapChainData.images.size());

    // loop over the images and create a uniform buffer for each
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void DuckApplication::updateUniformBuffer(uint32_t currentImage) {
    // compute the time elapsed since rendering began
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};

    // translate the model to start in view of the camera
    ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -30.0f, -85.0f));
    // add the user translation
    ubo.model = glm::translate(ubo.model, glm::vec3(translateX, translateY, translateZ));
    
    ubo.model = glm::scale(ubo.model, glm::vec3(zoom, zoom, zoom));

    // add the x, y, z rotations
    glm::quat rotation = glm::quat(glm::vec3(glm::radians(rotateX), glm::radians(rotateY), glm::radians(rotateZ)));

    ubo.model = ubo.model * glm::toMat4(rotation);

    // rotate the model to be upright
    //ubo.model = glm::rotate(ubo.model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

    // a simple rotation around the z axis
    //ubo.model = glm::rotate(ubo.model, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // make the camera view the geometry from above at a 45� angle (eye pos, subject pos, up direction)
    ubo.view = glm::mat4(1.0f); //glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // proect the scene with a 45� fov, use current swap chain extent to compute aspect ratio, near, far
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainData.extent.width / (float)swapChainData.extent.height, 0.1f, 100.0f);
    
    // designed for openGL, so y coordinates are inverted
    ubo.proj[1][1] *= -1;

    // set for mapping to texture
    ubo.uvToRgb = uvToRgb;
    // set for enabling/disabling material
    ubo.hasAmbient = enableAlbedo;
    ubo.hasDiffuse = enableDiffuse;
    ubo.hasSpecular = enableSpecular;

    // copy the uniform buffer object into the uniform buffer
    void* data;
    vkMapMemory(vkSetup.device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vkSetup.device, uniformBuffersMemory[currentImage]);
}

//////////////////////
//
// Command buffers setup
//
//////////////////////

void DuckApplication::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags) {
    // submit command buffers by submitting to one of the device queues, like graphics and presentation
    // each command pool can only allocate command buffers submitted on a single type of queue
    QueueFamilyIndices queueFamilyIndices = QueueFamilyIndices::findQueueFamilies(vkSetup.physicalDevice, vkSetup.surface);

    // command pool needs two parameters
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // the queue to submit to
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    // flag for command pool, influences how command buffers are rerecorded
    // VK_COMMAND_POOL_CREATE_TRANSIENT_BIT -> rerecorded with new commands often
    // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT -> let command buffers be rerecorded individually rather than together
    poolInfo.flags = flags; // in our case, we only record at beginning of program so leave empty

    // and create the command pool, we therfore ave to destroy it explicitly in cleanup
    if (vkCreateCommandPool(vkSetup.device, &poolInfo, nullptr, commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void DuckApplication::createCommandBuffers(std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool& commandPool) {
    // resize the command buffers container to the same size as the frame buffers container
    commandBuffers->resize(framebufferData.framebuffers.size());

    // create the struct
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; // identify the struct type
    allocInfo.commandPool = commandPool; // specify the command pool
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // level specifes is the buffers are primary or secondary
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY -> can be submitted to a queue for exec but not called from other command buffers
    // VK_COMMAND_BUFFER_LEVEL_SECONDARY -> cannot be submitted directly, but can be called from primary command buffers
    allocInfo.commandBufferCount = (uint32_t)commandBuffers->size(); // the number of buffers to allocate

    if (vkAllocateCommandBuffers(vkSetup.device, &allocInfo, commandBuffers->data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void DuckApplication::recordGemoetryCommandBuffer() {
    // start recording a command buffer 
    for (size_t i = 0; i < renderCommandBuffers.size(); i++) {
        // the following struct used as argument specifying details about the usage of specific command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0; // how we are going to use the command buffer
        // VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT -> command buffer will be immediately rerecorded
        // VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT -> is a secondary command buffer entirely within a single render pass
        // VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT -> command buffer can be resbmitted while also already pending execution
        beginInfo.pInheritanceInfo = nullptr; // relevant to secondary comnmand buffers

        // creating implicilty resets the command buffer if it was already recorded once, cannot append
        // commands to a buffer at a later time!
        if (vkBeginCommandBuffer(renderCommandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // create a render pass, initialised with some params in the following struct
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = swapChainData.renderPass; // handle to the render pass and the attachments to bind
        renderPassInfo.framebuffer = framebufferData.framebuffers[i]; // the framebuffer created for each swapchain image view
        renderPassInfo.renderArea.offset = { 0, 0 }; // some offset for the render area
        // best performance if same size as attachment
        renderPassInfo.renderArea.extent = swapChainData.extent; // size of the render area (where shaders load and stores occur, pixels outside are undefined)

        // because we used the VK_ATTACHMENT_LOAD_OP_CLEAR for load operations of the render pass, we need to set clear colours
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f }; // black with max opacity
        clearValues[1].depthStencil = { 1.0f, 0 }; // initialise the depth value to far (1 in the range of 0 to 1)
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size()); // only use a single value
        renderPassInfo.pClearValues = clearValues.data(); // the colour to use for clear operation

        // begin the render pass. All vkCmd functions are void, so error handling occurs at the end
        // first param for all cmd are the command buffer to record command to, second details the render pass we've provided
        vkCmdBeginRenderPass(renderCommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // final parameter controls how drawing commands within the render pass will be provided 
        // VK_SUBPASS_CONTENTS_INLINE -> render pass cmd embedded in primary command buffer and no secondary command buffers will be executed
        // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS -> render pass commands executed from secondary command buffers

            // bind the graphics pipeline, second param determines if the object is a graphics or compute pipeline
        vkCmdBindPipeline(renderCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.graphicsPipeline);

        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        // bind the vertex buffer, can have many vertex buffers
        vkCmdBindVertexBuffers(renderCommandBuffers[i], 0, 1, vertexBuffers, offsets);
        // bind the index buffer, can only have a single index buffer 
        // params (-the nescessary cmd) bufferindex buffer, byte offset into it, type of data
        vkCmdBindIndexBuffer(renderCommandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        // bind the uniform descriptor sets
        vkCmdBindDescriptorSets(renderCommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.graphicsPipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);

        // command to draw the vertices in the vertex buffer
        //vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0); 
        // params :
        // the command buffer
        // instance count, for instance rendering, so only one here
        // first vertex, offset into the vertex buffer. Defines lowest value of gl_VertexIndex
        // first instance, offset for instance rendering. Defines lowest value of gl_InstanceIndex

        vkCmdDrawIndexed(renderCommandBuffers[i], static_cast<uint32_t>(duckModel.indices.size()), 1, 0, 0, 0);
        // params :
        // the command buffer
        // the indices
        // no instancing so only a single instance
        // offset into index buffer
        // offest to add to the indices in the index buffer
        // offest for instancing

        // /!\ about vertex and index buffers /!\
            // The previous chapter already mentioned that should allocate multiple resources like buffers 
            // from a single memory allocation. Even better, Driver developers recommend to store multiple buffers, 
            // like the vertex and index buffer, into a single VkBuffer and use  offsets in commands like vkCmdBindVertexBuffers. 
            // The advantage is that your data is more cache friendly, because it's closer together. 

        // end the render pass
        vkCmdEndRenderPass(renderCommandBuffers[i]);

        // we've finished recording, so end recording and check for errors
        if (vkEndCommandBuffer(renderCommandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

//////////////////////
//
// Vertex buffer / Index buffer setup
//
//////////////////////

void DuckApplication::createVertexBuffer() {
    // precompute buffer size
    VkDeviceSize bufferSize = sizeof(duckModel.vertices[0]) * duckModel.vertices.size();
    // call our helper buffer creation function

    // use a staging buffer for mapping and copying 
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    // in host memory (cpu)
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    // VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.

    void* data;
    // access a region in memory ressource defined by offset and size (0 and bufferInfo.size), can use special value VK_WHOLE_SIZE to map all of the memory
    // second to last is for flages (none in current API so set to 0), last is output for pointer to mapped memory
    vkMapMemory(vkSetup.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, duckModel.vertices.data(), (size_t)bufferSize); // memcpy the data in the vertex list to that region in memory
    vkUnmapMemory(vkSetup.device, stagingBufferMemory); // unmap the memory 
    // possible issues as driver may not immediately copy data into buffer memory, writes to buffer may not be visible in mapped memory yet...
    // either use a heap that is host coherent (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT in memory requirements)
    // or call vkFlushMappedMemoryRanges after writing to mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory

    // create the vertex buffer, now the memory is device local (faster)
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
    // VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation
    utils::copyBuffer(&vkSetup.device, &vkSetup.graphicsQueue, renderCommandPool, stagingBuffer, vertexBuffer, bufferSize);

    // cleanup after using the staging buffer
    vkDestroyBuffer(vkSetup.device, stagingBuffer, nullptr);
    vkFreeMemory(vkSetup.device, stagingBufferMemory, nullptr);
}

void DuckApplication::createIndexBuffer() {
    // almost identical to the vertex buffer creation process except where commented
    VkDeviceSize bufferSize = sizeof(duckModel.indices[0]) * duckModel.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(vkSetup.device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, duckModel.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(vkSetup.device, stagingBufferMemory);

    // different usage bit flag VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice,bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    utils::copyBuffer(&vkSetup.device, &vkSetup.graphicsQueue, renderCommandPool, stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(vkSetup.device, stagingBuffer, nullptr);
    vkFreeMemory(vkSetup.device, stagingBufferMemory, nullptr);
}

//////////////////////
//
// Handling window resize events
//
//////////////////////

void DuckApplication::recreateVulkanData() {
    // for handling window minimisation, we get the size of the windo through the glfw framebuffer dimensions
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    // start an infinite loop to hang the process
    while (width == 0 || height == 0) {
        // continually evaluate the window dimensions, if the window is no longer hidder the loop will terminate
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    // wait before destroying if in use by the device
    vkDeviceWaitIdle(vkSetup.device);
    
    // destroy whatever is dependent on the old swap chain, tsarting with the command buffers
    vkFreeCommandBuffers(vkSetup.device, renderCommandPool, static_cast<uint32_t>(renderCommandBuffers.size()), renderCommandBuffers.data());
    vkFreeCommandBuffers(vkSetup.device, imGuiCommandPool, static_cast<uint32_t>(imGuiCommandBuffers.size()), imGuiCommandBuffers.data());
    
    // also destroy the uniform buffers that worked with the swap chain
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        vkDestroyBuffer(vkSetup.device, uniformBuffers[i], nullptr);
        vkFreeMemory(vkSetup.device, uniformBuffersMemory[i], nullptr);
    }

    // destroy the framebuffer data, followed by the swap chain data
    framebufferData.cleanupFrambufferData();
    swapChainData.cleanupSwapChainData();

    // recreate them
    swapChainData.initSwapChainData(&vkSetup, &descriptorSetLayout);
    framebufferData.initFramebufferData(&vkSetup, &swapChainData, renderCommandPool);

    // recreate descriptor data
    createUniformBuffers(); 
    createDescriptorSets();

    // recreate command buffers
    createCommandBuffers(&renderCommandBuffers, renderCommandPool); 
    createCommandBuffers(&imGuiCommandBuffers, imGuiCommandPool); 

    // record the rendering command buffer once it has been created
    recordGemoetryCommandBuffer();

    // update ImGui aswell
    ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(swapChainData.images.size()));
}

void DuckApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // pointer to this application class obtained from glfw, it doesnt know that it is a DuckApplication but we do so we can cast to it
    auto app = reinterpret_cast<DuckApplication*>(glfwGetWindowUserPointer(window));
    // and set the resize flag to true
    app->framebufferResized = true;
}

//////////////////////
//
// Synchronisation
//
//////////////////////

void DuckApplication::createSyncObjects() {
    // resize the semaphores to the number of simultaneous frames, each has its own semaphores
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainData.images.size(), VK_NULL_HANDLE); // explicitly initialise the fences in this vector to no fence

    VkSemaphoreCreateInfo semaphoreInfo{};
    // only required field at the moment, may change in the future
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // initialise in the signaled state

    // simply loop over each frame and create semaphores for them
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // attempt to create the semaphors
        if (vkCreateSemaphore(vkSetup.device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vkSetup.device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vkSetup.device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
    }
}

//////////////////////
//
// Main loop 
//
//////////////////////

void DuckApplication::mainLoop() {
    // loop keeps window open
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        drawFrame();
    }
    vkDeviceWaitIdle(vkSetup.device);
}

//////////////////////
//
// Frame drawing and GUI 
//
//////////////////////

void DuckApplication::drawFrame() {
    // will acquire an image from swap chain, exec commands in command buffer with images as attachments in the frameBuffer
    // return the image to the swap buffer. These tasks are started simultaneously but executed asynchronously.
    // However we want these to occur in sequence because each relies on the previous task success
    // For syncing can use semaphores or fences and coordinate operations by having one op signal another
    // op and another operation wait for a fence or semaphor to go from unsignaled to signaled.
    // we can access fence state with vkWaitForFences and not semaphores.
    // fences are mainly for syncing app with rendering op, use here to synchronise the frame rate
    // semaphores are for syncing ops within or across cmd queues. We want to sync queue op to draw cmds and presentation so pref semaphores here

    // at the start of the frame, make sure that the previous frame has finished which will signal the fence
    //vkWaitForFences(vkSetup.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // retrieve an image from the swap chain
    // swap chain is an extension so use the vk*KHR function
    VkResult result = vkAcquireNextImageKHR(vkSetup.device, swapChainData.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); // params:
    // the logical device and the swap chain we want to restrieve image from
    // a timeout in ns. Using UINT64_MAX disables it
    // synchronisation objects, so a semaphore
    // handle to another sync object (which we don't use so nul handle)
    // variable to output the swap chain image that has become available

    // Vulkan tells us if the swap chain is out of date with this result value (the swap chain is incompatible with the surface, eg window resize)
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // if so then recreate the swap chain and try to acquire the image from the new swap chain
        recreateVulkanData();
        return; // return to acquire the image again
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { // both values here are considered "success", even if partial, values
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(vkSetup.device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // update the unifrom buffer before submitting
    updateUniformBuffer(imageIndex);

    // update the UI, may change at every frame so this is where it is recorded, unlike the geometry which executes the same commands 
    // hence it is recorded once at the start 
    renderUI();

    // the two command buffers, for geometry and UI
    std::array<VkCommandBuffer, 2> submitCommandBuffers = { renderCommandBuffers[imageIndex], imGuiCommandBuffers[imageIndex] };

    // info needed to submit the command buffers
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // which semaphores to wait on before execution begins
    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    // which stages of the pipeline to wait at (here at the stage where we write colours to the attachment)
    // we can in theory start work on vertex shader etc while image is not yet available
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages; // for each sempahore we provide a wait stage
    // which command buffer to submit to, submit the cmd buffer that binds the swap chain image we acquired as color attachment
    submitInfo.commandBufferCount = static_cast<uint32_t>(submitCommandBuffers.size());
    submitInfo.pCommandBuffers = submitCommandBuffers.data();

    // which semaphores to signal once the command buffer(s) has finished, we are using the renderFinishedSemaphore for that
    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[imageIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    // reset the fence so that when submitting the commands to the graphics queue, the fence is set to block so no subsequent 
    vkResetFences(vkSetup.device, 1, &inFlightFences[currentFrame]);

    // submit the command buffer to the graphics queue, takes an array of submitinfo when work load is much larger
    // last param is a fence, which is signaled when the cmd buffer finishes executing and is used to inform that the frame has finished
    // being rendered (the commands were all executed). The next frame can start rendering!
    if (vkQueueSubmit(vkSetup.graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // submitting the result back to the swap chain to have it shown onto the screen
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // which semaphores to wait on 
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    // specify the swap chains to present image to and index of image for each swap chain
    VkSwapchainKHR swapChains[] = { swapChainData.swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    // allows to specify an array of vKResults to check for every individual swap chain if presentation is succesful
    presentInfo.pResults = nullptr; // Optional

    // submit the request to put an image from the swap chain to the presentation queue
    result = vkQueuePresentKHR(vkSetup.presentQueue, &presentInfo);

    // similar to when acquiring the swap chain image, check that the presentation queue can accept the image, also check for resizing
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized || (enableDepthTest != swapChainData.enableDepthTest)) {
        // tell the swapchaindata to change whether to enable or disable the depth testing when recreating the pipeline
        swapChainData.enableDepthTest = enableDepthTest;
        framebufferResized = false;
        recreateVulkanData();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // after the frame is drawn and presented, increment current frame count (% loops around)
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void DuckApplication::renderUI() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame(); // empty
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // for now just display the demo window
    //ImGui::ShowDemoWindow();

    ImGui::Begin("Transform the model"); // Create a window for transforming the model in world space
    ImGui::Text("Translation:");
    ImGui::SliderFloat("x ", &translateX, -100.0f, 100.0f);
    ImGui::SliderFloat("y ", &translateY, -100.0f, 100.0f);
    ImGui::SliderFloat("z ", &translateZ, -100.0f, 100.0f);
    ImGui::Text("Rotation:");
    ImGui::SliderFloat("x", &rotateX, 0.0f, 360.0f);
    ImGui::SliderFloat("y", &rotateY, 0.0f, 360.0f);
    ImGui::SliderFloat("z", &rotateZ, 0.0f, 360.0f);
    ImGui::Text("Zoom:");
    ImGui::SliderFloat("", &zoom, 0.0f, 1.0f);
    ImGui::End();

    ImGui::Begin("Render stages and material properties");
    ImGui::Checkbox("Depth test", &enableDepthTest);
    ImGui::Checkbox("UV to RGB", &uvToRgb);
    ImGui::Checkbox("Ambient/Albedo", &enableAlbedo);
    ImGui::Checkbox("Diffues/Labertian", &enableDiffuse);
    ImGui::Checkbox("Specular", &enableSpecular);
    ImGui::End();

    // tell ImGui to render
    ImGui::Render();

    // start recording into a command buffer
    VkCommandBufferBeginInfo commandbufferInfo = {};
    commandbufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandbufferInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(imGuiCommandBuffers[imageIndex], &commandbufferInfo);

    // begin the render pass
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = swapChainData.imGuiRenderPass;
    renderPassBeginInfo.framebuffer = framebufferData.imGuiFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.extent.width =  swapChainData.extent.width;
    renderPassBeginInfo.renderArea.extent.height = swapChainData.extent.height;
    renderPassBeginInfo.clearValueCount = 1;
    VkClearValue clearValue{ 0.0f, 0.0f, 0.0f, 0.0f }; // completely opaque clear value
    renderPassBeginInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(imGuiCommandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), imGuiCommandBuffers[imageIndex]);

    // Submit command buffer
    vkCmdEndRenderPass(imGuiCommandBuffers[imageIndex]);
    vkEndCommandBuffer(imGuiCommandBuffers[imageIndex]);
}

//////////////////////
//
// Cleanup
//
//////////////////////

void DuckApplication::cleanup() {

    // destroy the imgui context when the program ends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // destroy whatever is dependent on the old swap chain
    vkFreeCommandBuffers(vkSetup.device, renderCommandPool, static_cast<uint32_t>(renderCommandBuffers.size()), renderCommandBuffers.data());
    vkFreeCommandBuffers(vkSetup.device, imGuiCommandPool, static_cast<uint32_t>(imGuiCommandBuffers.size()), imGuiCommandBuffers.data());
    
    // also destroy the uniform buffers that worked with the swap chain
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        vkDestroyBuffer(vkSetup.device, uniformBuffers[i], nullptr);
        vkFreeMemory(vkSetup.device, uniformBuffersMemory[i], nullptr);
    }

    // call the function we created for destroying the swap chain and frame buffers
    // in the reverse order of their creation
    framebufferData.cleanupFrambufferData();
    swapChainData.cleanupSwapChainData();

    // cleanup the descriptor pools and descriptor sets
    vkDestroyDescriptorPool(vkSetup.device, imGuiDescriptorPool, nullptr);
    vkDestroyDescriptorPool(vkSetup.device, descriptorPool, nullptr);

    duckTexture.cleanupTexture();

    // destroy the descriptor layout
    vkDestroyDescriptorSetLayout(vkSetup.device, descriptorSetLayout, nullptr);

    // destroy the index buffer and free its memory
    vkDestroyBuffer(vkSetup.device, indexBuffer, nullptr);
    vkFreeMemory(vkSetup.device, indexBufferMemory, nullptr);

    // destroy the vertex buffer and free its memory
    vkDestroyBuffer(vkSetup.device, vertexBuffer, nullptr);
    vkFreeMemory(vkSetup.device, vertexBufferMemory, nullptr);


    // loop over each frame and destroy its semaphores 
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(vkSetup.device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(vkSetup.device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(vkSetup.device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(vkSetup.device, renderCommandPool, nullptr);
    vkDestroyCommandPool(vkSetup.device, imGuiCommandPool, nullptr);

    vkSetup.cleanupSetup();

    // destory the window
    glfwDestroyWindow(window);

    // terminate glfw
    glfwTerminate();
}

