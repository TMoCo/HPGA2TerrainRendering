#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Uniform
//

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    vec4 lightPos;
} ubo;

//layout(binding = 1) uniform sampler2D texSampler; // equivalent sampler1D and sampler3D

layout(binding = 2) uniform sampler2D textureSamplers[3];

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
    
mat rubber = mat (
    vec3(1.0, 1.0f, 1.0f), 
    vec3(0.05f, 0.05f, 0.05f),
    vec3(0.5f, 0.5f, 0.5f), 
    vec3(0.7, 0.7, 0.7), 
    7.8125f);

float getBlend(float height, float boundary, float range) {
    float normalised = (boundary - height) / range;
    return 1.0f / (1.0f + pow(0.005f, normalised));
}

void main()
{
    // the colour of the model without lighting
    float grass = getBlend(fragPos.y, 50.0f, 30.0f);
    float rock = getBlend(fragPos.y, 160.0f, 20.0f) - grass;
    float snow = 1.0f - (grass + rock);

    vec3 color = texture(textureSamplers[0], fragTexCoord).rgb * grass
        + texture(textureSamplers[1], fragTexCoord).rgb * rock
        + texture(textureSamplers[2], fragTexCoord).rgb * snow;

    //color = vec3(grass, rock, snow);

    // view direction, assumes eye is at the origin (which is the case)
    vec3 viewDir = normalize(-fragPos);
    // light direction from fragment to light
    vec3 lightDir = normalize(ubo.lightPos.xyz - fragPos);
    // reflect direction, reflection of the light direction by the fragment normal
    vec3 reflectDir = reflect(-lightDir, fragNormal);


    // diffuse (lambertian)
    float diff = max(dot(lightDir, fragNormal), 0.0f);
    vec3 diffuse = rubber.diffuse * diff;

    // specular 
    float spec = pow(max(dot(viewDir, reflectDir), 0.0f), rubber.exponent);
    vec3 specular = rubber.specular * spec;

    outColor = vec4((rubber.ambient + diffuse + specular) * color, 1.0f);
    //outColor = vec4(fragTexCoord, 0.0f, 1.0f);
    //outColor = vec4(color, 1.0f);
}