#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Uniform
//

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    int uvToRgb;
    int hasAmbient;
    int hasDiffuse;
    int hasSpecular;
} ubo;

layout(binding = 1) uniform sampler2D texSampler; // equivalent sampler1D and sampler3D

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

// light position
// NB we assume the light is white:
// const vec3 lightColour = vec3(1.0, 1.0, 1.0);
const vec3 lightPos = vec3(0, -30, 50);

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float specularExponent;
};

// material properties found in .mtl file
// const Material mallard = Material(vec3(0.1f, 0.1f, 0.1f), vec3(0.5f, 0.5f, 0.5f), vec3(0.7f, 0.7f, 0.7f), 38.0f);

void main()
{
    /*
    if (ubo.uvToRgb == 1) {
        outColor = vec4(fragTexCoord, 0.0, 1.0);
    }
    else {
        // the colour of the model without lighting
        vec3 color = texture(texSampler, fragTexCoord).rgb;
        // view direction, assumes eye is at the origin (which is the case)
        vec3 viewDir = normalize(-fragPos);
        // light direction from fragment to light
        vec3 lightDir = normalize(lightPos - fragPos);
        // reflect direction, reflection of the light direction by the fragment normal
        vec3 reflectDir = reflect(-lightDir, fragNormal);

        // ambient
        vec3 ambient = vec3(0,0,0);
        if (ubo.hasAmbient == 1)
            ambient = fragMaterial.xxx;

        // diffuse (lambertian)
        vec3 diffuse = vec3(0,0,0);
        if (ubo.hasDiffuse == 1) {
            float diff = max(dot(lightDir, fragNormal), 0.0f);
            diffuse = fragMaterial.yyy * diff;
        }

        // specular 
        vec3 specular = vec3(0,0,0);
        if (ubo.hasSpecular == 1) {
            float spec = pow(max(dot(viewDir, reflectDir), 0.0f), fragMaterial.w);
            specular = fragMaterial.zzz * spec;
        }


        // vector multiplication is element wise <3
        outColor = vec4((ambient + diffuse + specular) * color, 1.0f);
        //outColor = fragMaterial;

    }
    */
    //outColor = fragMaterial;
    outColor = vec4(fragNormal, 1.0f);
}