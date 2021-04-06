//
// Definition of the model class
//

#include <string> // string class
#include <utils/Model.h> // model class declaration

// model loading
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

void Model::loadModel(const std::string& path) {
    // setup variables to get model info
    tinyobj::attrib_t attrib; // contains all the positions, normals, textures and faces
    std::vector<tinyobj::shape_t> shapes; // all the separate objects and their faces
    std::vector<tinyobj::material_t> materials; // object materials
    std::string warn, err;

    // load the model, show error if not
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    // combine all the shapes into a single model
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            // set vertex data
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };
            // add to the centre of gravity
            centreOfGravity += vertex.pos;

            vertices.push_back(vertex);
            indices.push_back(indices.size());
        }
    }
    // now compute the centre by dividing by the number of vertices in the model
    centreOfGravity /= (float)vertices.size();
}

