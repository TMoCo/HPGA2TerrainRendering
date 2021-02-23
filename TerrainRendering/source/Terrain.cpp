//
// Terrain class definition
//
#include <fstream>

#include <Utils.h> // MAX_LINE
#include <Terrain.h>

void Terrain::loadTerrain(const std::string& path) {
	// a vector of ints for the height data
	std::vector<int> heights;

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
	// take the smallest of the image dimensions and use that as the size to get the size of the height vector
	hSize = (vertRows >= vertCols) ? vertCols : vertRows;

	// resize the heights vector accordingly
	heights.resize(hSize * hSize);

	// a value where we stream in unwanted values (such as the extra colour channels)
	int ignore;

	// ppm rgb range, which we don't need
	file >> ignore;

	// loop to read in the data. Gimp gave each line a single value, rather than a matrix like structure
	size_t n = 0;
	while(n < heights.size()) {
		// stream in the value
		file >> heights[n];
		// we also need to ignore the next two (because we have a grayscale image in rgb format)
		file >> ignore;
		file >> ignore;
		// increment index
		n++;
	}

	// by making the vector of vertices smaller, we can ignore the edge and corner cases heights
	// which is tolerale as the result is negligable on large data sets and saves a lot of computation
	size_t vSize, iSize;
	vSize = hSize - 2;
	//vSize = hSize;
	//iSize = hSize - 1;
	iSize = hSize - 3;
	vertices.resize(vSize * vSize);
	indices.resize(iSize * iSize * 6);


	float stepScale = 2.0f; // we can change this later for scaling spacing between vertices
	// create the vertices, the first one is in the -z -x position, we use the index size to centre the plane
	glm::vec3 startPos(iSize / -2.0f, 0.0f, iSize / -2.0f);

	for (size_t row = 0; row < vSize; row++) {
		for (size_t col = 0; col < vSize; col++) {
			// initialise empty vertex and set its position
			Vertex vertex{};
			//vertex.pos = startPos + glm::vec3(col * xStride, getHeight(row, col), row * zStride);
			vertex.pos = startPos * stepScale + glm::vec3(col * stepScale, getHeight(row, col, &heights), row * stepScale);
			vertex.normal = computeCFD(row, col, &heights);
			// set the vertex in the vector directly
			vertices[row * vSize + col] = vertex;
			centreOfGravity += vertex.pos;
		}
	}
	// update the centre of gravity
	centreOfGravity /= static_cast<float>(vertices.size());

	// set the indices, looping over each grid cell
	for (size_t row = 0; row < iSize; row++) {
		for (size_t col = 0; col < iSize; col++) {
			// set the triangles in the grid cell
			indices[(row * iSize + col) * 6 + 0] = static_cast<uint32_t>(row * vSize + col);
			indices[(row * iSize + col) * 6 + 1] = static_cast<uint32_t>(row * vSize + col + vSize);
			indices[(row * iSize + col) * 6 + 2] = static_cast<uint32_t>(row * vSize + col + 1);

			indices[(row * iSize + col) * 6 + 3] = static_cast<uint32_t>(row * vSize + col + 1);
			indices[(row * iSize + col) * 6 + 4] = static_cast<uint32_t>(row * vSize + col + vSize);
			indices[(row * iSize + col) * 6 + 5] = static_cast<uint32_t>(row * vSize + col + vSize + 1);
		}
	}

#ifdef DEBUG
	for (const uint32_t& index : indices) {
		std::cout << index << '\n';
	}

	for (const Vertex& vertex : vertices) {
		std::cout << vertex.pos.x << ' ' << vertex.pos.y << ' ' << vertex.pos.z << '\n';
	}
#endif // DEBUG
}

float Terrain::getHeight(const size_t& row, const size_t& col, const std::vector<int>* heights) {
	// return the element in the vertex grid at position row, col
	return *(heights->data() + (row+1) * hSize + col + 1);
}

glm::vec3 Terrain::computeCFD(const size_t& row, const size_t& col, const std::vector<int>* heights) {
	// get the neighbouring vertices' x and z average around the desired vertex
	glm::vec3 normal(
		(getHeight(row, col - 1, heights) - getHeight(row, col + 1, heights)) / 2.0f,
		1.0f,
		(getHeight(row - 1, col, heights) - getHeight(row + 1, col, heights)) / 2.0f);
	return normal;
}