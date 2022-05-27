#pragma once

#include "nmath/vectors/Vector3f.h"

#include <cstdint>

struct KDTreeNode {
	float split;
	uint64_t childrenIndex;
	uint64_t parentIndex;
	uint64_t leaf;
};

struct KDTree {
	nmath::Vector3f position;
	nmath::Vector3f size;
	float split;
};
