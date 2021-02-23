//
// Terrain class definition
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>
#include <vector>

#include <utils/Model.h>
#include <utils/Vertex.h>

// inherits from Model for some member variables
class Terrain : public Model {
public:
	// load terrain from a greyscale ppm file for simplicity
	void loadTerrain(const std::string& path);

private:
	// returns a pointer to a vertex at a given row and col, with offsets if specified
	float getHeight(const size_t& row, const size_t& col, const std::vector<int>* heights);
	// compute the Central Finite Difference for a vertex at position row, col in the grid
	glm::vec3 computeCFD(const size_t& row, const size_t& col, const std::vector<int>* heights);

	// height data size
	size_t hSize = 0;
};

#endif // !TERRAIN_H
