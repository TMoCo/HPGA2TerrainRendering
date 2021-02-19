//
// FramebufferData class definition
//

#include <FramebufferData.h>

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>
#include <array> // array container

//////////////////////
//
// Create and destroy the framebuffer data
//
//////////////////////

void FramebufferData::initFramebufferData(VulkanSetup* pVkSetup, const SwapChainData* swapChainData, const VkCommandPool& commandPool) {
    // update the pointer to the setup data rather than passing as argument to functions
    vkSetup = pVkSetup;
    // first create the depth resource
    depthResource.createDepthResource(vkSetup, swapChainData->extent, commandPool);
    // then create the framebuffers
    createFrameBuffers(swapChainData);
    createImGuiFramebuffers(swapChainData);
}

void FramebufferData::cleanupFrambufferData() {
    // destroy the depth image and related data (view and free memory)
    vkDestroyImageView(vkSetup->device, depthResource.depthImageView, nullptr);
    vkDestroyImage(vkSetup->device, depthResource.depthImage, nullptr);
    vkFreeMemory(vkSetup->device, depthResource.depthImageMemory, nullptr);
    // then desroy the frame buffers
    for (size_t i = 0; i < framebuffers.size(); i++) {
        vkDestroyFramebuffer(vkSetup->device, framebuffers[i], nullptr);
        vkDestroyFramebuffer(vkSetup->device, imGuiFramebuffers[i], nullptr);
    }
}

//////////////////////
//
// The framebuffers
//
//////////////////////

void FramebufferData::createFrameBuffers(const SwapChainData* swapChainData) {
    // resize the container to hold all the framebuffers, or image views, in the swap chain
    framebuffers.resize(swapChainData->imageViews.size());

    // now loop over the image views and create the framebuffers, also bind the image to the attachment
    for (size_t i = 0; i < swapChainData->imageViews.size(); i++) {
        // get the attachment 
        std::array<VkImageView, 2> attachments = {
            swapChainData->imageViews[i],
            depthResource.depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = swapChainData->renderPass; // which renderpass the framebuffer needs, only one at the moment
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // the number of attachments, or VkImageView objects, to bind to the buffer
        framebufferInfo.pAttachments = attachments.data(); // pointer to the attachment(s)
        framebufferInfo.width = swapChainData->extent.width; // specify dimensions of framebuffer depending on swapchain dimensions
        framebufferInfo.height = swapChainData->extent.height;
        framebufferInfo.layers = 1; // single images so only one layer

        // attempt to create the framebuffer and place in the framebuffer container
        if (vkCreateFramebuffer(vkSetup->device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void FramebufferData::createImGuiFramebuffers(const SwapChainData* swapChainData) {
    // resize the container to hold all the framebuffers, or image views, in the swap chain
    imGuiFramebuffers.resize(swapChainData->imageViews.size());

    for (size_t i = 0; i < swapChainData->imageViews.size(); i++)
    {
        //VkImageView attachment = swapChainData->imageViews[i];
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = swapChainData->imGuiRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &swapChainData->imageViews[i]; // only the image view as attachment needed
        framebufferInfo.width = swapChainData->extent.width;
        framebufferInfo.height = swapChainData->extent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vkSetup->device, &framebufferInfo, nullptr, &imGuiFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

