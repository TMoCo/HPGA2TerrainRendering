//
// Terrain class definition
//

#include <fstream>

#include <utils/Utils.h> // MAX_LINE
#include <utils/Terrain.h>
#include <glm/gtx/string_cast.hpp>

void Terrain::createTerrain(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, uint32_t mapId) {
	// load the texture as a grayscale
	heightMap.createTexture(pVkSetup, TERRAIN_HEIGHTS_PATHS[mapId], commandPool, VK_FORMAT_R8_SRGB);
	hSize = heightMap.width < heightMap.height ? heightMap.width : heightMap.height;
	heights.resize(hSize * hSize); // resize vector just in case

	generateTerrainMesh();
	generateChunks();
	
	sortIndicesByChunk();
}

void Terrain::destroyTerrain() {
	heightMap.cleanupTexture();
}

void Terrain::updateVisibleChunks(const Camera& cam, float tolerance, float vertexStride, float aspectRatio) {
	visible.clear();
	// loop through the chunks, get those that are currently visible
	for (size_t i = 0; i < chunks.size(); i++) {
		chunks[i].centrePoint *= vertexStride;   // apply current vertex stride
		// check the angle between the direction from centre point to camera position, with the view direction
		glm::vec3 toCamera = glm::normalize(chunks[i].centrePoint - cam.position);
		
		// compute the dot product
		float dot = glm::dot(cam.orientation.front, toCamera);
		
		// depending on angle, put in visible or not
		if (dot > tolerance) {
			visible.insert(std::make_pair<int, Chunk*>(int(i), chunks.data() + i));
		}
	}
}

int Terrain::getNumVertices() {
	return hSize * hSize;
}

int Terrain::getNumPolygons() {
	return hSize * hSize * 6;
}

int Terrain::getNumDrawnPolygons() {
	return hSize * hSize * 6;
}

void Terrain::generateTerrainMesh() {
	if (heights.empty()) {
		throw std::runtime_error("No heights loaded, could not generate terrain!");
	}

	// by making the vector of vertices smaller, we can ignore the edge and corner cases heights
	// which is tolerable as the result is negligable on large data sets and saves some code branching
	int vSize, iSize;
	vSize = hSize;	// vertex vector size
	iSize = hSize - 1; // index vector size
	vertices.resize(vSize * vSize);
	indices.resize(iSize * iSize * 6);

	// create the vertices, the first one is in the -z -x position, we use the index size to centre the plane
	glm::vec3 startPos(vSize * -0.5f, 0.0f, vSize * -0.5f);

	for (int row = 0; row < vSize; row++) {
		for (int col = 0; col < vSize; col++) {
			// initialise empty vertex and set its position
			Vertex vertex{};
			vertex.pos = startPos + glm::vec3(col, getHeight(row, col), row);
			vertex.normal = computeCFD(row, col);
			// set the vertex in the vector directly
			vertices[row * vSize + col] = vertex;
			centreOfGravity += vertex.pos;
		}
	}

	// update the centre of gravity
	centreOfGravity /= static_cast<float>(vertices.size());

	sortIndicesByCell();
}

void Terrain::generateChunks() {
	// terrain's atomic unit is the grid cell, consisting of 4 vertices. Using the index row and col (iSize = hSize - 1), we can
	// get the indices that belong to a single cell. Cell indices are stored in a row major format, as can be seen in the generateMesh 
	// method. We need to loop over each individual grid cell and assign it to a chunk. We need a function that computes the chunk id
	// for a given grid cell index. Before that, we need to divide the terrain into a grid of chunks. The simplest solution is 
	// to divide the grid evenly. If the grid cannot be divided evenly, then the chunks in the last row and column will have a smaller number 
	// of indices.
	chunks.clear();

	int iSize = hSize - 1; // hsize should always be greater than 1

	// if the cells are not divisible by num chunks, add one to chunk size where remainders are accumulated
	if (!(iSize % numChunks)) {
		numChunks++;
	}

	// resize chunk grid container
	chunks.resize(numChunks * numChunks);

	// loop over each grid cell index
	for (int row = 0; row < iSize; row++) {
		for (int col = 0; col < iSize; col++) {
			// get the index of the chunk the grid cell belongs to
			int chunkIndex = getChunkIndexFromCellIndex(row, col);
			auto* indices = &(chunks[chunkIndex].indices); // pointer to the indices in a chunk
			auto cell = getIndicesCell(row, col);		   // contains the indices in a cell
			indices->insert(indices->end(), cell.begin(), cell.end()); // append the cell indices to chunk's indices.
		}
	}

	float chunkWidth = hSize / static_cast<float>(numChunks);		 // width of a chunk in vertex units
	glm::vec3 startPos((-static_cast<float>(hSize) + chunkWidth) * 0.5f, 0.0f, (-static_cast<float>(hSize) + chunkWidth) * 0.5f); // initial centre point
	
	uint32_t offset = 0;
	// each chunk now has its vertices, we can determine its offset and centrePoint
	for (int row = 0; row < numChunks; row++) {
		for (int col = 0; col < numChunks; col++) {
			auto chunk = getChunk(row, col);
			chunk->centrePoint += startPos + glm::vec3(chunkWidth * col, 0.0f, chunkWidth * row);
			chunk->chunkOffset = offset; // chunk offset used in draw commands
			offset += static_cast<uint32_t>(chunk->indices.size());
		}
	}
}

void Terrain::sortIndicesByCell() {
	size_t vSize = hSize;
	size_t iSize = hSize - 1;
	// set the indices, looping over each grid cell (contains two triangles)
	for (int row = 0; row < iSize; row++) {
		for (int col = 0; col < iSize; col++) {
			// set the triangles in the grid cell
			size_t cellId = getIndicesCellFirstIndex(row, col); // id of the first index in the grid cell
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

void Terrain::sortIndicesByChunk() {
	indices.clear();
	indices.resize((hSize-1) * (hSize-1) * 6);
	size_t currentIndex = 0;
	// loop over each chunk
	float chunkVal = 0;
	for (auto& chunk : chunks) {
		// in the chunk order, set the indices 
		for (size_t i = 0; i < chunk.indices.size(); i++) {
			indices[currentIndex] = chunk.indices[i];
			vertices[indices[currentIndex]].material = glm::vec4(chunkVal) / static_cast<float>(chunks.size());
			currentIndex++;
		}
		chunkVal++;
	}
}

int Terrain::getIndicesCellFirstIndex(int row, int col) {
	// take care of clamping values out of range for us
	row = row < 0 ? 0 : row >= hSize - 1 ? hSize - 2 : row;
	col = col < 0 ? 0 : col >= hSize - 1 ? hSize - 2 : col;
	// return the first index of a grid cell of size vertices
	return (row * (hSize-1) + col) * 6;
}

std::vector<uint32_t> Terrain::getIndicesCell(int row, int col) {
	int index = getIndicesCellFirstIndex(row, col);
	return std::vector<uint32_t>(std::next(indices.begin(), index), std::next(indices.begin(), index + 6));
}

int Terrain::getChunkIndexFromCellIndex(int row, int col) {
	// the inputs, row and col of a grid cell, are normalised to determine which chunk they are in 
	return (int)(row / (float)(hSize - 1) * numChunks) * numChunks // chunk row
			+ (int)(col / (float)(hSize - 1) * numChunks);		 // chunk col
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
	hSize = static_cast<int>(vertRows >= vertCols ? vertCols : vertRows);

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

float Terrain::getHeight(int row, int col) {
	// clamp values out of range
	row = row < 0 ? 0 : row >= hSize ? hSize - 1 : row;
	col = col < 0 ? 0 : col >= hSize ? hSize - 1 : col;
	// return the element in the vertex grid at position row, col
	return heights[row * hSize + col];
}

Chunk* Terrain::getChunk(int row, int col) {
	// !! does not check out of bounds !!
	return &(chunks[row * numChunks + col]);
}

glm::vec3 Terrain::computeCFD(int row, int col) {
	// get the neighbouring vertices' x and z average around the desired vertex
	glm::vec3 normal(
		(getHeight(row, col - 1) - getHeight(row, col + 1)) * 0.5f * 255.0f,
		1.0f,
		(getHeight(row - 1, col) - getHeight(row + 1, col)) * 0.5f * 255.0f);
	return normal;
}
