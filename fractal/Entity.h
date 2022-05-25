#pragma once

#include "nmath/vectors/Vector3f.h"

#include "cl_bindings_and_helpers.h"

struct Entity {
	nmath::Vector3f position;
	nmath::Vector3f rotation;
	nmath::Vector3f scale;
	cl_uint material;
};
