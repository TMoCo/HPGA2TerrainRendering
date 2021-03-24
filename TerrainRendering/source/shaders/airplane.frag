#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Uniform
//

layout(binding = 0, std140) uniform UniformBufferObject {
	mat4 modl;
	mat4 view;
	mat4 proj;
} ubo;

//
// Texture
//

layout(binding = 1) uniform sampler2D texSampler; 

//
// Input from previous stage
//

layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec4 fragMaterial;
layout(location = 3) in vec2 fragTexCoord;
//
// Output
//

layout(location = 0) out vec4 outColor;

void main() {
	// do something
	outColor = texture(texSampler, fragTexCoord).rgba;
}