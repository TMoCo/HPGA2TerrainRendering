// include class definition
#include <TerrainApplication.h>

// include constants
#include <utils/Utils.h>

// transformations
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // because OpenGL uses depth range -1.0 - 1.0 and Vulkan uses 0.0 - 1.0
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#ifdef DEBUG
#include <glm/gtx/string_cast.hpp>
#endif // DEBUG


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

void TerrainApplication::run() {
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

void TerrainApplication::initVulkan() {
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
    createCommandPool(&renderCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    createCommandPool(&imGuiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    //
    // STEP 3: Swap chain and frame buffers
    //

    // create the swap chain
    swapChainData.initSwapChainData(&vkSetup, descriptorSetLayouts.data());
    // create the frame buffers
    framebufferData.initFramebufferData(&vkSetup, &swapChainData, renderCommandPool);

    //
    // STEP 4: Create the application's data (models, textures...)
    //

    airplane.createAirplane(&vkSetup, renderCommandPool, glm::vec3(0.0f, 255.0f, 0.0f));

    terrain.createTerrain(&vkSetup, renderCommandPool, selectedMap);
    
    // load terrain textures
    textures.resize(TERRAIN_TEXTURE_PATHS.size());
    for (int i = 0; i < textures.size(); i++) {
        textures[i].createTexture(&vkSetup, TERRAIN_TEXTURE_PATHS[i], renderCommandPool, VK_FORMAT_R8G8B8A8_SRGB);
    }

    // set the debug camera
    debugCamera = Camera(glm::vec3(0.0f, 100.0f, 0.0f), 50.0f, 50.0f);

    createVertexBuffer<Vertex>(&terrain.vertices, &bTerrainVertex);
    createVertexBuffer(&airplane.model.vertices, &bAirplaneVertex);

    createIndexBuffer(&terrain.indices, &bTerrainIndex);
    createIndexBuffer(&airplane.model.indices, &bAirplaneIndex);


    //
    // STEP 5: create the vulkan data for accessing and using the app's data
    //

    // these depend on the size of the framebuffer, so create them after 
    createUniformBuffer<TerrainUBO>(&bTerrainUniforms);
    createUniformBuffer<AirplaneUBO>(&bAirplaneUniforms);
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers(&renderCommandBuffers, renderCommandPool);
    createCommandBuffers(&imGuiCommandBuffers, imGuiCommandPool);
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        recordGeometryCommandBuffer(i);
    }

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

void TerrainApplication::initImGui() {
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

void TerrainApplication::uploadFonts() {
    VkCommandBuffer commandbuffer = utils::beginSingleTimeCommands(&vkSetup.device, imGuiCommandPool);
    ImGui_ImplVulkan_CreateFontsTexture(commandbuffer);
    utils::endSingleTimeCommands(&vkSetup.device, &vkSetup.graphicsQueue, &commandbuffer, &imGuiCommandPool);
}

//////////////////////
//
// Initialise GLFW
//
//////////////////////

void TerrainApplication::initWindow() {
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

void TerrainApplication::createDescriptorSetLayout() {
    // need two descriptor set layouts for two pipelines
    descriptorSetLayouts.resize(N_DESCRIPTOR_LAYOUTS);

    // Terrain descriptor layout
    VkDescriptorSetLayoutBinding terrainUboLayoutBinding{};
    terrainUboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // this is a uniform descriptor
    terrainUboLayoutBinding.binding            = 0; // the first descriptor
    terrainUboLayoutBinding.descriptorCount    = 1; // single uniform buffer object so just 1, could be used to specify a transform for each bone in a skeletal animation
    terrainUboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // in which shader stage is the descriptor going to be referenced
    terrainUboLayoutBinding.pImmutableSamplers = nullptr; // relevant to image sampling related descriptors

    // same as above but for a texture sampler rather than for uniforms
    VkDescriptorSetLayoutBinding terrainHeightSamplerLayoutBinding{};
    terrainHeightSamplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // this is a sampler descriptor
    terrainHeightSamplerLayoutBinding.binding            = 1; // the second descriptor
    terrainHeightSamplerLayoutBinding.descriptorCount    = 1; // height map 
    terrainHeightSamplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    terrainHeightSamplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding terrainTextureSamplerLayoutBinding{};
    terrainTextureSamplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // this is a sampler descriptor
    terrainTextureSamplerLayoutBinding.binding            = 2; // the third descriptor
    terrainTextureSamplerLayoutBinding.descriptorCount    = 3; // textures
    terrainTextureSamplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    terrainTextureSamplerLayoutBinding.pImmutableSamplers = nullptr;

    // put the descriptors in an array
    std::array<VkDescriptorSetLayoutBinding, 3> terrainBindings = { terrainUboLayoutBinding, terrainHeightSamplerLayoutBinding, terrainTextureSamplerLayoutBinding };

    VkDescriptorSetLayoutCreateInfo terrainLayoutInfo{};
    terrainLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    terrainLayoutInfo.bindingCount = static_cast<uint32_t>(terrainBindings.size()); // number of bindings
    terrainLayoutInfo.pBindings    = terrainBindings.data(); // pointer to the bindings

    if (vkCreateDescriptorSetLayout(vkSetup.device, &terrainLayoutInfo, nullptr, &descriptorSetLayouts[0]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    // Airplane descriptor layout
    VkDescriptorSetLayoutBinding airplaneUboLayoutBinding{};
    airplaneUboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    airplaneUboLayoutBinding.binding            = 0; // the first descriptor
    airplaneUboLayoutBinding.descriptorCount    = 1; // single uniform buffer object so just 1, could be used to specify a transform for each bone in a skeletal animation
    airplaneUboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // in which shader stage is the descriptor going to be referenced
    airplaneUboLayoutBinding.pImmutableSamplers = nullptr; // relevant to image sampling related descriptors


    // same as above but for a texture sampler rather than for uniforms
    VkDescriptorSetLayoutBinding airplaneSamplerLayoutBinding{};
    airplaneSamplerLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // this is a sampler descriptor
    airplaneSamplerLayoutBinding.binding            = 1; // the second descriptor
    airplaneSamplerLayoutBinding.descriptorCount    = 1;
    airplaneSamplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT; ;// the shader stage we wan the descriptor to be used in, ie the fragment shader stage
    // can use the texture sampler in the vertex stage as part of a height map to deform the vertices in a grid
    airplaneSamplerLayoutBinding.pImmutableSamplers = nullptr;

    // put the descriptors in an array
    std::array<VkDescriptorSetLayoutBinding, 2> airplaneBindings = { airplaneUboLayoutBinding, airplaneSamplerLayoutBinding };
    
    // descriptor set bindings combined into a descriptor set layour object, created the same way as other vk objects by filling a struct in
    VkDescriptorSetLayoutCreateInfo airplaneLayoutInfo{};
    airplaneLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    airplaneLayoutInfo.bindingCount = static_cast<uint32_t>(airplaneBindings.size());; // number of bindings
    airplaneLayoutInfo.pBindings    = airplaneBindings.data(); // pointer to the bindings

    if (vkCreateDescriptorSetLayout(vkSetup.device, &airplaneLayoutInfo, nullptr, &descriptorSetLayouts[1]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void TerrainApplication::createDescriptorPool() {
    // descriptor layout describes descriptors that can be bound. Create a descriptor set for each buffer. We need 
    // to create a descriptor pool to get the descriptor set (much like the command pool for command queues)

    // from the ImGUI example function, the pool sizes have a descriptor count of 1000
    // we also need to allocate one pool for each frame for our descriptors (uniform and texture sampler)
    uint32_t swapChainImageCount = static_cast<uint32_t>(swapChainData.images.size());
    VkDescriptorPoolSize poolSizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_POOL_NUM + swapChainImageCount * N_DESCRIPTOR_LAYOUTS },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, IMGUI_POOL_NUM },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, IMGUI_POOL_NUM + swapChainImageCount * N_DESCRIPTOR_LAYOUTS },
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

void TerrainApplication::createDescriptorSets() {
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainData.images.size());

    // terrain's descriptor sets 
    std::vector<VkDescriptorSetLayout> terrainLayouts(swapChainData.images.size(), descriptorSetLayouts[0]);
    allocInfo.pSetLayouts = terrainLayouts.data();

    terrainDescriptorSets.resize(swapChainData.images.size());

    if (vkAllocateDescriptorSets(vkSetup.device, &allocInfo, terrainDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // airplane's descriptor sets 
    std::vector<VkDescriptorSetLayout> airplaneLayouts(swapChainData.images.size(), descriptorSetLayouts[1]);
    allocInfo.pSetLayouts = airplaneLayouts.data();

    airplaneDescriptorSets.resize(swapChainData.images.size());

    if (vkAllocateDescriptorSets(vkSetup.device, &allocInfo, airplaneDescriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    // loop over the created descriptor sets to configure them
    for (size_t i = 0; i < swapChainData.images.size(); i++) {
        // the buffer and the region of it that contain the data for the descriptor
        VkDescriptorBufferInfo terrainBufferInfo{};
        terrainBufferInfo.buffer = bTerrainUniforms.buffer;       // the buffer containing the uniforms 
        terrainBufferInfo.offset = sizeof(TerrainUBO) * i; // the offset to access uniforms for image i
        terrainBufferInfo.range  = sizeof(TerrainUBO);     // here the size of the buffer we want to access

        // bind the actual image and sampler to the descriptors in the descriptor set
        VkDescriptorImageInfo terrainImageInfo{};
        terrainImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        terrainImageInfo.imageView   = terrain.heightMap.textureImageView;
        terrainImageInfo.sampler     = terrain.heightMap.textureSampler;

        std::vector<VkDescriptorImageInfo> terrainTexturesImageInfos(textures.size());

        for (uint32_t i = 0; i < textures.size(); ++i)
        {
            terrainTexturesImageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            terrainTexturesImageInfos[i].imageView = textures[i].textureImageView;
            terrainTexturesImageInfos[i].sampler = textures[i].textureSampler;
        }

        std::array<VkWriteDescriptorSet, 3> terrainDescriptorWrites{};

        // the uniform buffer
        terrainDescriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        terrainDescriptorWrites[0].dstSet           = terrainDescriptorSets[i]; // wich set to update
        terrainDescriptorWrites[0].dstBinding       = 0; // uniform buffer has binding 0
        terrainDescriptorWrites[0].dstArrayElement  = 0; // descriptors can be arrays, only one element so first index
        terrainDescriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        terrainDescriptorWrites[0].descriptorCount  = 1; // can update multiple descriptors at once starting at dstArrayElement, descriptorCount specifies how many elements
        terrainDescriptorWrites[0].pBufferInfo      = &terrainBufferInfo;

        // the height sampler
        terrainDescriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        terrainDescriptorWrites[1].dstSet          = terrainDescriptorSets[i];
        terrainDescriptorWrites[1].dstBinding      = 1; // texture sampler has binding 1
        terrainDescriptorWrites[1].dstArrayElement = 0; // only one element so index 0
        terrainDescriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        terrainDescriptorWrites[1].descriptorCount = 1;
        terrainDescriptorWrites[1].pImageInfo      = &terrainImageInfo; // for image data

        // the textures 
        terrainDescriptorWrites[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        terrainDescriptorWrites[2].dstSet          = terrainDescriptorSets[i];
        terrainDescriptorWrites[2].dstBinding      = 2; // texture sampler has binding 1
        terrainDescriptorWrites[2].dstArrayElement = 0; // only one element so index 0
        terrainDescriptorWrites[2].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        terrainDescriptorWrites[2].descriptorCount = 3;
        terrainDescriptorWrites[2].pImageInfo      = terrainTexturesImageInfos.data(); // for image data

        // update according to the configuration
        vkUpdateDescriptorSets(vkSetup.device, static_cast<uint32_t>(terrainDescriptorWrites.size()), terrainDescriptorWrites.data(), 0, nullptr);
         
        // do the same for the airplane descriptor set
        VkDescriptorBufferInfo airplaneBufferInfo{};
        airplaneBufferInfo.buffer = bAirplaneUniforms.buffer;           // the buffer containing the uniforms 
        airplaneBufferInfo.offset = sizeof(AirplaneUBO) * i; // the offset to access uniforms for image i
        airplaneBufferInfo.range  = sizeof(AirplaneUBO);     // here the size of the buffer we want to access

        // bind the actual image and sampler to the descriptors in the descriptor set
        VkDescriptorImageInfo airplaneImageInfo{};
        airplaneImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        airplaneImageInfo.imageView   = airplane.texture.textureImageView;
        airplaneImageInfo.sampler     = airplane.texture.textureSampler;

        // the struct configuring the descriptor set
        std::array<VkWriteDescriptorSet, 2> airplaneDescriptorWrites{};

        // the uniform buffer
        airplaneDescriptorWrites[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        airplaneDescriptorWrites[0].dstSet          = airplaneDescriptorSets[i]; // wich set to update
        airplaneDescriptorWrites[0].dstBinding      = 0; // uniform buffer has binding 0
        airplaneDescriptorWrites[0].dstArrayElement = 0; // descriptors can be arrays, only one element so first index
        airplaneDescriptorWrites[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        airplaneDescriptorWrites[0].descriptorCount = 1; // can update multiple descriptors at once starting at dstArrayElement, descriptorCount specifies how many elements

        airplaneDescriptorWrites[0].pBufferInfo      = &airplaneBufferInfo; // for descriptors that use buffer data
        airplaneDescriptorWrites[0].pImageInfo       = nullptr; // for image data
        airplaneDescriptorWrites[0].pTexelBufferView = nullptr; // desciptors refering to buffer views

        // the texture sampler
        airplaneDescriptorWrites[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        airplaneDescriptorWrites[1].dstSet          = airplaneDescriptorSets[i];
        airplaneDescriptorWrites[1].dstBinding      = 1; // texture sampler has binding 1
        airplaneDescriptorWrites[1].dstArrayElement = 0; // only one element so index 0
        airplaneDescriptorWrites[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        airplaneDescriptorWrites[1].descriptorCount = 1;

        airplaneDescriptorWrites[1].pBufferInfo      = nullptr; // for descriptors that use buffer data
        airplaneDescriptorWrites[1].pImageInfo       = &airplaneImageInfo; // for image data
        airplaneDescriptorWrites[1].pTexelBufferView = nullptr; // desciptors refering to buffer views
        
        vkUpdateDescriptorSets(vkSetup.device, static_cast<uint32_t>(airplaneDescriptorWrites.size()), airplaneDescriptorWrites.data(), 0, nullptr);
    }
}

//////////////////////
// 
// Uniforms
//
//////////////////////

void TerrainApplication::updateUniformBuffer(uint32_t currentImage) {

    // scene light
    glm::vec4 lightPos{ 0.0f, 400.0f, 0.0f, 0.0f};

    // view matrix depending on debug state
    glm::mat4 view;
    if (debugCameraState) {
        view = debugCamera.getViewMatrix();
        viewDir = glm::to_string(debugCamera.getOrientation().front);
    }
    else {
        view = airplane.camera.getViewMatrix();
        viewDir = glm::to_string(airplane.camera.getOrientation().front);
    }

    // projection matrix
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), swapChainData.extent.width / (float)swapChainData.extent.height, 0.1f, 1000.0f);
    // glm designed for openGL, so y coordinates are inverted. Vulkan origin is at top left vs OpenGL at bottom left
    proj[1][1] *= -1;
    
    // model matrix
    glm::mat4 model;

    // terrain uniform buffer object
    TerrainUBO terrainUbo{};

    // translation
    model = glm::translate(glm::mat4(1.0f), glm::vec3(translateX, translateY, translateZ));
    // uniform scale
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    // rotation
    model *= glm::toMat4(glm::quat(glm::vec3(glm::radians(rotateX), glm::radians(rotateY), glm::radians(rotateZ))));

    // set uniform buffer
    terrainUbo.model = model;
    terrainUbo.view = view;
    terrainUbo.proj = proj;
    terrainUbo.normal = glm::transpose(glm::inverse(model));
    terrainUbo.lightPos = lightPos;
    terrainUbo.vertexStride = vertexStride;
    terrainUbo.heightScalar = heightScalar;
    terrainUbo.mapDim = terrain.heightMap.height; // assumes same height and width
    terrainUbo.invMapDim = 1.0f / (float)(terrainUbo.mapDim == 0 ? 1 : terrainUbo.mapDim);

 
    // copy the uniform buffer object into the uniform buffer
    void* terrainData;
    vkMapMemory(vkSetup.device, bTerrainUniforms.memory, currentImage * sizeof(terrainUbo), sizeof(terrainUbo), 0, &terrainData);
    memcpy(terrainData, &terrainUbo, sizeof(terrainUbo));
    vkUnmapMemory(vkSetup.device, bTerrainUniforms.memory);

    // airplane uniform buffer object
    AirplaneUBO airplaneUbo{};

    // overwrite model matrix used for terrain
    model = glm::translate(glm::mat4(1.0f), airplane.camera.position);  // translate the plane to the camera
    model = glm::translate(model, airplane.camera.orientation.front * 10.0f); // translate the plane in front of the camera
    model *= airplane.camera.orientation.toWorldSpaceRotation(); // rotate the plane based on the camera's orientation
    model = glm::scale(model, glm::vec3(0.4f, 0.4f, 0.4f)); // scale the plane to an acceptable size

    airplaneUbo.model = model;
    airplaneUbo.view = view;
    airplaneUbo.proj = proj;
    airplaneUbo.normal = glm::transpose(glm::inverse(model));
    airplaneUbo.lightPos = lightPos;

    void* airplaneData;
    vkMapMemory(vkSetup.device, bAirplaneUniforms.memory, currentImage * sizeof(airplaneUbo), sizeof(airplaneUbo), 0, &airplaneData);
    memcpy(airplaneData, &airplaneUbo, sizeof(airplaneUbo));
    vkUnmapMemory(vkSetup.device, bAirplaneUniforms.memory);
}

//////////////////////
//
// Command buffers setup
//
//////////////////////

void TerrainApplication::createCommandPool(VkCommandPool* commandPool, VkCommandPoolCreateFlags flags) {
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

void TerrainApplication::createCommandBuffers(std::vector<VkCommandBuffer>* commandBuffers, VkCommandPool& commandPool) {
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

void TerrainApplication::recordGeometryCommandBuffer(size_t cmdBufferIndex) {
    // the following struct used as argument specifying details about the usage of specific command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // how we are going to use the command buffer
    beginInfo.pInheritanceInfo = nullptr; // relevant to secondary comnmand buffers

    // creating implicilty resets the command buffer if it was already recorded once, cannot append
    // commands to a buffer at a later time!
    if (vkBeginCommandBuffer(renderCommandBuffers[cmdBufferIndex], &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // create a render pass, initialised with some params in the following struct
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = swapChainData.renderPass; // handle to the render pass and the attachments to bind
    renderPassInfo.framebuffer       = framebufferData.framebuffers[cmdBufferIndex]; // the framebuffer created for each swapchain image view
    renderPassInfo.renderArea.offset = { 0, 0 }; // some offset for the render area
    // best performance if same size as attachment
    renderPassInfo.renderArea.extent = swapChainData.extent; // size of the render area (where shaders load and stores occur, pixels outside are undefined)

    // because we used the VK_ATTACHMENT_LOAD_OP_CLEAR for load operations of the render pass, we need to set clear colours
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color           = { 0.0f, 0.0f, 0.0f, 1.0f }; // black with max opacity
    clearValues[1].depthStencil    = { 1.0f, 0 }; // initialise the depth value to far (1 in the range of 0 to 1)
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size()); // only use a single value
    renderPassInfo.pClearValues    = clearValues.data(); // the colour to use for clear operation

    // begin the render pass. All vkCmd functions are void, so error handling occurs at the end
    // first param for all cmd are the command buffer to record command to, second details the render pass we've provided
    vkCmdBeginRenderPass(renderCommandBuffers[cmdBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    // final parameter controls how drawing commands within the render pass will be provided 
    VkDeviceSize offset = 0;

    //
    // DRAW TERRAIN
    //

    // check if drawing on CPU or GPU
    if (onGPU) {
        // bind the graphics pipeline, second param determines if the object is a graphics or compute pipeline
        vkCmdBindPipeline(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.terrainPipelineGPU);
        // bind the index buffer, can only have a single index buffer 
        vkCmdBindIndexBuffer(renderCommandBuffers[cmdBufferIndex], bTerrainIndex.buffer, 0, VK_INDEX_TYPE_UINT32);
        // bind the uniform descriptor sets
        vkCmdBindDescriptorSets(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.terrainPipelineLayout, 0, 1, &terrainDescriptorSets[cmdBufferIndex], 0, nullptr);

        if (!applyBinning) {
            // simply draw all terrain chunks
            for (auto& chunk: terrain.chunks) {
                vkCmdDrawIndexed(renderCommandBuffers[cmdBufferIndex], static_cast<uint32_t>(chunk.indices.size()), 1, chunk.chunkOffset, 0, 0);
            }
        }
        else {
            // loop over the visible chuncks and draw it
            for (auto& pair : terrain.visible) {
                auto chunk = pair.second;
                vkCmdDrawIndexed(renderCommandBuffers[cmdBufferIndex], static_cast<uint32_t>(chunk->indices.size()), 1, chunk->chunkOffset, 0, 0);
            }
        }

    }
    else {
        // bind the graphics pipeline, second param determines if the object is a graphics or compute pipeline
        vkCmdBindPipeline(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.terrainPipeline);
        // bind the vertex buffer, can have many vertex buffers
        vkCmdBindVertexBuffers(renderCommandBuffers[cmdBufferIndex], 0, 1, &bTerrainVertex.buffer, &offset);
        // bind the index buffer, can only have a single index buffer 
        vkCmdBindIndexBuffer(renderCommandBuffers[cmdBufferIndex], bTerrainIndex.buffer, 0, VK_INDEX_TYPE_UINT32);
        // bind the uniform descriptor sets
        vkCmdBindDescriptorSets(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.terrainPipelineLayout, 0, 1, &terrainDescriptorSets[cmdBufferIndex], 0, nullptr);
        if (!applyBinning) {
            // simply draw all terrain chunks
            for (auto& chunk : terrain.chunks) {
                vkCmdDrawIndexed(renderCommandBuffers[cmdBufferIndex], static_cast<uint32_t>(chunk.indices.size()), 1, chunk.chunkOffset, 0, 0);
            }
        }
        else {
            // loop over the visible chuncks and draw it
            for (auto& pair : terrain.visible) {
                auto chunk = pair.second;
                vkCmdDrawIndexed(renderCommandBuffers[cmdBufferIndex], static_cast<uint32_t>(chunk->indices.size()), 1, chunk->chunkOffset, 0, 0);
            }
        }
    }
    
    //
    // DRAW AIRPLANE
    //

    // bind to a new pipeline to draw the plane
    vkCmdBindPipeline(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.airplanePipeline);

    vkCmdBindVertexBuffers(renderCommandBuffers[cmdBufferIndex], 0, 1, &bAirplaneVertex.buffer, &offset);
    vkCmdBindIndexBuffer(renderCommandBuffers[cmdBufferIndex], bAirplaneIndex.buffer, 0, VK_INDEX_TYPE_UINT32); // index offset is in bytes

    vkCmdBindDescriptorSets(renderCommandBuffers[cmdBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, swapChainData.airplanePipelineLayout, 0, 1, &airplaneDescriptorSets[cmdBufferIndex], 0, nullptr);

    vkCmdDrawIndexed(renderCommandBuffers[cmdBufferIndex], static_cast<uint32_t>(airplane.model.indices.size()), 1, 0, 0, 0);
        
    // end the render pass
    vkCmdEndRenderPass(renderCommandBuffers[cmdBufferIndex]);

    // we've finished recording, so end recording and check for errors
    if (vkEndCommandBuffer(renderCommandBuffers[cmdBufferIndex]) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

//////////////////////
//
// Vertex buffer / Index buffer setup
//
//////////////////////

void TerrainApplication::createIndexBuffer(std::vector<uint32_t>* indices, BufferData* bufferData) {
    // because we generated a mesh for the terrain, we can use the indices computed
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

    BufferData stagingBuffer;

    BufferCreateInfo createInfo{};
    createInfo.size = bufferSize;
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    createInfo.pBufferData = &stagingBuffer;

    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, &createInfo);

    void* data;
    vkMapMemory(vkSetup.device, stagingBuffer.memory, 0, bufferSize, 0, &data);
    memcpy(data, indices->data(), (size_t)bufferSize);
    vkUnmapMemory(vkSetup.device, stagingBuffer.memory);

    // different usage bit flag VK_BUFFER_USAGE_INDEX_BUFFER_BIT instead of VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    createInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    createInfo.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createInfo.pBufferData = bufferData;

    utils::createBuffer(&vkSetup.device, &vkSetup.physicalDevice, &createInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;

    BufferCopyInfo copyInfo{};
    copyInfo.pSrc = &stagingBuffer.buffer;
    copyInfo.pDst = &bufferData->buffer;
    copyInfo.copyRegion = copyRegion;

    utils::copyBuffer(&vkSetup.device, &vkSetup.graphicsQueue, renderCommandPool, &copyInfo);

    stagingBuffer.cleanupBufferData(vkSetup.device);
}

//////////////////////
//
// Handling window resize events
//
//////////////////////

void TerrainApplication::recreateVulkanData() {
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
    bTerrainUniforms.cleanupBufferData(vkSetup.device);
    bAirplaneUniforms.cleanupBufferData(vkSetup.device);

    // destroy the framebuffer data, followed by the swap chain data
    framebufferData.cleanupFrambufferData();
    swapChainData.cleanupSwapChainData();

    // recreate them
    swapChainData.initSwapChainData(&vkSetup, descriptorSetLayouts.data());
    framebufferData.initFramebufferData(&vkSetup, &swapChainData, renderCommandPool);

    // recreate descriptor data
    createUniformBuffer<TerrainUBO>(&bTerrainUniforms);
    createUniformBuffer<AirplaneUBO>(&bAirplaneUniforms);
    createDescriptorSets();

    // recreate command buffers
    createCommandBuffers(&renderCommandBuffers, renderCommandPool); 
    createCommandBuffers(&imGuiCommandBuffers, imGuiCommandPool); 

    // update ImGui aswell
    ImGui_ImplVulkan_SetMinImageCount(static_cast<uint32_t>(swapChainData.images.size()));
}

void TerrainApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    // pointer to this application class obtained from glfw, it doesnt know that it is a TerrainApplication but we do so we can cast to it
    auto app = reinterpret_cast<TerrainApplication*>(glfwGetWindowUserPointer(window));
    // and set the resize flag to true
    app->framebufferResized = true;
}

//////////////////////
//
// Synchronisation
//
//////////////////////

void TerrainApplication::createSyncObjects() {
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

void TerrainApplication::mainLoop() {
    // compute the time elapsed since rendering began
    static auto startTime = std::chrono::high_resolution_clock::now();
    prevTime = std::chrono::high_resolution_clock::now();
    
    // loop keeps window open
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        // get the time before the drawing frame
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(std::chrono::high_resolution_clock::now() - prevTime).count();
        prevTime = std::chrono::high_resolution_clock::now();
        // process user input, returns 0 when we want to exit the application
        if (processKeyInput() == 0)
            break;
    
        // draw the frame
        drawFrame();
        // update the plane's position
        airplane.updatePosition(deltaTime);
    }
    vkDeviceWaitIdle(vkSetup.device);
}

//////////////////////
//
// Frame drawing, GUI setting and UI
//
//////////////////////

void TerrainApplication::drawFrame() {
    // will acquire an image from swap chain, exec commands in command buffer with images as attachments in the frameBuffer
    // return the image to the swap buffer. These tasks are started simultaneously but executed asynchronously.
    // However we want these to occur in sequence because each relies on the previous task success
    // For syncing can use semaphores or fences and coordinate operations by having one op signal another
    // op and another operation wait for a fence or semaphor to go from unsignaled to signaled.
    // we can access fence state with vkWaitForFences and not semaphores.
    // fences are mainly for syncing app with rendering op, use here to synchronise the frame rate
    // semaphores are for syncing ops within or across cmd queues. We want to sync queue op to draw cmds and presentation so pref semaphores here

    // at the start of the frame, make sure that the previous frame has finished which will signal the fence
    vkWaitForFences(vkSetup.device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

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

    // get the chunks to draw
    terrain.updateVisibleChunks(airplane.camera, tolerance);

    // record the geometry command buffer with the new indices
    recordGeometryCommandBuffer(imageIndex);

    // update the UI, may change at every frame so this is where it is recorded, unlike the geometry which executes the same commands 
    // hence it is recorded once at the start 
    setGUI();

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
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        // tell the swapchaindata to change whether to enable or disable the depth testing when recreating the pipeline
        framebufferResized = false;
        recreateVulkanData();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // after the frame is drawn and presented, increment current frame count (% loops around)
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void TerrainApplication::setGUI() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame(); // empty
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // for now just display the demo window
    //ImGui::ShowDemoWindow();

    ImGui::Begin("Terrain Options", nullptr, ImGuiWindowFlags_NoMove);
    ImGui::BulletText("Transforms:");
    ImGui::SliderFloat("x", &translateX, -500.0f, 500.0f);
    ImGui::SliderFloat("y", &translateY, -500.0f, 500.0f);
    ImGui::SliderFloat("z", &translateZ, -500.0f, 500.0f);
    ImGui::SliderFloat("x rot", &rotateX, -180.0f, 180.0f);
    ImGui::SliderFloat("y rot", &rotateY, -180.0f, 180.0f);
    ImGui::SliderFloat("z rot", &rotateZ, -180.0f, 180.0f);
    ImGui::SliderFloat("Scale:", &scale, 0.0f, 1.0f);
    ImGui::BulletText("Terrain:");
    ImGui::SliderFloat("Vertex stride:", &vertexStride, 1.0f, 20.0f);
    ImGui::SliderFloat("Angle tolerance:", &tolerance, 0.0f, 1.0f);
    ImGui::SliderFloat("Height scalar:", &heightScalar, 0.0f, 255.0f);
    ImGui::Checkbox("On GPU:", &onGPU);
    ImGui::Checkbox("Debug camera:", &debugCameraState);
    ImGui::Checkbox("Draw all terrain:", &applyBinning);
    ImGui::Text("Select map to load:");
    ImGui::BulletText("Info:");
    ImGui::Text("View direction:");
    ImGui::SameLine();
    ImGui::Text(viewDir.c_str());
    char buf[32]; // buffer for outputting info
    ImGui::Text("Vertices total: ");
    sprintf_s(buf, "%i", terrain.getNumVertices());
    ImGui::SameLine();
    ImGui::Text(buf);
    ImGui::Text("Polygons total: ");
    sprintf_s(buf, "%i", terrain.getNumPolygons());
    ImGui::SameLine();
    ImGui::Text(buf);
    ImGui::Text("Polygons drawn: ");
    sprintf_s(buf, "%i", terrain.getNumDrawnPolygons());
    ImGui::SameLine();
    ImGui::Text(buf);
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

int TerrainApplication::processKeyInput() {
    // special case return 0 to exit the program
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
        return 0;

    // debug camera on 
    if (debugCameraState) {
        if (glfwGetKey(window, GLFW_KEY_W))
            debugCamera.processInput(CameraMovement::PitchUp, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S))
            debugCamera.processInput(CameraMovement::PitchDown, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A))
            debugCamera.processInput(CameraMovement::RollLeft, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D))
            debugCamera.processInput(CameraMovement::RollRight, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q))
            debugCamera.processInput(CameraMovement::YawLeft, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E))
            debugCamera.processInput(CameraMovement::YawRight, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT))
            debugCamera.processInput(CameraMovement::Left, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_RIGHT))
            debugCamera.processInput(CameraMovement::Right, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)) {
            if (glfwGetKey(window, GLFW_KEY_UP))
                debugCamera.processInput(CameraMovement::Upward, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_DOWN))
                debugCamera.processInput(CameraMovement::Downward, deltaTime);
        }
        else {
            if (glfwGetKey(window, GLFW_KEY_UP))
                debugCamera.processInput(CameraMovement::Forward, deltaTime);
            if (glfwGetKey(window, GLFW_KEY_DOWN))
                debugCamera.processInput(CameraMovement::Backward, deltaTime);
        }


        // fake input to rotate the plane 
        // airplane.camera.processInput(CameraMovement::PitchDown, deltaTime);
    }
    // airplane camera is on
    else {
        if (glfwGetKey(window, GLFW_KEY_W))
            airplane.camera.processInput(CameraMovement::PitchUp, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S))
            airplane.camera.processInput(CameraMovement::PitchDown, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A))
            airplane.camera.processInput(CameraMovement::RollLeft, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D))
            airplane.camera.processInput(CameraMovement::RollRight, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_Q))
            airplane.camera.processInput(CameraMovement::YawLeft, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_E))
            airplane.camera.processInput(CameraMovement::YawRight, deltaTime);
    }

    // other wise just return true
    return 1;
}

//////////////////////
//
// Cleanup
//
//////////////////////

void TerrainApplication::cleanup() {
    // destroy the scene 
    airplane.destroyAirplane();
    terrain.destroyTerrain();
    for (auto& texture : textures) {
        texture.cleanupTexture();
    }

    // destroy the imgui context when the program ends
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // destroy whatever is dependent on the old swap chain
    vkFreeCommandBuffers(vkSetup.device, renderCommandPool, static_cast<uint32_t>(renderCommandBuffers.size()), renderCommandBuffers.data());
    vkFreeCommandBuffers(vkSetup.device, imGuiCommandPool, static_cast<uint32_t>(imGuiCommandBuffers.size()), imGuiCommandBuffers.data());
    
    // also destroy the uniform buffers that worked with the swap chain
    bTerrainUniforms.cleanupBufferData(vkSetup.device);
    bAirplaneUniforms.cleanupBufferData(vkSetup.device);

    // call the function we created for destroying the swap chain and frame buffers
    // in the reverse order of their creation
    framebufferData.cleanupFrambufferData();
    swapChainData.cleanupSwapChainData();

    // cleanup the descriptor pools and descriptor sets
    vkDestroyDescriptorPool(vkSetup.device, descriptorPool, nullptr);

    // destroy the descriptor layout
    for (size_t i = 0; i < descriptorSetLayouts.size(); i++) {
        vkDestroyDescriptorSetLayout(vkSetup.device, descriptorSetLayouts[i], nullptr);
    }

    // destroy the index buffers and free their memory
    bTerrainIndex.cleanupBufferData(vkSetup.device);
    bAirplaneIndex.cleanupBufferData(vkSetup.device);

    // destroy the vertex buffers and free their memory
    bTerrainVertex.cleanupBufferData(vkSetup.device);
    bAirplaneVertex.cleanupBufferData(vkSetup.device);


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

