#version 450
#extension GL_ARB_separate_shader_objects : enable

//
// Inputs from CPU
//

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

//
// Outputs to next shader stage
// 

// for our pipeline goes to the fragment stage
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

//
// Shader
//

vec2 getTexCoord(int row, int col) {
    return vec2(col * ubo.invMapDim, row * ubo.invMapDim);
}

vec3 getPosition(int row, int col) {
    return vec3(
        (col - (ubo.mapDim * 0.5f)) * ubo.vertexStride, 
        texture(heightSampler, getTexCoord(row, col)).r * ubo.heightScalar, 
        (row - (ubo.mapDim * 0.5f)) * ubo.vertexStride);
}

// main function, entry point to the shader
void main() {
    int id = gl_VertexIndex;

    // get the row and column the vertex belongs to
    int row = id / ubo.mapDim;
    int col = id % ubo.mapDim;

    // 1- compute the position from the vertex row and column in grid
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(getPosition(row, col), 1.0f);
    gl_Position = pos;
    fragPos = (ubo.model * vec4(getPosition(row, col), 1.0f)).xyz;

    // get the height values of neighbouring vertices
    float a = texture(heightSampler, getTexCoord(row - 1, col)).r; //  x neighbour (right)
    float b = texture(heightSampler, getTexCoord(row + 1, col)).r; // -x neighbour (left)
    float c = texture(heightSampler, getTexCoord(row, col - 1)).r; //  z neighbour (top)
    float d = texture(heightSampler, getTexCoord(row, col + 1)).r; // -z neighbour (bottom)
    
    // compute central finite difference, taking into account the terrain scalars
    vec3 normal = normalize(vec3( 
    0.5f * (b - a) * ubo.heightScalar,   // x
    1.0f * ubo.vertexStride,             // y
    0.5f * (d - c) * ubo.heightScalar)); // z

    // 2- transform normal 
    fragNormal = mat3(ubo.normal) * normal;

    // 3- compute the texture coordinates of the vertex
    fragTexCoord = getTexCoord(row, col);
}
