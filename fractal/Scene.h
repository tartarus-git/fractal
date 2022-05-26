#pragma once

#include "Entity.h"

#include "Light.h"

#include <new>
#include <cstdint>

class Scene {
public:
	Entity* entityHeap;
	size_t entityHeapLength;

	Light* lightHeap;
	uint64_t lightHeapLength;

	constexpr Scene() = default;
	constexpr Scene(size_t entityHeapLength, uint64_t lightHeapLength) : entityHeapLength(entityHeapLength), lightHeapLength(lightHeapLength) {
		entityHeap = new (std::nothrow) Entity[entityHeapLength];
		lightHeap = new (std::nothrow) Light[lightHeapLength];
	}

	constexpr Scene& operator=(Scene&& right) {
		entityHeap = right.entityHeap;
		right.entityHeap = nullptr;
		lightHeap = right.lightHeap;
		right.lightHeap = nullptr;
		entityHeapLength = right.entityHeapLength;
		lightHeapLength = right.lightHeapLength;
		return *this;
	}

	constexpr ~Scene() { delete[] entityHeap; delete[] lightHeap; }
};
