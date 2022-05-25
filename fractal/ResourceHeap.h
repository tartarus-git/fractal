#pragma once

#include "Material.h"

class ResourceHeap {
public:
	Material* materialHeap;
	size_t materialHeapOffset;
	size_t materialHeapLength;

	constexpr ResourceHeap() = default;
	constexpr ResourceHeap(Material* materialHeap, size_t materialHeapOffset, size_t materialHeapLength) : materialHeap(materialHeap), materialHeapOffset(materialHeapOffset), materialHeapLength(materialHeapLength) { }
																			// SIDE-NOTE: constexpr is implicitly inline, which enables you to write the implementation directly in the headers. YOU DON'T HAVE TO DO THAT THOUGH, at least not in C++20.
																			// ANOTHER SIDE-NOTE: Unnamed namespaces do the same thing as static. They just make it local to the translation unit.
	constexpr ~ResourceHeap() {
		//delete[] materialHeap;
	}
};
