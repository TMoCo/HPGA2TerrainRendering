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

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}