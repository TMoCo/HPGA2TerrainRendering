#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Uniform
//

layout(binding = 0, std140) uniform UniformBufferObject {
	vec4 lightPos;
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
layout(location = 2) in vec2 fragTexCoord;

//
// Output
//

layout(location = 0) out vec4 outColor;

// Custom materials for phong shading

struct mat {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float exponent;
};

mat chrome = mat (
    vec3(0.25f, 0.25f, 0.25f),
    vec3(0.4f, 0.4, 0.4f), 
    vec3(0.774597f, 0.774597f, 0.774597f), 
    0.1f);

void main() {
	// do something
	vec4 albedo = texture(texSampler, fragTexCoord).rgba;

    // view direction, assumes eye is at the origin (which is the case)
    vec3 viewDir = normalize(-fragPos);
    // light direction from fragment to light
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos);
    // reflect direction, reflection of the light direction by the fragment normal
    vec3 reflectDir = reflect(-lightDir, fragNormal);


    // diffuse (lambertian)
    float diff = max(dot(lightDir, fragNormal), 0.0f);
    vec3 diffuse = chrome.diffuse * diff;

    // specular 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), chrome.exponent);
    vec3 specular = chrome.specular * spec;


    // vector multiplication is element wise <3
    outColor = vec4(chrome.ambient + diffuse + specular, 1.0f) * albedo;
}