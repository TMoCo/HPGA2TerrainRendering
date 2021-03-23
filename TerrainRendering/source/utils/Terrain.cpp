//
// Terrain class definition
//

#include <fstream>

#include <utils/Utils.h> // MAX_LINE
#include <utils/Terrain.h>


void Terrain::createTerrain(VulkanSetup* pVkSetup, const VkCommandPool& commandPool) {
	// load the texture as a grayscale
	heightMap.createTexture(pVkSetup, TERRAIN_HEIGHTS_PATH, commandPool, VK_FORMAT_R8G8B8A8_SRGB);
	hSize = static_cast<size_t>(heightMap.width < heightMap.height ? heightMap.width : heightMap.height);
	heights.resize(hSize * hSize); // resize vector just in case
	generateTerrainMesh();
	updateChuncks();
}

void Terrain::destroyTerrain() {
	heightMap.cleanupTexture();
}

bool Terrain::updateChuncks() {
	// loop through the chuncks 
	return false;
}

void Terrain::generateTerrainMesh() {
	if (heights.empty()) {
		throw std::runtime_error("No heights loaded, could not generate terrain!");
	}

	// by making the vector of vertices smaller, we can ignore the edge and corner cases heights
	// which is tolerable as the result is negligable on large data sets and saves some code branching
	size_t vSize, iSize;
	vSize = hSize;	// vertex vector size
	iSize = hSize - 1; // index vector size
	vertices.resize(vSize * vSize);
	indices.resize(iSize * iSize * 6);

	float stepScale = 2.0f; // we can change this later for scaling spacing between vertices
	// create the vertices, the first one is in the -z -x position, we use the index size to centre the plane
	glm::vec3 startPos(iSize / -2.0f, 0.0f, iSize / -2.0f);

	for (size_t row = 1; row < vSize - 1; row++) {
		for (size_t col = 1; col < vSize - 1; col++) {
			// initialise empty vertex and set its position
			Vertex vertex{};
			vertex.pos = startPos * stepScale + glm::vec3(col * stepScale, getHeight(row, col), row * stepScale);
			vertex.normal = computeCFD(row, col);
			// set the vertex in the vector directly
			vertices[row * vSize + col] = vertex;
			centreOfGravity += vertex.pos;
		}
	}
	// update the centre of gravity
	centreOfGravity /= static_cast<float>(vertices.size());

	// set the indices, looping over each grid cell (contains two triangles)
	for (size_t row = 0; row < iSize; row++) {
		for (size_t col = 0; col < iSize; col++) {
			// set the triangles in the grid cell
			size_t cellId = getCellIndex(row, col);
			// triangle 1
			indices[cellId + 0] = static_cast<uint32_t>(row * vSize + col);
			indices[cellId + 1] = static_cast<uint32_t>(row * vSize + col + vSize);
			indices[cellId + 2] = static_cast<uint32_t>(row * vSize + col + 1);
			// triangle 2
			indices[cellId + 3] = static_cast<uint32_t>(row * vSize + col + 1);
			indices[cellId + 4] = static_cast<uint32_t>(row * vSize + col + vSize);
			indices[cellId + 5] = static_cast<uint32_t>(row * vSize + col + vSize + 1);
		}
	}
}

void Terrain::generateChuncks() {
	// terrain's atomic unit is the grid cell, consisting of 4 vertices. Using the index row and col (iSize = hSize - 1), we can
	// get the indices that belong to a single cell. Cell indices are stored in a row major format, as can be seen in the generateMesh 
	// method. We need to loop over each individual grid cell and assign it to a chunk. We need a function that computes the chunck id
	// for a given grid cell index. Before that, we need to divide the terrain into a grid of chuncks. The simplest solution is 
	// to divide the grid evenly.


}

size_t Terrain::getCellIndex(size_t row, size_t col) {
	return (row * (hSize-1) + col) * 6;
}

size_t Terrain::getChunckIndex(size_t row, size_t col) {
}

void Terrain::loadHeights(const std::string& path) {
	// load in the ppm file
	// !! NB: We assume the file is rectangular (n x n) and all comments have been removed !!
	std::ifstream file(path);

	// check that the stream was succesfully opened
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	// ignore the first line (ppm magic value)
	file.ignore(MAX_SIZE, '\n');

	// get the file dimensions
	size_t vertRows, vertCols;
	file >> vertRows;
	file >> vertCols;
	// check that they are legal
	if ((vertRows <= 0) || (vertCols <= 0))
		throw std::runtime_error("invalid image dimensions!");
	// take the smallest of the image dimensions and use that as the size for a rectangular grid of heights
	hSize = (vertRows >= vertCols) ? vertCols : vertRows;

	// resize the heights vector accordingly
	heights.resize(hSize * hSize);

	// a value where we stream in unwanted values (such as the extra colour channels)
	int ignore;

	// ppm rgb range, which we don't need
	file >> ignore;

	// loop to read in the data. Gimp gave each line a single value, rather than a matrix like structure...
	size_t n = 0;
	while (n < heights.size()) {
		// stream in the value
		file >> heights[n];
		// we also need to ignore the next two (because we have a grayscale image in rgb format)
		file >> ignore;
		file >> ignore;
		// increment index
		n++;
	}
}

float Terrain::getHeight(const size_t& row, const size_t& col) {
	// return the element in the vertex grid at position row, col
	return heights[row * hSize + col];
}

glm::vec3 Terrain::computeCFD(const size_t& row, const size_t& col) {
	// get the neighbouring vertices' x and z average around the desired vertex
	glm::vec3 normal(
		(getHeight(row, col - 1) - getHeight(row, col + 1)) / 2.0f,
		255.0f,
		(getHeight(row - 1, col) - getHeight(row + 1, col)) / 2.0f);
	return normal;
}