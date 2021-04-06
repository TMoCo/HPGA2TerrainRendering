#version 450
#extension GL_ARB_separate_shader_objects : enable

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    vec4 lightPos;
    mat4 model;
    mat4 view;
    mat4 proj;
    mat4 normal;
    float vertexStride;
    float heightScalar;
    int mapDim;
    float invMapDim;
} ubo;

// height from a texture
layout(binding = 1) uniform sampler2D heightSampler;

// inputs specified in the vertex buffer attributes
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

// the shader output, goes to the next stage in the pipeline (for our pipeline goes to the fragment stage)
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

vec4 applyParamsPos(vec3 pos) {
    return vec4(pos.x * ubo.vertexStride, pos.y * ubo.heightScalar, pos.z * ubo.vertexStride, 1.0f);
}

vec3 applyParamsNormal(vec3 normal) {
    return normalize(vec3(normal.x * ubo.heightScalar, normal.y, normal.z * ubo.heightScalar));
}

// main function, entry point to the shader
void main() {
    // do something
	vec4 pos = ubo.proj * ubo.view * ubo.model * applyParamsPos(inPosition);
	gl_Position = pos;

	fragPos = (ubo.model * applyParamsPos(inPosition)).xyz;
    fragNormal = mat3(ubo.normal) * applyParamsNormal(inNormal);
    fragTexCoord = inTexCoord;
}