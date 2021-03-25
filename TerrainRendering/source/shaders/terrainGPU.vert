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

// the shader output, goes to the next stage in the pipeline (for our pipeline goes to the fragment stage)
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec4 fragMaterial;
layout(location = 3) out vec2 fragTexCoord;

// for now keep image dimension as a constant in the shader
const int mapDim = 500;

vec2 getTexCoord(int row, int col) {
    return vec2(col / float(mapDim), row / float(mapDim));
}

vec3 getPosition(int row, int col) {
    return vec3(
        (col - (mapDim / 2.0f)) * ubo.vertexStride, 
        texture(heightSampler, getTexCoord(row, col)).r * 255, 
        (row - (mapDim / 2.0f)) * ubo.vertexStride);
}

// main function, entry point to the shader
void main() {
    int id = gl_VertexIndex;

    // get the row and column the vertex belongs to
    int row = id / mapDim;
    int col = id % mapDim;

    // compute the texture coordinates of the vertex
    fragTexCoord = getTexCoord(row, col); // divide by grid dimension to get value from 0 to 1 

    // compute the position from the vertex row and column in grid
    vec4 pos = ubo.proj * ubo.view * ubo.model * vec4(getPosition(row, col), 1.0f);
    gl_Position = pos;
    fragPos = pos.xyz;

    // get the positions of neighbouring vertices
    vec3 a = getPosition(row, col + 1); //  x neigbhour
    vec3 b = getPosition(row, col - 1); // -x neigbhour
    vec3 c = getPosition(row + 1, col); //  z neigbhour
    vec3 d = getPosition(row - 1, col); // -z neigbhour

    vec3 k = (a + b) / 2.0f - pos.xyz;
    //k.y = abs(k.y); // keep y positive
    vec3 l = (c + d) / 2.0f - pos.xyz;
    //l.y = abs(l.y); 

    vec4 normal =  ubo.model * vec4(((k + l) / 2.0f), 1.0f);

    //fragNormal = vec3(0.0f, 1.0f, 0.0f); // point upwards for now

    //fragMaterial = vec4(texture(heightSampler, fragTexCoord).rgb, 1.0f) ;
    fragMaterial = vec4(normal.xyz, 1.0f) ;
}
