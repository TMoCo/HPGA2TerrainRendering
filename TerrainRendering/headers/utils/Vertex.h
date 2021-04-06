#ifndef VERTEX_H
#define VERTEX_H

#include <array> // array container

#include <glm/glm.hpp> // shader vectors, matrices ...

#include <vulkan/vulkan_core.h>

// a simple vertex struct

struct Vertex {

    // position 
    glm::vec3 pos; 
    // normal
    glm::vec3 normal; 
    // texture coordinate
    glm::vec2 texCoord; 


    // binding description of a vertex
    static VkVertexInputBindingDescription getBindingDescription() {
        // a struct containing info on how to store vertex data
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // index of binding in array of bindings
        bindingDescription.stride = sizeof(Vertex); // bytes in one entry
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // how to move the next data
        // other option for instancing -> VK_VERTEX_INPUT_RATE_INSTANCE
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        // we have two attributes: position and colour. The struct describes how to extract an attribute
        // from a chunk of vertex data from a binding description
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        // position (vec3)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        // normal (vec3)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        // tex coord (vec2)
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        return attributeDescriptions;
    }
};


#endif // !VERTEX_H
