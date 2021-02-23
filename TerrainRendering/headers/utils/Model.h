//
// A model class for handling model related operations and data
//

#ifndef MODEL_H
#define MODEL_H

#include <utils/Vertex.h> // vertex struct

#include <string> // string for model path
#include <vector> // vector container

#include <vulkan/vulkan_core.h>

class Model {
public:
    // for now only loads in an obj file
	void loadModel(const std::string& path);

public:
    //
    // object data
    //

    glm::vec3 centreOfGravity;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

#endif // !MODEL_H
