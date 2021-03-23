//
// Chunck class
//

#ifndef CHUNK_H
#define CHUNCK_H

#include <vector>

class Chunk {
public:
	Chunk(size_t nIndices) : indices(nIndices) {}

	std::vector<uint32_t> indices; // chunk's vertex indices
};

#endif // !CHUNK_H

