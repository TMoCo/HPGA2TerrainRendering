//
// Main function for the application
//

// reporting and propagating exceptions
#include <iostream> 
#include <stdexcept>

#include <glm/gtx/string_cast.hpp>

#include <cstdlib> // EXIT_SUCCES & EXIT_FAILURE macros

// include the application definition
#include <TerrainApplication.h>

int main() {
    TerrainApplication app;

    glm::quat t = glm::angleAxis(0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    glm::quat u = glm::angleAxis(0.0f, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::quat v = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));

    std::cout << glm::to_string(t * u * v) << '\n';

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}