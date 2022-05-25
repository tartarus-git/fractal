#pragma once

#include "nmath/vectors/Vector4f.h"

#include "cl_bindings_and_helpers.h"

struct Entity {
	nmath::Vector4f position;
	nmath::Vector4f rotation;
	nmath::Vector4f scale;
	//cl_uint material;
};
