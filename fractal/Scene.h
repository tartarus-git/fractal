#pragma once

#include "Entity.h"

#include <cstdint>

class Scene {
public:
	Entity* entities;
	size_t length;

	constexpr Scene() = default;
	constexpr Scene(Entity* entities, size_t length) : entities(entities), length(length) { }

	constexpr Scene& operator=(Scene&& right) {
		entities = right.entities;
		right.entities = nullptr;
		length = right.length;
		return *this;
	}

	constexpr ~Scene() { delete[] entities; }				// TODO: Make sure delete[] doesn't do anything for nullptr. It doesn't though, I'm pretty sure.
};
