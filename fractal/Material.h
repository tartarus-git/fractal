#pragma once

#include "nmath/vectors/Vector3f.h"

#include <cstdint>

struct Material {
	nmath::Vector3f color;
	alignas(16) float reflectivity;
};
