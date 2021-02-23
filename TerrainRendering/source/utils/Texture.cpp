//
// Texture class definition
//

#include <utils/Texture.h>

#include <utils/Utils.h> // utils namespace

// image loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

//////////////////////
//
// Create and destroy the texture data
//
//////////////////////

void Texture::createTexture(VulkanSetup* pVkSetup, const std::string& path, const VkCommandPool& commandPool) {
    vkSetup = pVkSetup;
    // create the image and its memory
    createTextureImage(path, commandPool);
    // create the image view
    textureImageView = utils::createImageView(&vkSetup->device, textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    // create the sampler
    createTextureSampler();
}

void Texture::cleanupTexture() {
    // destroy the texture image view and sampler
    vkDestroySampler(vkSetup->device, textureSampler, nullptr);
    vkDestroyImageView(vkSetup->device, textureImageView, nullptr);

    // destroy the texture image and its memory
    vkDestroyImage(vkSetup->device, textureImage, nullptr);
    vkFreeMemory(vkSetup->device, textureImageMemory, nullptr);
}

//////////////////////
//
// Texture image and sampler
//
//////////////////////

void Texture::createTextureImage(const std::string& path, const VkCommandPool& commandPool) {
    // uses command buffer so should be called after create command pool
    int texWidth, texHeight, texChannels;

    // load the file 
    // forces image to be loaded with an alpha channel, returns ptr to first element in an array of pixels
    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    // laid out row by row with 4 bytes by pixel in case of STBI_rgb_alpha
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    // create a staging buffer in host visible memory so we can map it, not device memory although that is our destination
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    utils::createBuffer(&vkSetup->device, &vkSetup->physicalDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    // directly copy the pixels in the array from the image loading library to the buffer
    void* data;
    vkMapMemory(vkSetup->device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vkSetup->device, stagingBufferMemory);

    // and cleanup pixels after copying in the data
    stbi_image_free(pixels);

    // now create the image
    CreateImageData info{};
    info.width = texWidth;
    info.height = texHeight;
    info.format = VK_FORMAT_R8G8B8A8_SRGB;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    info.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    info.image = &textureImage;
    info.imageMemory = &textureImageMemory;
    utils::createImage(&vkSetup->device, &vkSetup->physicalDevice, info);

    // next step is to copy the staging buffer to the texture image using our helper functions
    TransitionImageLayoutData transitionData{};
    transitionData.image = &textureImage;
    transitionData.renderCommandPool = commandPool;
    transitionData.format = VK_FORMAT_R8G8B8A8_SRGB;
    transitionData.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    transitionData.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    utils::transitionImageLayout(&vkSetup->device, &vkSetup->graphicsQueue, transitionData); // specify the initial layout VK_IMAGE_LAYOUT_UNDEFINED
    utils::copyBufferToImage(&vkSetup->device, &vkSetup->graphicsQueue, commandPool, stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    // need another transfer to give the shader access to the texture
    transitionData.image = &textureImage;
    transitionData.renderCommandPool = commandPool;
    transitionData.format = VK_FORMAT_R8G8B8A8_SRGB;
    transitionData.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    transitionData.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    utils::transitionImageLayout(&vkSetup->device, &vkSetup->graphicsQueue, transitionData);

    // cleanup the staging buffer and its memory
    vkDestroyBuffer(vkSetup->device, stagingBuffer, nullptr);
    vkFreeMemory(vkSetup->device, stagingBufferMemory, nullptr);
}

void Texture::createTextureSampler() {
    // configure the sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // how to interpolate texels that are magnified or minified
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // addressing mode
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    // VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : Take the color of the edge closest to the coordinate beyond the image dimensions.
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE : Like clamp to edge, but instead uses the edge opposite to the closest edge.
    // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER : Return a solid color when sampling beyond the dimensions of the image

    samplerInfo.anisotropyEnable = VK_TRUE; // use unless performance is a concern (IT WILL BE)
    VkPhysicalDeviceProperties properties{}; // can query these here or at beginning for reference
    vkGetPhysicalDeviceProperties(vkSetup->physicalDevice, &properties);
    // limites the amount of texel samples that can be used to calculate final colours, obtain from the device properties
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    // self ecplanatory, can't be an arbitrary colour
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE; // which coordinate system we want to use to address texels! usually always normalised
    // if comparison enabled, texels will be compared to a value and result is used in filtering (useful for shadow maps)
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    // mipmapping fields
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    // now create the configured sampler
    if (vkCreateSampler(vkSetup->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}