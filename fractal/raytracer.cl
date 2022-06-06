typedef struct Material {
	float3 color;
} Material;

typedef struct Entity {
	float3 position;
	float3 rotation;
	float3 scale;
	uint type;
	uint material;
} Entity;

inline float intersectLineSphere(float3 origin, float3 ray, float3 spherePos, float sphereRadius) {

	float otherTerm = -(dot((origin - spherePos), ray)) * 2;
	float determinant = dot(dot((origin - spherePos), ray) * 2, dot((origin - spherePos), ray) * 2) - 4 * dot(ray, ray) * (dot(origin - spherePos, origin - spherePos) - sphereRadius * sphereRadius);

	if (determinant > 0) {
		determinant = sqrt(determinant);
		float d0 = (otherTerm + determinant) / dot(ray, ray) / 2;
		float d1 = (otherTerm - determinant) / dot(ray, ray) / 2;
		if (d0 <= d1) {
			if (d0 < 0) {
				if (d1 < 0) {
					return -1;
				}
				return d1;
			}
			return d0;
		} else {
			if (d1 < 0) {
				if (d0 < 0) {
					return -1;
				}
				return d0;
			}
			return d1;
		}
	} else if (determinant == 0) {
		float distance = otherTerm / dot(ray, ray) / 2;
		if (distance < 0) { return -1; }
		return distance;
	} else {
		return -1;
	}

}

float3 intersectWithSphere(float3 rayOrigin, float3 ray, float3 spherePosition, float sphereRadius, bool* didItHit, float* tthing) {
	float t = intersectLineSphere(rayOrigin, ray, spherePosition, sphereRadius);
	if (t < 0) { *didItHit = false; return (float3)(0, 0, 0); }
	*didItHit = true;
	*tthing = t;
	return (float3)(rayOrigin + ray * t);
}

typedef struct Light {
	float3 position;
	float3 color;
} Light;

typedef struct Matrix4f { float data[16]; } Matrix4f;

float3 multiplyMatWithFloat3(Matrix4f mat, float3 vec) {
	float3 result;
	result.x = mat.data[0] * vec.x + mat.data[1] * vec.y + mat.data[2] * vec.z;
	result.y = mat.data[4] * vec.x + mat.data[5] * vec.y + mat.data[6] * vec.z;
	result.z = mat.data[8] * vec.x + mat.data[9] * vec.y + mat.data[10] * vec.z;
	return result;
}

typedef struct KDTreeNode {
	ulong childrenIndex;                // dimension encoded in here
	ulong parentIndex;
	uint objectCount;
	float split;
} KDTreeNode;

inline float rayIntersectAABB(float3 rayOrigin, float3 ray, float3 startPosition, float3 stopPosition) {

	/*

	How does this algorithm work?
		1. Checks each dimension of rayOrigin against the AABB and sees which side the rayOrigin is on or if it's inside the AABB.
			It also calculates the "time" it would take to collide with the closest side of the AABB in that dimension.
			If at any point one of the dimensions of rayOrigin is outside of the AABB and the ray component in that dimension is also zero, returns -1 since ray will never hit AABB.
		(The reason step 1 has so much code is because of efficiency. I don't see a way around the code bloat without sacrificing efficiency.)

		2. Finds the biggest recorded time and uses that time and it's corresponding dimension to calculate where in space the hit point is.
			NOTE: We don't do the following calculations for every hit point as soon as we get it because of average efficiency for this and pretty much every other use-case.
			NOTE: Instead we find the biggest value and only do it for that value. Obviously finding that value takes a couple instructions, but it's well worth it.
			If the hit point is inside the bounds of the other two dimensions, that means that the hit point actually hit a face of the AABB, so we return the "time".
			If the hit point is outside the bounds of the other two dimensions, that means the hit point hit the wall plane, but hit it in free space and didn't hit the AABB. Return -1.

		NOTE: Why are we looking for the biggest recorded time? Shouldn't it be the smallest recorded time?
			- If we looked for the smallest recorded time, there would be a chance that it wouldn't actually hit the AABB, and we would then have to go to the second smallest
				recorded time and try that, and so on for the third.
			- That doesn't happen for the biggest recorded time because of the geometry of an AABB and the fact that we automatically cull the back-faces in the intersection
				part of the algorithm.
				- The front faces of a rectangle never overlap each other when they are projected down to a plane.
				- If you consider this and consider the geometry a little bit, it's easy to see there are only two possibilities for hitting an AABB.
					- Either the biggest time value is the hit point, or there is no hit point at all.
					---> So by using the biggest time value, we avoid having to create a sorted list and do multiple checks, but we still get the closest hit point (cuz back-face culling).
		
		NOTE: I've optimized the algorithm for rayOrigin's that aren't already inside the AABB, which is perfect for this program. It's good all round though, the trade-offs for
			the optimization are very small.
	
	*/

	float3 intersectionTimes;
	
	if (rayOrigin.x <= startPosition.x) {
		if (ray.x != 0) {
			intersectionTimes.x = (startPosition.x - rayOrigin.x) / ray.x;
		} else {
			return -1;
		}
	} 
	else if (rayOrigin.x >= stopPosition.x) {
		if (ray.x != 0) {
			intersectionTimes.x = (stopPosition.x - rayOrigin.x) / ray.x;
		} else {
			return -1;
		}
	}
	else {
		intersectionTimes.x = 0;

		if (rayOrigin.y <= startPosition.y) {
			if (ray.y != 0) {
				intersectionTimes.y = (startPosition.y - rayOrigin.y) / ray.y;
				goto finishZ;
			} else {
				return -1;
			}
		}
		else if (rayOrigin.y >= stopPosition.y) {
			if (ray.y != 0) {
				intersectionTimes.y = (stopPosition.y - rayOrigin.y) / ray.y;
				goto finishZ;
			} else {
				return -1;
			}
		}
		else {
			intersectionTimes.y = 0;

			if (rayOrigin.z <= startPosition.z) {
				if (ray.z != 0) {
					intersectionTimes.z = (startPosition.z - rayOrigin.z) / ray.z;
					goto nothingToFinish;
				} else {
					return -1;
				}
			}
			else if (rayOrigin.z >= stopPosition.z) {
				if (ray.z != 0) {
					intersectionTimes.z = (stopPosition.z - rayOrigin.z) / ray.z;
					goto nothingToFinish;
				} else {
					return -1;
				}
			} else {
				return 0;
			}
		}
	}

		if (rayOrigin.y <= startPosition.y) {
			if (ray.y != 0) {
				intersectionTimes.y = (startPosition.y - rayOrigin.y) / ray.y;
			} else {
				return -1;
			}
		}
		else if (rayOrigin.y >= stopPosition.y) {
			if (ray.y != 0) {
				intersectionTimes.y = (stopPosition.y - rayOrigin.y) / ray.y;
			} else {
				return -1;
			}
		} else {
			intersectionTimes.y = 0;
		}

finishZ:
			if (rayOrigin.z <= startPosition.z) {
				if (ray.z != 0) {
					intersectionTimes.z = (startPosition.z - rayOrigin.z) / ray.z;
				} else {
					return -1;
				}
			}
			else if (rayOrigin.z >= stopPosition.z) {
				if (ray.z != 0) {
					intersectionTimes.z = (stopPosition.z - rayOrigin.z) / ray.z;
				} else {
					return -1;
				}
			} else {
				intersectionTimes.z = 0;
			}

nothingToFinish:

	if (intersectionTimes.x > intersectionTimes.y) {
		if (intersectionTimes.x > intersectionTimes.z) {			// TODO: Why doesn't this algorithm produce mirror images?
			rayOrigin += ray * intersectionTimes.x;
			if (rayOrigin.y > startPosition.y && rayOrigin.y < stopPosition.y && rayOrigin.z > startPosition.z && rayOrigin.z < stopPosition.z) { return intersectionTimes.x; }
		} else {
			rayOrigin += ray * intersectionTimes.z;
			if (rayOrigin.y > startPosition.y && rayOrigin.y < stopPosition.y && rayOrigin.x > startPosition.x && rayOrigin.x < stopPosition.x) { return intersectionTimes.z; }
		}
	} else {
		if (intersectionTimes.y > intersectionTimes.z) {
			rayOrigin += ray * intersectionTimes.y;
			if (rayOrigin.x > startPosition.x && rayOrigin.x < stopPosition.x && rayOrigin.z > startPosition.z && rayOrigin.z < stopPosition.z) { return intersectionTimes.y; }
		} else {
			rayOrigin += ray * intersectionTimes.z;
			if (rayOrigin.y > startPosition.y && rayOrigin.y < stopPosition.y && rayOrigin.x > startPosition.x && rayOrigin.x < stopPosition.x) { return intersectionTimes.z; }
		}
	}

	return -1;
}

inline uint4 sampleSkybox(float3 ray) {
	return (uint4)(0, 255, 0, 255);
}

__kernel void traceRays(__write_only image2d_t frame, uint frameWidth, uint frameHeight, float3 cameraPos, Matrix4f cameraRotationMat, float rayOriginZ, 
						__global Entity* entityHeap, ulong entityHeapLength, float3 kdTreeStartPosition, float3 kdTreeStopPosition, __global KDTreeNode* kdTreeNodeHeap, ulong kdTreeNodeHeapLength, 
						__global ulong* leafObjectHeap, ulong leafObjectHeapLength, 
						__global Light* lightHeap, ulong lightHeapLength, __global Material* materialHeap, ulong materialHeapLength, 
						ulong materialHeapOffset) {
	int x = get_global_id(0);
	if (x >= frameWidth) { return; }
	int2 coords = (int2)(x, get_global_id(1));

	float3 rayOrigin = (float3)(0, 0, rayOriginZ);

	float3 ray = (float3)(coords.x - (int)frameWidth / 2, -coords.y + (int)frameHeight / 2, -rayOriginZ);

	ray = multiplyMatWithFloat3(cameraRotationMat, ray);
	ray = normalize(ray);

	// TODO: Think about what checks you should do and what errors you should just let crash the system. Speed is key here.
	if (entityHeapLength == 0 || kdTreeNodeHeapLength == 0) { write_imageui(frame, coords, sampleSkybox(ray)); return; }

	float3 colorCollector = (float3)(0, 0, 0);

	if (rayIntersectAABB(cameraPos, ray, kdTreeStartPosition, kdTreeStopPosition) == -1) { write_imageui(frame, coords, sampleSkybox(ray)); return; }
	//if (rayIntersectAABB(cameraPos, ray, (float3)(0, 0, 0), (float3)(10, 10, 10)) == -1) { write_imageui(frame, coords, sampleSkybox(ray)); return; }
	//write_imageui(frame, coords, (uint4)(0, 0, 0, 255));
	//return;

	ulong previousKDTreeNodeIndex = 0;
	ulong currentKDTreeNodeIndex = 0;
	float tempdist;
	while (true) {
		if (kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount == -1) {
			float3 previousKDTreeStopPosition = kdTreeStopPosition;
			// NOTE: If no case is hit, program keeps executing, defined so that no weird behaviour happens. Basically as if default: break; were there.
				switch (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex >> (sizeof(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex) - 2)) {
				case 0: kdTreeStopPosition.x = kdTreeNodeHeap[currentKDTreeNodeIndex].split; break;			// TODO: Consider optimizing this by array accessing the vector and thereby not needing the switch case. Is that more efficient? No pitfalls?
				case 1: kdTreeStopPosition.y = kdTreeNodeHeap[currentKDTreeNodeIndex].split; break;
				case 2: kdTreeStopPosition.z = kdTreeNodeHeap[currentKDTreeNodeIndex].split;
				}
				float leftDist = rayIntersectAABB(rayOrigin, ray, kdTreeStartPosition, kdTreeStopPosition);				// TODO: Something's wrong with this algorithm I think, somehow the half-half thing I've got going doesn't show in this system.
				float rightDist = rayIntersectAABB(rayOrigin, ray, kdTreeStopPosition, previousKDTreeStopPosition);
				if (leftDist < rightDist || rightDist == -1) {			// TODO: Add -1 checks.
					if (previousKDTreeNodeIndex == kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex) {
						kdTreeStartPosition = kdTreeStopPosition;
						kdTreeStopPosition = previousKDTreeStopPosition;
						currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + 1;
						continue;
					}            // First parentIndex needs to be something other than the two child indices.
					if (previousKDTreeNodeIndex == kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + 1) {
						if (currentKDTreeNodeIndex == 0) { write_imageui(frame, coords, sampleSkybox(ray)); return; }
						previousKDTreeNodeIndex = currentKDTreeNodeIndex;
						currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
						continue;
					}
					currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex;
					tempdist = leftDist;
					continue;
				}
					if (previousKDTreeNodeIndex == kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + 1) {
						currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex;
						continue;
					}
					if (previousKDTreeNodeIndex == kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex) {
						templabelthing:
						if (currentKDTreeNodeIndex == 0) { write_imageui(frame, coords, sampleSkybox(ray)); return; }
						previousKDTreeNodeIndex = currentKDTreeNodeIndex;
						currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
						continue;
					}
					kdTreeStartPosition = kdTreeStopPosition;
					kdTreeStopPosition = previousKDTreeStopPosition;
					currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + 1;
					tempdist = rightDist;
					continue;
		}
		if (kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount == 0) {
			write_imageui(frame, coords, (uint4)(255, 0, 0, 255));
			return;
		}
		for (ulong i = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex; i < kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount; i++) {
			write_imageui(frame, coords, (uint4)(0, 0, fmin(tempdist, 1) * 255, 255));
			return;
			//bool good = getRayColor(entityHeap[leafObjectHeap[i]]);
			//if (good) { goto escapeWhileLoop; }
			// TODO: Make distance aware this part.
		}

		previousKDTreeNodeIndex = currentKDTreeNodeIndex;
		currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
		if (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex == previousKDTreeNodeIndex) {
			switch (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex >> (sizeof(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex) - 2)) {
			case 0: kdTreeStopPosition.x += kdTreeNodeHeap[currentKDTreeNodeIndex].split - kdTreeStartPosition.x; continue;			// TODO: Make this compatible with non-half lengths.
			case 1: kdTreeStopPosition.y += kdTreeNodeHeap[currentKDTreeNodeIndex].split - kdTreeStartPosition.y; continue;			// TODO: Use percentage as split to make that happen.
			case 2: kdTreeStopPosition.z += kdTreeNodeHeap[currentKDTreeNodeIndex].split - kdTreeStartPosition.z; continue;
			}
		}
		switch (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex >> (sizeof(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex) - 2)) {
		case 0: kdTreeStartPosition.x += kdTreeStopPosition.x - kdTreeNodeHeap[currentKDTreeNodeIndex].split; continue;
		case 1: kdTreeStartPosition.y += kdTreeStopPosition.y - kdTreeNodeHeap[currentKDTreeNodeIndex].split; continue;
		case 2: kdTreeStartPosition.z += kdTreeStopPosition.z - kdTreeNodeHeap[currentKDTreeNodeIndex].split;
		}

		/*while (true) {
			if (currentKDTreeNodeIndex == 0) { write_imageui(frame, coords, sampleSkybox(ray)); return; }
			if (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex == previousKDTreeNodeIndex) {
				switch (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex & 0b11) {
				case 0: kdTreePosition.x += kdTreeSize.x; break;
				case 1: kdTreePosition.y += kdTreeSize.y; break;
				case 2: kdTreePosition.z += kdTreeSize.z; break;
				}
				if (!rayIntersectAABB(kdTreePosition, kdTreeSize)) { goto templabel; }
				currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + 1;
				break;          // TODO: Check about cache stuff, cuz technically you could just increment the child counter instead of setting it around, maybe more cache coherent.
			}
			templabel:
			previousKDTreeNodeIndex = currentKDTreeNodexIndex;
			currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
			switch (kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex & 0b11) {
			case 0: kdTreePosition.x -= kdTreeSize.x; kdTreeSize.x *= 2; break;
			case 1: kdTreePosition.y -= kdTreeSize.y; kdTreeSize.y *= 2; break;
			case 2: kdTreePosition.z -= kdTreeSize.z; kdTreeSize.z *= 2; break;
			}*/
		}
	}
/*













int lastCollidedWithObject = 100;
for (int j = 0; j < 10; j++) {
	float3 closestPoint;
	float closestLength = 10000000;
	int closestIndex = 0;
	bool noHits = true;

	for (int i = 0; i < entityHeapLength; i++) {
		if (i == lastCollidedWithObject) { continue; }
		bool didItHit = false;
		float tthing;
		float3 hitPoint = intersectWithSphere(cameraPos, ray, entityHeap[i].position, entityHeap[i].scale.x, &didItHit, &tthing);
		if (didItHit) {
			//write_imageui(frame, coords, (uint4)(255, 255, 0, 255)); return;
			noHits = false;
			if (tthing < closestLength) { closestPoint = hitPoint; closestIndex = i; closestLength = tthing;
			}
			//write_imageui(frame, coords, (uint4)(fabs(normal.x) * 255, fabs(normal.y) * 255, fabs(normal.z) * multiplier * 255, 1)); return;
		}
	}
	if (!noHits) {

			lastCollidedWithObject = closestIndex;
			if (closestIndex & 1) { 
				
			float3 normal = normalize(closestPoint - entityHeap[closestIndex].position);
			bool free = true;
			for (int k = 0; k < entityHeapLength; k++) {
				if (k == closestIndex) { continue; }
				bool thing = false;
				float tthing;
				intersectWithSphere(closestPoint, (float3)(0, 1, 0), entityHeap[k].position, entityHeap[k].scale.x, &thing, &tthing);
				if (thing) { free = false; break; }
			}
			if (free) {
			float multiplier = fmax(dot(normal, (float3)(0, 1, 0)), 0);
			//float multiplier = 1;
			float3 color = (float3)(multiplier, multiplier, multiplier);
			colorCollector += color;
			} else {
				// nothing.
			}
			break;
			 }

			float3 normal = normalize(closestPoint - entityHeap[closestIndex].position);
			//float multiplier = fmax(dot(normal, (float3)(0, 1, 0)), 0);
			//float multiplier = 1;
			//float3 color = (float3)(multiplier, multiplier, multiplier);
			//colorCollector += color;

			cameraPos = closestPoint;
			float dotval = dot(ray, normal);
			ray -= 2 * dotval * normal;
			//colorCollector.z += 0.5f;

		continue; }
		else {
			colorCollector.y += 1;
			break;
		}
}
	//write_imageui(frame, coords, (uint4)(fabs(ray.x) * 255, fabs(ray.y) * 255, fabs(ray.z) * 255, 1));
	write_imageui(frame, coords, (uint4)(255 * fmin(colorCollector.x, 1), 255 * fmin(colorCollector.y, 1), 255 * fmin(colorCollector.z, 1), 1));
	*/
//}