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
	// load a terrain model from a greyscale ppm file 
	void generateTerrainMesh();

	// load a height map (greyscale ppm image)
	void loadHeights(const std::string& path);

	// returns a pointer to a height at a given row and col, with offsets if specified
	float getHeight(const size_t& row, const size_t& col);
	// compute the Central Finite Difference for a vertex at position row, col in the grid
	glm::vec3 computeCFD(const size_t& row, const size_t& col);

	// height data
	std::vector<float> heights; // the heights
	size_t hSize;				// the size of a row / column in the height map

};

#endif // !TERRAIN_H
