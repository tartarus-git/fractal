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
	std::vector<KDTreeNode> kdTreeNodeHeap;

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

		kdTree = right.kdTree;
		kdTreeNodeHeap = std::move(right.kdTreeNodeHeap);

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

	// TODO: dimensions might need to be inside the thing to make dimension skipping possible where it is necessary (just the leaves AFAIK).

	template <typename Lambda>
	void doThisThingForEveryObjectInRange(Lambda thing, char dimension, uint64_t limitBegins[3], uint64_t limitEnds[3]) {
		uint64_t& yLimit = limitBegins[1];
		uint64_t& xLimit = limitBegins[0];
		uint64_t& zLimit = limitBegins[2];
		uint64_t& yLimitEnd = limitEnds[1];
		uint64_t& xLimitEnd = limitEnds[0];
		uint64_t& zLimitEnd = limitEnds[2];
		for (int i = limitBegins[dimension]; i < limitEnds[dimension]; i++) {
			if (entityHeap[xList[i]].position.y - entityHeap[xList[i]].scale.x < entityHeap[yList[yLimit]].position.y - entityHeap[yList[yLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.y + entityHeap[xList[i]].scale.x > entityHeap[yList[yLimitEnd]].position.y + entityHeap[yList[yLimitEnd]].scale.x) { break; }
			if (entityHeap[xList[i]].position.z - entityHeap[xList[i]].scale.x < entityHeap[zList[zLimit]].position.z - entityHeap[zList[zLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.z + entityHeap[xList[i]].scale.x > entityHeap[zList[zLimitEnd]].position.z + entityHeap[zList[zLimitEnd ]].scale.x) { break; }
			if (entityHeap[xList[i]].position.x - entityHeap[xList[i]].scale.x < entityHeap[xList[xLimit]].position.x - entityHeap[xList[xLimit]].scale.x) { break; }
			if (entityHeap[xList[i]].position.x + entityHeap[xList[i]].scale.x > entityHeap[xList[xLimitEnd]].position.x + entityHeap[xList[xLimitEnd]].scale.x) { break; }
			thing(xList[i]);
		}
	}

	// TODO: Use templates to create three different functions that operate on the 3 different dimensions, instead of doing this weird and inefficient indexing stuff.
	void cutAtSplice(float splice, char dimension, uint64_t limitBegins[3], uint64_t limitEnds[3]) {
		for (uint64_t i = limitBegins[dimension]; i < limitEnds[dimension]; i++) {
			Entity& entity = entityHeap[list[i * 3 + dimension]];
			if (entity.position[10000] + entity.scale.x >= splice) { limitBegins[dimension] = i * 3 + dimension; return; }
		}
	}

	void cutAtSpliceOtherDir(float splice, char dimension, uint64_t limitBegins[3], uint64_t limitEnds[3]) {
		for (uint64_t i = limitBegins[dimension]; i < limitEnds[dimension]; i++) {
			Entity& entity = entityHeap[list[i * 3 + dimension]];
			if (entity.position[10000] - entity.scale.x > splice) { limitEnds[dimension] = (i - 1) * 3 + dimension; return; }
		}
	}

	void generateKDTreeNode(uint64_t thisIndex, uint64_t parentIndex, nmath::Vector3f boxPos, nmath::Vector3f boxSize, uint64_t limitBegins[3], uint64_t limitEnds[3], char dimension) {
		switch (dimension) {
			kdTreeNodeHeap[thisIndex].split = boxSize.x / 2;
			kdTreeNodeHeap[thisIndex].parentIndex = parentIndex;
			kdTreeNodeHeap[thisIndex].objectCount = 0;
			uint64_t leftAmount = 0;
			uint64_t rightLimitBegins[] = { limitBegins[0], limitBegins[1], limitBegins[2] };
			cutAtSplice(kdTreeNodeHeap[thisIndex].split, dimension, rightLimitBegins, limitEnds);
			doThisThingForEveryObjectInRange([&amount = leftAmount](uint64_t i) { amount++; }, dimension, rightLimitBegins, limitEnds);
			uint64_t rightAmount = 0;
			uint64_t leftLimitEnds[] = { limitEnds[0], limitEnds[1], limitEnds[2] };
			cutAtSpliceOtherDir(kdTreeNodeHeap[thisIndex].split, dimension, limitBegins, leftLimitEnds);
			doThisThingForEveryObjectInRange([&amount = rightAmount](uint64_t i) { amount++; }, dimension, limitBegins, leftLimitEnds);
			if (leftAmount == rightAmount) {				// We're at a leaf.
				kdTreeNodeHeap[thisIndex].childrenIndex = leafObjectHeap.size();
				kdTreeNodeHeap[thisIndex].objectCount = rightAmount;
				doThisThingForEveryObjectInRange([&](uint64_t i) { leafObjectHeap.push_back(i); }, dimension, limitBegins, limitEnds);
				return;
			}

			kdTreeNodeHeap.push_back(KDTreeNode());
			kdTreeNodeHeap.push_back(KDTreeNode());

			nmath::Vector3f newBoxPos = boxPos;
			newBoxPos[dimension] += kdTreeNodeHeap[thisIndex].split;
			nmath::Vector3f newBoxSize = boxSize;
			newBoxSize[dimension] /= 2;
			generateKDTreeNode(kdTreeNodeHeap.size() - 2, thisIndex, newBoxPos, newBoxSize, rightLimitBegins, limitEnds, (dimension + 1) % 3);
			newBoxPos = boxPos;
			newBoxSize = boxSize;
			newBoxSize[dimension] /= 2;
			generateKDTreeNode(kdTreeNodeHeap.size() - 1, thisIndex, newBoxPos, newBoxSize, limitBegins, leftLimitEnds, (dimension + 1) % 3);
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
		kdTree.objectCount = 0;
		kdTreeNodeHeap.push_back(KDTreeNode());
		kdTreeNodeHeap.push_back(KDTreeNode());
		uint64_t limitBegins[] = { 0, 0, 0 };
		uint64_t limitEnds[] = { entityHeapLength, entityHeapLength, entityHeapLength };

		uint64_t rightLimitBegins[] = { limitBegins[0], limitBegins[1], limitBegins[2] };
		cutAtSplice(kdTree.split, 0, rightLimitBegins, limitEnds);
		uint64_t leftLimitEnds[] = { limitEnds[0], limitEnds[1], limitEnds[2] };
		cutAtSpliceOtherDir(kdTree.split, 0, limitBegins, leftLimitEnds);

		generateKDTreeNode(0, -1, kdTree.position, nmath::Vector3f(kdTree.size.x / 2, kdTree.size.y, kdTree.size.z), limitBegins, leftLimitEnds, 0);
		generateKDTreeNode(1, -1, nmath::Vector3f(kdTree.position.x + kdTree.split, kdTree.position.y, kdTree.position.z), nmath::Vector3f(kdTree.size.x / 2, kdTree.size.y, kdTree.size.z), rightLimitBegins, limitEnds, 0);
	}




	constexpr ~Scene() { delete[] entityHeap; delete[] lightHeap; }
};
