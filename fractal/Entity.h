#pragma once

#include "nmath/vectors/Vector4f.h"

#include "cl_bindings_and_helpers.h"

enum class EntityType {
	SPHERE
};

struct Entity {
	nmath::Vector3f position;
	alignas(16) nmath::Vector3f rotation;
	alignas(16) nmath::Vector3f scale;
	alignas(16) cl_uint type;
	cl_uint material;
};
