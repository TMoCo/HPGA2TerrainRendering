//
// Terrain class definition
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>
#include <vector>

#include <utils/Model.h>   // Model class
#include <utils/Vertex.h>  // Vertex data structure
#include <utils/Texture.h> // height map as a Texture object

#include <utils/Chunk.h>

// inherits from Model for some member variables
class Terrain : public virtual Model {
public:

	Terrain() : numChuncks(4), hSize(0) {}

	void createTerrain(VulkanSetup* pVkSetup, const VkCommandPool& commandPool);
	
	void destroyTerrain();

	bool updateChuncks();

private:
	// load a terrain model from a greyscale ppm file 
	void generateTerrainMesh();

	// generate a set of chuncks using the indices computed when creating a mesh
	void generateChuncks();

	// load a height map (greyscale ppm image)
	void loadHeights(const std::string& path);

	// returns a pointer to a height at a given row and col, with offsets if specified
	float getHeight(const size_t& row, const size_t& col);

	// returns the first index of a grid cell
	size_t getCellIndex(size_t row, size_t col);

	// returns the index of a cunck from a grid cell index
	size_t getChunckIndex(size_t row, size_t col);

	// compute the Central Finite Difference for a vertex at position row, col in the grid
	glm::vec3 computeCFD(const size_t& row, const size_t& col);

public:
	// height data
	std::vector<float> heights;    // the heights
	std::vector<Chunk> chuncks;	   // a vector containing the indices of the terrain that should be drawn. Updated at every frame
	
	size_t numChuncks;			   // number of chuncks
	size_t hSize;				   // the size of a row / column in the height map
	Texture heightMap;			   // the height map texture
};

#endif // !TERRAIN_H
