#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Uniform
//

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    vec4 lightPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler; // equivalent sampler1D and sampler3D

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
    vec3 albedo;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float exponent;
};

mat bronze = mat (
    vec3(0.801f, 0.49609f, 0.1953f),
    vec3(0.19125f, 0.0735f, 0.0225f), 
    vec3(0.7038f, 0.27048f, 0.0828f), 
    vec3(0.256777f, 0.137622f, 0.086014f), 
    0.1f);

void main()
{
    // the colour of the model without lighting (
    vec3 color = bronze.albedo; //texture(texSampler, fragTexCoord).rgb;
    // view direction, assumes eye is at the origin (which is the case)
    vec3 viewDir = normalize(-fragPos);
    // light direction from fragment to light
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos);
    // reflect direction, reflection of the light direction by the fragment normal
    vec3 reflectDir = reflect(-lightDir, fragNormal);


    // diffuse (lambertian)
    float diff = max(dot(lightDir, fragNormal), 0.0f);
    vec3 diffuse = bronze.diffuse * diff;

    // specular 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), bronze.exponent);
    vec3 specular = bronze.specular * spec;


    // vector multiplication is element wise <3
    outColor = vec4((bronze.ambient + diffuse + specular) * color, 1.0f);
    //outColor = vec4(fragNormal, 1.0f);
}