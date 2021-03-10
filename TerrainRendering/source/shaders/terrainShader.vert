#version 450
#extension GL_ARB_separate_shader_objects : enable

// the uniform buffer object
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    float vertexStride;
} ubo;

// inputs specified in the vertex buffer attributes
layout(location = 0) in float inHeight;

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
    // get the row the index belongs
    int row = id / mapDim;
    int col = id % mapDim;
    vec3 position = vec3((row - (mapDim / 2.0f)) * ubo.vertexStride, inHeight, (col - (mapDim / 2.0f)) * ubo.vertexStride);
    // compute the position from the vertex index
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(position, 1.0f);
    gl_Position = pos;
    fragPos = pos.xyz; // swizzle to get the vec3 xyz components of the shader

    //fragNormal = (ubo.model * vec4(inNormal, 1.0f)).xyz;
    fragNormal = vec3(0.0f, 1.0f, 0.0f); // point upwards for now

    fragMaterial = vec4(inHeight, inHeight, inHeight, 255) / 255;
    //fragMaterial = vec4(1.0f, float(id / 250000.0f), 1.0f, 0.0f);
    
    fragTexCoord = vec2(0.0f, 0.0f); // no textures just yet
}