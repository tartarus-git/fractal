#pragma once

#include <new>

#include "Material.h"

class ResourceHeap {
public:
	Material* materialHeap;
	size_t materialHeapOffset;
	size_t materialHeapLength;

	constexpr ResourceHeap() = default;
	constexpr ResourceHeap(size_t materialHeapOffset, size_t materialHeapLength) : materialHeapOffset(materialHeapOffset), materialHeapLength(materialHeapLength) {
		materialHeap = new (std::nothrow) Material[materialHeapLength];
	}


	constexpr ResourceHeap& operator=(ResourceHeap&& right) {
		materialHeap = right.materialHeap;
		right.materialHeap = nullptr;
		materialHeapOffset = right.materialHeapOffset;
		materialHeapLength = right.materialHeapLength;
		return *this;
	}


																			// SIDE-NOTE: constexpr implementations need to be available in every translation unit that they are used in because the implementations are used in the main compilation step, which comes before normal, separate
																			// implementations would get linked together. That's why constexpr automatically means "inline", which allows you to define them directly in the header.
																			// Technically, the language could handle them like normal functions and have the implementations somewhere else, but that would require even more compilation than there already is after linking.
																			// Basically, it makes decent sense.
																			// ANOTHER SIDE-NOTE: Unnamed namespaces do the same thing as static. They just make it local to the translation unit.
	constexpr ~ResourceHeap() {
		delete[] materialHeap;												// NOTE: Safe to delete even if the materialHeap pointer ends up being nullptr, because delete and delete[] check for that case themselves.
	}
};
