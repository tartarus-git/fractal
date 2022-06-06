#pragma once

#include "nmath/vectors/Vector3f.h"

#include <cstdint>

struct KDTreeNode {
	uint64_t childrenIndex;					// dimension encoded in first two bits
	uint64_t parentIndex;
	uint32_t objectCount;
	float split;
	//char dimension;
};

struct KDTree {
	nmath::Vector3f position;
	alignas(16) nmath::Vector3f size;
	alignas(16) uint64_t objectCount;
	float split;
};
