//
// Terrain class definition
//

#ifndef TERRAIN_H
#define TERRAIN_H

#include <string>
#include <vector>
#include <map>

#include <utils/Camera.h>  // Camera class
#include <utils/Model.h>   // Model class
#include <utils/Vertex.h>  // Vertex data structure
#include <utils/Texture.h> // height map as a Texture object

#include <utils/Chunk.h>

// inherits from Model for some member variables
class Terrain : public virtual Model {
public:

	Terrain() : numChunks(20), hSize(0) {}

	void createTerrain(VulkanSetup* pVkSetup, const VkCommandPool& commandPool);
	
	void destroyTerrain();

	void updateVisibleChunks(const Camera& cam, float tolerance, float vertexStride = 1.0f, float aspectRatio = 1.0f);

private:
	// load a terrain model from a greyscale ppm file 
	void generateTerrainMesh();

	// generate a set of chunks using the indices computed when creating a mesh
	void generateChunks();

	void sortIndicesByChunk();

	void sortIndicesByCell();

	// load a height map (greyscale ppm image)
	void loadHeights(const std::string& path);

	// returns a height at a given row and col, with offsets if specified
	float getHeight (int row, int col);

	Chunk* getChunk(int row, int col);

	// returns the first index of a grid cell
	int getIndicesCellFirstIndex(int row, int col);

	// return a vector contain the indices in a grid cell
	std::vector<uint32_t> getIndicesCell(int row, int col);

	// returns the index of a cunck from a grid cell index
	int getChunkIndexFromCellIndex(int row, int col);

	// compute the Central Finite Difference for a vertex at position row, col in the grid
	glm::vec3 computeCFD(int row, int col);

	glm::vec3 removeY(glm::vec3 vec);

public:
	// height data
	std::vector<float> heights;    // the heights
	std::vector<Chunk> chunks;	   // a vector containing the indices of the terrain that should be drawn

	std::map<int, Chunk*> visible; // a map containing all currently visible chunks
	
	int numChunks;		           // number of chunks
	int hSize;				   // the size of a row / column in the height map
	Texture heightMap;			   // the height map texture
};

#endif // !TERRAIN_H
