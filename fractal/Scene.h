#pragma once

#include "Entity.h"

#include "Light.h"

#include <new>
#include <cstdint>

class Scene {
public:
	Entity* entityHeap;
	size_t entityHeapLength;

	Light* lights;
	uint64_t lightsLength;

	constexpr Scene() = default;
	constexpr Scene(size_t entityHeapLength, uint64_t lightsLength) : entityHeapLength(entityHeapLength), lightsLength(lightsLength) {
		entityHeap = new (std::nothrow) Entity[entityHeapLength];
		lights = new (std::nothrow) Entity[lightsLength];
	}

	constexpr Scene& operator=(Scene&& right) {
		entityHeap = right.entityHeap;
		right.entityHeap = nullptr;
		lights = right.lights;
		right.lights = nullptr;
		entityHeapLength = right.entityHeapLength;
		return *this;
	}

	constexpr ~Scene() { delete[] entityHeap; delete[] lights; }
};
