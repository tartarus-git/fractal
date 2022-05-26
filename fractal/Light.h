#pragma once

#include "nmath/vectors/Vector3f.h"

struct Light {
	nmath::Vector3f position;
	alignas(16) nmath::Vector3f color;
};
