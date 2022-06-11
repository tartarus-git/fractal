#pragma once

#include "Entity.h"

#include "Light.h"

#include "KDTree.h"

#include <new>
#include <cstdint>

#include <vector>

#include "logging/debugOutput.h"

#include <Windows.h>

#include "nmath/vectors/Vector3f.h"

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

		leafObjectHeap = std::move(right.leafObjectHeap);

		return *this;
	}

	uint64_t* xList;
	uint64_t* yList;
	uint64_t* zList;

	uint64_t* list;

	uint64_t* xListR;
	uint64_t* yListR;
	uint64_t* zListR;

	uint64_t* listR;

	void releaseSortedLists() {
		delete[] xList;
		delete[] yList;
		delete[] zList;
		delete[] list;
	}

	void createSortedLists() {
		xList = new (std::nothrow) uint64_t[entityHeapLength];
		yList = new (std::nothrow) uint64_t[entityHeapLength];
		zList = new (std::nothrow) uint64_t[entityHeapLength];

		list = new (std::nothrow) uint64_t[entityHeapLength * 3];

		xListR = new (std::nothrow) uint64_t[entityHeapLength];
		yListR = new (std::nothrow) uint64_t[entityHeapLength];
		zListR = new (std::nothrow) uint64_t[entityHeapLength];

		listR = new (std::nothrow) uint64_t[entityHeapLength * 3];

		if (!(xList && yList && zList && list)) {
			debuglogger::out << "failed to allocate one of the sort lists\n";
			DebugBreak();
		}

		if (!(xListR && yListR && zListR && listR)) {
			debuglogger::out << "failed to allocate one of the sort lists\n";
			DebugBreak();
		}

		for (int i = 0; i < entityHeapLength; i++) {
			xList[i] = i;
			yList[i] = i;
			zList[i] = i;

			xListR[i] = i;
			yListR[i] = i;
			zListR[i] = i;
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

		List = xListR;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.x - entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.x - entityHeap[List[i + 1]].scale.x) {
					uint64_t temp = List[i];
					List[i] = List[i + 1];
					List[i + 1] = temp;
					neededToSort = true;
				}
			}
			if (!neededToSort) { break; }
		}

		List = yListR;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.y - entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.y - entityHeap[List[i + 1]].scale.x) {
					uint64_t temp = List[i];
					List[i] = List[i + 1];
					List[i + 1] = temp;
					neededToSort = true;
				}
			}
			if (!neededToSort) { break; }
		}

		List = zListR;

		while (true) {
			bool neededToSort = false;
			for (int i = 0; i < entityHeapLength - 1; i++) {
				if (entityHeap[List[i]].position.z - entityHeap[List[i]].scale.x > entityHeap[List[i + 1]].position.z - entityHeap[List[i + 1]].scale.x) {
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

			listR[i * 3 + 0] = xListR[i];
			listR[i * 3 + 1] = yListR[i];
			listR[i * 3 + 2] = zListR[i];
		}
	}

	std::vector<uint64_t> leafObjectHeap;

	template <typename Lambda>
	void doThisThingForEveryObjectInRange(Lambda thing, char dimension, uint64_t limitBegins[6], uint64_t limitEnds[6]) {
		uint64_t& yLimit = limitBegins[1];
		uint64_t& xLimit = limitBegins[0];
		uint64_t& zLimit = limitBegins[2];

		uint64_t& yLimitR = limitBegins[1];
		uint64_t& xLimitR = limitBegins[0];
		uint64_t& zLimitR = limitBegins[2];

		uint64_t& yLimitEnd = limitEnds[1];
		uint64_t& xLimitEnd = limitEnds[0];
		uint64_t& zLimitEnd = limitEnds[2];

		uint64_t& yLimitEndR = limitEnds[1];
		uint64_t& xLimitEndR = limitEnds[0];
		uint64_t& zLimitEndR = limitEnds[2];

		if ((yLimit == yLimitEnd && yLimitR == yLimitEndR) || (zLimit == zLimitEnd && zLimitR == zLimitEndR)) { return; }
		// TODO: I think the above is too simple, also we're causing stackoverflow with the current setup. Why?

		for (int i = limitBegins[0]; i < limitEnds[0]; i++) {
			if (entityHeap[xList[i]].position.y + entityHeap[xList[i]].scale.x < entityHeap[yList[yLimit]].position.y + entityHeap[yList[yLimit]].scale.x) { continue; }
			if (entityHeap[xList[i]].position.y - entityHeap[xList[i]].scale.x > entityHeap[yListR[yLimitEndR - 1]].position.y - entityHeap[yListR[yLimitEndR - 1]].scale.x) { continue; }
			if (entityHeap[xList[i]].position.z + entityHeap[xList[i]].scale.x < entityHeap[zList[zLimit]].position.z + entityHeap[zList[zLimit]].scale.x) { continue; }
			if (entityHeap[xList[i]].position.z - entityHeap[xList[i]].scale.x > entityHeap[zListR[zLimitEndR - 1]].position.z - entityHeap[zListR[zLimitEndR - 1]].scale.x) { continue; }

			if (entityHeap[xList[i]].position.x - entityHeap[xList[i]].scale.x == entityHeap[xListR[i]].position.x - entityHeap[xListR[i]].scale.x) { break; }

			thing(xList[i]);
		}
		for (int i = xLimitR; i < xLimitEndR; i++) {
			if (entityHeap[xListR[i]].position.y + entityHeap[xListR[i]].scale.x < entityHeap[yList[yLimit]].position.y + entityHeap[yList[yLimit]].scale.x) { continue; }
			if (entityHeap[xListR[i]].position.y - entityHeap[xListR[i]].scale.x > entityHeap[yListR[yLimitEndR - 1]].position.y - entityHeap[yListR[yLimitEndR - 1]].scale.x) { continue; }
			if (entityHeap[xListR[i]].position.z + entityHeap[xListR[i]].scale.x < entityHeap[zList[zLimit]].position.z + entityHeap[zList[zLimit]].scale.x) { continue; }
			if (entityHeap[xListR[i]].position.z - entityHeap[xListR[i]].scale.x > entityHeap[zListR[zLimitEndR - 1]].position.z - entityHeap[zListR[zLimitEndR - 1]].scale.x) { continue; }

			thing(xList[i]);
		}

		return;

		/*for (int i = limitBegins[0]; i < limitEnds[0]; i++) {
			if (yLimitR == yLimitEndR || entityHeap[xListR[i]].position.y - entityHeap[xList[i]].scale.x < entityHeap[yListR[yLimitR]].position.y - entityHeap[yListR[yLimitR]].scale.x) { continue; }
			if (yLimitEnd == 0 || entityHeap[xList[i]].position.y + entityHeap[xList[i]].scale.x > entityHeap[yList[yLimitEnd - 1]].position.y + entityHeap[yList[yLimitEnd - 1]].scale.x) { continue; }
			if (zLimit == zLimitEnd || entityHeap[xListR[i]].position.z - entityHeap[xList[i]].scale.x < entityHeap[zList[zLimit]].position.z - entityHeap[zList[zLimit]].scale.x) { continue; }
			if (zLimitEnd == 0 || entityHeap[xList[i]].position.z + entityHeap[xList[i]].scale.x > entityHeap[zList[zLimitEnd - 1]].position.z + entityHeap[zList[zLimitEnd - 1]].scale.x) { continue; }
			//if (entityHeap[xList[i]].position.x - entityHeap[xList[i]].scale.x < entityHeap[xList[xLimit]].position.x - entityHeap[xList[xLimit]].scale.x) { break; }
			//if (entityHeap[xList[i]].position.x + entityHeap[xList[i]].scale.x > entityHeap[xList[xLimitEnd]].position.x + entityHeap[xList[xLimitEnd]].scale.x) { break; }
			thing(xList[i]);
		}*/
	}

	// TODO: Use templates to create three different functions that operate on the 3 different dimensions, instead of doing this weird and inefficient indexing stuff.
	void cutAtSplice(float absoluteSplice, char dimension, uint64_t limitBegins[6], uint64_t limitEnds[6]) {
		for (uint64_t i = limitBegins[dimension]; i < limitEnds[dimension]; i++) {
			Entity& entity = entityHeap[list[i * 3 + dimension]];
			if (entity.position[dimension] + entity.scale.x >= absoluteSplice) { limitBegins[dimension] = i; return; }
		}
		limitBegins[dimension] = limitEnds[dimension];

		for (uint64_t i = limitBegins[dimension + 3]; i < limitEnds[dimension + 3]; i++) {
			Entity& entity = entityHeap[listR[i * 3 + dimension]];
			if (entity.position[dimension] - entity.scale.x >= absoluteSplice) { limitBegins[dimension + 3] = i; return; }
		}
		limitBegins[dimension + 3] = limitEnds[dimension + 3];
	}

	void cutAtSpliceOtherDir(float absoluteSplice, char dimension, uint64_t limitBegins[6], uint64_t limitEnds[6]) {
		for (uint64_t i = limitBegins[dimension + 3]; i < limitEnds[dimension + 3]; i++) {
			Entity& entity = entityHeap[listR[i * 3 + dimension]];
			if (entity.position[dimension] - entity.scale.x > absoluteSplice) { limitEnds[dimension + 3] = i; return; }
		}

		for (uint64_t i = limitBegins[dimension]; i < limitEnds[dimension]; i++) {
			Entity& entity = entityHeap[list[i * 3 + dimension]];
			if (entity.position[dimension] + entity.scale.x > absoluteSplice) { limitEnds[dimension] = i; return; }
		}
	}

	void generateKDTreeNode(uint64_t thisIndex, uint64_t parentIndex, nmath::Vector3f boxPos, nmath::Vector3f boxSize, uint64_t limitBegins[6], uint64_t limitEnds[6], char dimension) {


		/*if (entityHeap[xList[limitBegins[0]]].position.x == 0 && entityHeap[xList[limitEnds[0] - 1]].position.x == 0 &&
			entityHeap[yList[limitBegins[1]]].position.y == 40 && entityHeap[yList[limitEnds[1] - 1]].position.y == 40 &&
			entityHeap[zList[limitBegins[2]]].position.z == 0 && entityHeap[zList[limitEnds[2] - 1]].position.z == 40) {
			DebugBreak();
		}*/

		if (boxPos.x == kdTree.position.x && boxSize.x == kdTree.size.x / 2 &&
			boxPos.y == kdTree.position.y && boxSize.y == kdTree.size.y / 2 &&
			boxPos.z == kdTree.position.z + boxSize.z && boxSize.z == kdTree.size.z / 2) {
			DebugBreak();
		}

		int tryCounter = 0;
		labelthing:
			kdTreeNodeHeap[thisIndex].split = 0.5f;				// TODO: Make this be able to be variable in the rest of the code. Just support not cost calculations yet.
			kdTreeNodeHeap[thisIndex].parentIndex = parentIndex;
			kdTreeNodeHeap[thisIndex].objectCount = -1;
			uint64_t leftAmount = 0;
			uint64_t rightLimitBegins[] = { limitBegins[0], limitBegins[1], limitBegins[2], limitBegins[3], limitBegins[4], limitBegins[5] };
			cutAtSplice(kdTreeNodeHeap[thisIndex].split * boxSize[dimension] + boxPos[dimension], dimension, rightLimitBegins, limitEnds);
			doThisThingForEveryObjectInRange([&amount = leftAmount](uint64_t i) { amount++; }, dimension, rightLimitBegins, limitEnds);
			uint64_t rightAmount = 0;
			uint64_t leftLimitEnds[] = { limitEnds[0], limitEnds[1], limitEnds[2], limitEnds[3], limitEnds[4], limitEnds[5] };
			cutAtSpliceOtherDir(kdTreeNodeHeap[thisIndex].split * boxSize[dimension] + boxPos[dimension], dimension, limitBegins, leftLimitEnds);
			doThisThingForEveryObjectInRange([&amount = rightAmount](uint64_t i) { amount++; }, dimension, limitBegins, leftLimitEnds);
			uint64_t totalAmount = 0;
			doThisThingForEveryObjectInRange([&amount = totalAmount](uint64_t i) { amount++; }, dimension, limitBegins, limitEnds);
			if (leftAmount == totalAmount && rightAmount == totalAmount) {				// We're at a leaf.
				if (totalAmount != 0) {
					//DebugBreak();
				}
				if (tryCounter < 2) {
					tryCounter++;
					dimension = (dimension + 1) % 3;
					goto labelthing;
				}
				kdTreeNodeHeap[thisIndex].childrenIndex = leafObjectHeap.size();
				kdTreeNodeHeap[thisIndex].childrenIndex |= (uint64_t)dimension << (sizeof(uint64_t) * 8 - 2);
				kdTreeNodeHeap[thisIndex].objectCount = totalAmount;
				doThisThingForEveryObjectInRange([&](uint64_t i) { leafObjectHeap.push_back(i); }, dimension, limitBegins, limitEnds);
				return;
			}

			// SIDE-NOTE: Write about the weird behaviour of switch cases and that if no case is inside, no statement is executed. See that one construct on wikipedia for inspiration.
			kdTreeNodeHeap[thisIndex].childrenIndex = kdTreeNodeHeap.size();
			kdTreeNodeHeap[thisIndex].childrenIndex |= (uint64_t)dimension << (sizeof(uint64_t) * 8 - 2);

			kdTreeNodeHeap.push_back(KDTreeNode());
			kdTreeNodeHeap.push_back(KDTreeNode());
			uint64_t tempsave = kdTreeNodeHeap.size() - 1;

			nmath::Vector3f newBoxSize = boxSize;
			newBoxSize[dimension] *= kdTreeNodeHeap[thisIndex].split;
			nmath::Vector3f newBoxPos = boxPos;
			generateKDTreeNode(kdTreeNodeHeap.size() - 2, thisIndex, newBoxPos, newBoxSize, limitBegins, leftLimitEnds, (dimension + 1) % 3);
			newBoxPos[dimension] += newBoxSize[dimension];
			generateKDTreeNode(tempsave, thisIndex, newBoxPos, newBoxSize, rightLimitBegins, limitEnds, (dimension + 1) % 3);
	}

	void generateKDTree() {

		kdTree.position = nmath::Vector3f(10000, 1000000, 1000000);
		kdTree.size = nmath::Vector3f(-10000, -1000000, -1000000);

		for (uint64_t i = 0; i < entityHeapLength; i++) {
			float entityLowestValue = entityHeap[i].position.x - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.x) { kdTree.position.x = entityLowestValue; }
			entityLowestValue = entityHeap[i].position.y - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.y) { kdTree.position.y = entityLowestValue; }
			entityLowestValue = entityHeap[i].position.z - entityHeap[i].scale.x;
			if (entityLowestValue < kdTree.position.z) { kdTree.position.z = entityLowestValue; }
		}
		for (uint64_t i = 0; i < entityHeapLength; i++) {
			float entityLowestValue = entityHeap[i].position.x + entityHeap[i].scale.x;
			if (entityLowestValue > kdTree.size.x + kdTree.position.x) { kdTree.size.x = entityLowestValue - kdTree.position.x; }
			entityLowestValue = entityHeap[i].position.y + entityHeap[i].scale.x;
			if (entityLowestValue > kdTree.size.y + kdTree.position.y) { kdTree.size.y = entityLowestValue - kdTree.position.y; }
			entityLowestValue = entityHeap[i].position.z + entityHeap[i].scale.x;
			if (entityLowestValue > kdTree.size.z + kdTree.position.z) { kdTree.size.z = entityLowestValue - kdTree.position.z; }
		}

		createSortedLists();

		kdTreeNodeHeap.push_back(KDTreeNode());
		uint64_t limitBegins[] = { 0, 0, 0, 0, 0, 0 };
		uint64_t limitEnds[] = { entityHeapLength, entityHeapLength, entityHeapLength, entityHeapLength, entityHeapLength, entityHeapLength };
		generateKDTreeNode(0, -1, kdTree.position, kdTree.size, limitBegins, limitEnds, 0);

		releaseSortedLists();
	}




	constexpr ~Scene() { delete[] entityHeap; delete[] lightHeap; /* TODO: Delete the rest of the stuff too. */ }
};
