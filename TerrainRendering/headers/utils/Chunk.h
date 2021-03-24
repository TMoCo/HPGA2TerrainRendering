//
// Chunck class
//

#ifndef CHUNK_H
#define CHUNCK_H

#include <vector>

class Chunk {
public:
	Chunk() : centrePoint(0.0f), chunkOffset(0) {}

	glm::vec3 centrePoint;

	std::vector<uint32_t> indices; // chunk's vertex indices
	uint32_t chunkOffset; // offset into the indices when sorted by chunk
};

#endif // !CHUNK_H

