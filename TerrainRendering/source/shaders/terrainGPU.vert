#version 450
#extension GL_ARB_separate_shader_objects : enable

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
    float vertexStride;
} ubo;

// height from a texture
layout(binding = 1) uniform sampler2D heightSampler;

// inputs specified in the vertex buffer attributes
/*
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inMaterial;
layout(location = 3) in vec2 inTexCoord;
*/

// the shader output, goes to the next stage in the pipeline (for our pipeline goes to the fragment stage)
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragMaterial;
layout(location = 3) out vec2 fragTexCoord;

// for now keep image dimension as a constant in the shader
const int mapDim = 500;

// main function, entry point to the shader
void main() {
    int id = gl_VertexIndex;

    // get the row and column the vertex belongs to
    int row = id / mapDim;
    int col = id % mapDim;

    // compute the texture coordinates of the vertex
    fragTexCoord = vec2(col / float(mapDim), row / float(mapDim)); // divide by grid dimension to get value from 0 to 1 

    // compute the position from the vertex row and column in grid
    vec3 position = vec3(
        (col - (mapDim / 2.0f)) * ubo.vertexStride, 
        texture(heightSampler, fragTexCoord).r * 255, 
        (row - (mapDim / 2.0f)) * ubo.vertexStride);

    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0f);
    gl_Position = pos;
    fragPos = pos.xyz; // swizzle to get the vec3 xyz components of the shader

    fragNormal = vec3(0.0f, 1.0f, 0.0f); // point upwards for now

    fragMaterial = vec4(texture(heightSampler, fragTexCoord).rgb, 1.0f) ;
    //fragMaterial = vec4(inMaterial.x, 1.0f - inMaterial.x,vec2(1.0f));

}