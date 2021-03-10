#version 450
#extension GL_ARB_separate_shader_objects : enable

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

// inputs specified in the vertex buffer attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inMaterial;
layout(location = 3) in vec2 inTexCoord;

// the shader output, goes to the next stage in the pipeline (for our pipeline goes to the fragment stage)
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragMaterial;
layout(location = 3) out vec2 fragTexCoord;

// main function, entry point to the shader
void main() {
    // gl_position is a keyword 
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    gl_Position = pos;
    fragPos = pos.xyz; // swizzle to get the vec3 xyz components of the shader
    // simply pass along the vertex colour and texture coordinate
    fragNormal = (ubo.model * vec4(inNormal, 1.0f)).xyz;
    fragMaterial = vec4(1.0f, float(gl_VertexIndex / 250000.0f), 1.0f, 0.0f); //inMaterial;
    fragTexCoord = inTexCoord;
}