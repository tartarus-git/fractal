#pragma once

#include "nmath/vectors/Vector3f.h"

struct Camera {
	nmath::Vector3f position;
	nmath::Vector3f rotation;
	float FOV;

	Camera() = default;
	Camera(nmath::Vector3f position, nmath::Vector3f rotation, float FOV) : position(position), rotation(rotation), FOV(FOV) { }
};
