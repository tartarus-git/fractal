#pragma once

#include "Entity.h"

#include "Light.h"

#include "KDTree.h"

#include <new>
#include <cstdint>

#include <vector>

class Scene {
public:
	Entity* entityHeap;
	size_t entityHeapLength;

	KDTree kdTree;
	std::vector<KDTreeNode> kdTreeNodes;

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

	uint64_t* xList;
	uint64_t* yList;
	uint64_t* zList;

	uint64_t* list;

	void createSortedLists() {
		xList = new (std::nothrow) uint64_t[entityHeapLength];
		yList = new (std::nothrow) uint64_t[entityHeapLength];
		zList = new (std::nothrow) uint64_t[entityHeapLength];

		list = new (std::nothrow) uint64_t[entityHeapLength * 3];

		for (int i = 0; i < entityHeapLength; i++) {
			xList[i] = i;
			yList[i] = i;
			zList[i] = i;
		}

		uint64_t* List = xList;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.x + entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.x + entityHeap[List[i + 1]].scale.x) {
					uint64_t temp = List[i];
					List[i] = List[i + 1];
					List[i + 1] = temp;
					neededToSort = true;
				}
			}
			if (!neededToSort) { break; }
		}

		List = yList;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.y + entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.y + entityHeap[List[i + 1]].scale.x) {
					uint64_t temp = List[i];
					List[i] = List[i + 1];
					List[i + 1] = temp;
					neededToSort = true;
				}
			}
			if (!neededToSort) { break; }
		}

		List = zList;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.z + entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.z + entityHeap[List[i + 1]].scale.x) {
					uint64_t temp = List[i];
					List[i] = List[i + 1];
					List[i + 1] = temp;
					neededToSort = true;
				}
			}
			if (!neededToSort) { break; }
		}

		// interleave lists:

		for (int i = 0; i < entityHeapLength; i++) {
			list[i * 3 + 0] = xList[i];
			list[i * 3 + 1] = yList[i];
			list[i * 3 + 2] = zList[i];
		}
	}

	std::vector<uint64_t> leafObjectHeap;

	uint64_t xLimit;
	uint64_t xLimitRange;

	uint64_t yLimit;
	uint64_t yLimitRange;

	uint64_t zLimit;
	uint64_t zLimitRange;

	// TODO: dimensions might need to be inside the thing to make dimension skipping possible where it is necessary (just the leaves AFAIK).

	template <typename lambda>
	void doThisThingForEveryObjectInRange(lambda thing) {
		for (int i = xLimit; i < xLimit + xLimitRange; i++) {
			if (entityHeap[xList[i]].position.y - entityHeap[xList[i]].scale.x < entityHeap[yList[yLimit]].position.y - entityHeap[yList[yLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.y + entityHeap[xList[i]].scale.x > entityHeap[yList[yLimit + yLimitRange]].position.y + entityHeap[yList[yLimit + yLimitRange]].scale.x) { break; }
			if (entityHeap[xList[i]].position.z - entityHeap[xList[i]].scale.x < entityHeap[zList[zLimit]].position.z - entityHeap[zList[zLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.z + entityHeap[xList[i]].scale.x > entityHeap[zList[zLimit + zLimitRange]].position.z + entityHeap[zList[zLimit + zLimitRange]].scale.x) { break; }
			if (entityHeap[xList[i]].position.x - entityHeap[xList[i]].scale.x < entityHeap[xList[xLimit]].position.x - entityHeap[xList[xLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.x + entityHeap[xList[i]].scale.x > entityHeap[xList[xLimit + xLimitRange]].position.x + entityHeap[xList[xLimit + xLimitRange]].scale.x) { break; }
			int hi = thing();
		}
	}

	void generateKDTreeNode(uint64_t parentIndex, nmath::Vector3f boxPos, nmath::Vector3f boxSize, char dimension) {
		kdTreeNodes.push_back(KDTreeNode());
		switch (dimension) {
		case 0:
			kdTreeNodes.back().split = boxSize.x / 2;
			kdTreeNodes.back().parentIndex = parentIndex;
			kdTreeNodes.back().leaf = 0;
			uint64_t leftAmount = 0;
			doThisThingForEveryObjectInRange([&amount = leftAmount]() { amount++; });
			uint64_t rightAmount = 0;
			doThisThingForEveryObjectInRange([&amount = rightAmount]() { amount++; });
			if (leftAmount == rightAmount) {				// We're at a leaf.
				kdTreeNodes.back().childrenIndex = leafObjectHeap.size();
				kdTreeNodes.back().leaf = rightAmount;
				for (int i = xLimit; i < xLimit + xLimitRange; i++) {
					if (entityHeap[xList[i]].position.y - entityHeap[xList[i]].scale.x < entityHeap[yList[yLimit]].position.y - entityHeap[yList[yLimit]].scale.x) { break; }
					if (entityHeap[xList[i]].position.y + entityHeap[xList[i]].scale.x > entityHeap[yList[yLimit]].position.y + entityHeap[yList[yLimit]].scale.x) { break; }
					if (entityHeap[xList[i]].position.z - entityHeap[xList[i]].scale.x < entityHeap[zList[zLimit]].position.z - entityHeap[zList[zLimit]].scale.x) { break; }
					if (entityHeap[xList[i]].position.z + entityHeap[xList[i]].scale.x > entityHeap[zList[zLimit]].position.z + entityHeap[zList[zLimit]].scale.x) { break; }

					if (entityHeap[xList[i]].position.x - entityHeap[xList[i]].scale.x > boxPos.x && entityHeap[xList[i]].position.x + entityHeap[xList[i]].scale.x < boxPos.x + boxSize.x) {
						leafObjectHeap.push_back(xList[i]);
					}
				}
				return;
			}
			generateKDTreeNode(kdTreeNodes.size() - 1, nmath::Vector3f(boxPos.x + kdTreeNodes.back().split, boxPos.y, boxPos.z), nmath::Vector3f(boxSize.x / 2, boxSize.y, boxSize.z), (dimension + 1) % 3);
			break;
		}
	}

	void generateKDTree() {
		for (uint64_t i = 0; i < entityHeapLength; i++) {
			float entityLowestValue = entityHeap[i].position.x - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.x) { kdTree.position.x = entityLowestValue; }
			entityLowestValue = entityHeap[i].position.y - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.y) { kdTree.position.y = entityLowestValue; }
			entityLowestValue = entityHeap[i].position.z - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.z) { kdTree.position.z = entityLowestValue; }
		}
		kdTree.split = kdTree.size.x / 2;
		while (true) {
			kdTreeNodes.push_back(KDTreeNode());
		}
	}




	constexpr ~Scene() { delete[] entityHeap; delete[] lightHeap; }
};
