//
// Terrain class definition
//

#include <fstream>

// image loading
//#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <utils/Utils.h> // MAX_LINE
#include <utils/Terrain.h>
#include <glm/gtx/string_cast.hpp>

void Terrain::createTerrain(VulkanSetup* pVkSetup, const VkCommandPool& commandPool, uint32_t mapId) {
	// load the texture as a grayscale
	heightMap.createTexture(pVkSetup, TERRAIN_HEIGHTS_PATHS[mapId], commandPool, VK_FORMAT_R8_SRGB);
	hSize = heightMap.width < heightMap.height ? heightMap.width : heightMap.height;

	loadHeights(TERRAIN_HEIGHTS_PATHS[mapId]);

	generateTerrainMesh();
	generateChunks();
	
	sortIndicesByChunk();
}

void Terrain::destroyTerrain() {
	heightMap.cleanupTexture();

}

void Terrain::updateVisibleChunks(const Camera& cam, float tolerance, float vertexStride, glm::mat4 terrainTransform) {
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
	return hSize * hSize; // square size of grid 
}

int Terrain::getNumPolygons() {
	return (hSize - 1) * (hSize - 1) * 2; // 2 triangles per grid cell
}

int Terrain::getNumDrawnPolygons() {
	// loop over chunks, cumulating indices size (some chunks have less indices)
	int indexCount = 0;
	for (auto& chunk : visible) {
		indexCount += chunk.second->indices.size();
	}
	return indexCount * 0.33333333333f; // two triangles every six indices, so divide by three
}

void Terrain::loadHeights(const std::string& path) {
	// load the file 
	int width, height, texChannels;

	// forces image to be loaded in grayscale, returns ptr to first element in an array of pixels
	stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &texChannels, 1);
	// no pixels loaded
	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}
	
	heights.resize(width * height);
	// loop over pixels and place in heights vector
	for (int i = 0; i < heights.size(); i++) {
		heights[i] = (float)*(pixels + i) / 255.0f;
	}
}

void Terrain::generateTerrainMesh() {
	// by making the vector of vertices smaller, we can ignore the edge and corner cases heights
	// which is tolerable as the result is negligable on large data sets and saves some code branching
	int vSize, iSize;
	vSize = hSize;	// vertex vector size
	iSize = hSize - 1; // index vector size
	vertices.resize(vSize * vSize);
	indices.resize(iSize * iSize * 6);

	// create the vertices, the first one is in the -z -x position, we use the index size to centre the plane
	glm::vec3 startPos(vSize * -0.5f, 0.0f, vSize * -0.5f);
	float invDim = 1.0f / heightMap.height;
	std::cout << hSize;
	for (int row = 0; row < vSize; row++) {
		for (int col = 0; col < vSize; col++) {
			// initialise empty vertex and set its position
			Vertex vertex{};
			vertex.pos = startPos + glm::vec3(col, getHeight(row, col), row);
			//vertex.normal = computeCFD(row, col);
			vertex.texCoord = glm::vec2(col * invDim, row * invDim);
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
			+ (int)(col / (float)(hSize - 1) * numChunks);		   // chunk col
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
		(getHeight(col, row + 1) - getHeight(col, row - 1)) * 0.5f,
		1.0f,
		(getHeight(row, col + 1) - getHeight(row, col - 1)) * 0.5f);
	return normal;
}
