#pragma once

#include "nmath/vectors/Vector3f.h"

struct Material {
	alignas(16) nmath::Vector3f color;
};
