typedef struct Material {
	float3 color;
	float reflectivity;
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
			If at any point one of the dimensions of rayOrigin is outside of the AABB and the ray component in that dimension is going away from it's corresponding wall, returns -1 since ray will never hit AABB.
			PROOF for the above statement: The only way to move away from a wall in a single dimension of the ray is to be outside of the AABB in that dimension. Since you are moving away from the plane of the wall, and since
			every other wall requires the ray to cross the plane from which you are currently moving yourself away from (because you are currently outside of the AABB), it follows that you can never hit any valid wall like this.
			NOTE: We used to just do ray.dimension != 0 for the check, but replacing != with >/< is good for the above reason and shouldn't hurt our performance.
			NOTE: Even with != 0 though, you still wouldn't see any mirror images of the AABB's in the sky, like you might expect. This is because of the filtering code at the end of the function. It looks for the biggest distance. This distance will be closest to zero if the distances
			are negative (like they would be when you're looking into the opposite direction of the AABB (into the sky)). The distances that are closest to zero are either negative distances to parts of the dimension planes that aren't part of the AABB (so those will get filtered out) or
			0 if you are inside the box in at least one dimension (if this is the case and the rest are negative, 0 will be selected and multiplied with ray before being added to rayOrigin. Since that point will always be outside of the AABB in the other two dimensions, it gets filtered out).
			It's kind of complicated, but basically everything works out even if we only use !=. Using >/< is just way better for efficiency in the cases where the player isn't looking at an AABB, then we can save a bunch of processing at no extra cost in other situations. VERY GOOD DEAL!
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
		if (ray.x > 0) {
			intersectionTimes.x = (startPosition.x - rayOrigin.x) / ray.x;
		} else {
			return -1;
		}
	} 
	else if (rayOrigin.x >= stopPosition.x) {
		if (ray.x < 0) {
			intersectionTimes.x = (stopPosition.x - rayOrigin.x) / ray.x;
		} else {
			return -1;
		}
	}
	else {
		intersectionTimes.x = 0;

		if (rayOrigin.y <= startPosition.y) {
			if (ray.y > 0) {
				intersectionTimes.y = (startPosition.y - rayOrigin.y) / ray.y;
				goto finishZ;
			} else {
				return -1;
			}
		}
		else if (rayOrigin.y >= stopPosition.y) {
			if (ray.y < 0) {
				intersectionTimes.y = (stopPosition.y - rayOrigin.y) / ray.y;
				goto finishZ;
			} else {
				return -1;
			}
		}
		else {
			intersectionTimes.y = 0;

			if (rayOrigin.z <= startPosition.z) {
				if (ray.z > 0) {
					intersectionTimes.z = (startPosition.z - rayOrigin.z) / ray.z;
					goto nothingToFinish;
				} else {
					return -1;
				}
			}
			else if (rayOrigin.z >= stopPosition.z) {
				if (ray.z < 0) {
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
			if (ray.y > 0) {
				intersectionTimes.y = (startPosition.y - rayOrigin.y) / ray.y;
			} else {
				return -1;
			}
		}
		else if (rayOrigin.y >= stopPosition.y) {
			if (ray.y < 0) {
				intersectionTimes.y = (stopPosition.y - rayOrigin.y) / ray.y;
			} else {
				return -1;
			}
		} else {
			intersectionTimes.y = 0;
		}

finishZ:
			if (rayOrigin.z <= startPosition.z) {
				if (ray.z > 0) {
					intersectionTimes.z = (startPosition.z - rayOrigin.z) / ray.z;
				} else {
					return -1;
				}
			}
			else if (rayOrigin.z >= stopPosition.z) {
				if (ray.z < 0) {
					intersectionTimes.z = (stopPosition.z - rayOrigin.z) / ray.z;
				} else {
					return -1;
				}
			} else {
				intersectionTimes.z = 0;
			}

nothingToFinish:

	if (intersectionTimes.x > intersectionTimes.y) {
		if (intersectionTimes.x > intersectionTimes.z) {
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

#define DEBUG_RETURN write_imageui(frame, coords, (uint4)(100, 100, 0, 255)); return;
#define SECOND_DEBUG_RETURN write_imageui(frame, coords, (uint4)(0, 100, 100, 255)); return;

inline uint4 sampleSkybox(float3 ray) {
	//return (uint4)(0, 255, 0, 255);
	ray = normalize(ray);
	return (uint4)(fabs(ray.x * 255), fabs(ray.y * 255), fabs(ray.z * 255), 255);
}

inline ulong initRandSeed(uint frameWidth) {
	return ((ulong)get_global_id(0) * (ulong)frameWidth + (ulong)get_global_id(1)) << 32;
}

inline uint randInt(ulong* seed) {
	*seed *= 1345678;
	return *seed >> 32;
}

inline float randFloat(ulong* seed) {
	uint randomInteger = randInt(seed);
	return randomInteger * (1 / (float)((uint)-1));
}

#define randInt() randInt(&randSeed)
#define randFloat() randFloat(&randSeed)

inline char extractDimensionValue(ulong input) { return input >> (sizeof(input) * 8 - 2); }
inline ulong removeDimensionValue(ulong input) { return input & ((ulong)-1 >> 2); }

#define RECONSTRUCT_PARENT 	if (noDimChildrenIndex == previousKDTreeNodeIndex) { \
								switch (splitDimension) { \
								case 0: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreeSize.x = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									DEBUG_RETURN; kdTreeSize.x /= kdTreeNodeHeap[currentKDTreeNodeIndex].split; \
									break; \
								case 1: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreeSize.y = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									kdTreeSize.y /= kdTreeNodeHeap[currentKDTreeNodeIndex].split; \
									break; \
								case 2: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreeSize.z = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									kdTreeSize.z /= kdTreeNodeHeap[currentKDTreeNodeIndex].split; \
								} \
							} else { \
								switch (splitDimension) { \
								case 0: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreePosition.x -= upwardsTraversalCache[upwardsTraversalCacheSize] - kdTreeSize.x; \
										kdTreeSize.x = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									DEBUG_RETURN; kdTreePosition.x += kdTreeSize.x; kdTreeSize.x /= 1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split; kdTreePosition.x -= kdTreeSize.x; \
									break; \
								case 1: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreePosition.y -= upwardsTraversalCache[upwardsTraversalCacheSize] - kdTreeSize.y; \
										kdTreeSize.y = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									kdTreePosition.y += kdTreeSize.y; kdTreeSize.y /= 1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split; kdTreePosition.y -= kdTreeSize.y; \
									break; \
								case 2: \
									if (upwardsTraversalCacheSize != 0) { \
										upwardsTraversalCacheSize--; \
										kdTreePosition.z -= upwardsTraversalCache[upwardsTraversalCacheSize] - kdTreeSize.z; \
										kdTreeSize.z = upwardsTraversalCache[upwardsTraversalCacheSize]; \
										break; \
									} \
									kdTreePosition.z += kdTreeSize.z; kdTreeSize.z /= 1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split; kdTreePosition.z -= kdTreeSize.z; \
								} \
							}

//#define RENDER colorSum += (float3)(1, 1, 1) * colorProduct; write_imageui(frame, coords, (uint4)(fmin(colorSum.x, 1) * 255, fmin(colorSum.y, 1) * 255, fmin(colorSum.z, 1) * 255, 255))
#define RENDER colorSum += (float3)(1, 1, 1) * colorProduct; renderColorSum += colorSum;

#define FREE_STACK_SPACE_IN_UNITS_OF_4 100

__kernel void traceRays(__write_only image2d_t frame, uint frameWidth, uint frameHeight, 
						float3 cameraPos, Matrix4f cameraRotationMat, float rayOriginZ, 
						__global Entity* entityHeap, ulong entityHeapLength, 
						float3 kdTreePosition, float3 kdTreeSize, __global KDTreeNode* kdTreeNodeHeap, ulong kdTreeNodeHeapLength, __global ulong* leafObjectHeap, ulong leafObjectHeapLength, 
						__global Light* lightHeap, ulong lightHeapLength, 
						__global Material* materialHeap, ulong materialHeapLength, ulong materialHeapOffset) {

	ulong randSeed = initRandSeed(frameWidth);

	int x = get_global_id(0);
	if (x >= frameWidth) { return; }
	int2 coords = (int2)(x, get_global_id(1));

	float3 renderColorSum = (float3)(0, 0, 0);

	float3 cameraBackup = cameraPos;

	float upwardsTraversalCache[FREE_STACK_SPACE_IN_UNITS_OF_4];
	ulong upwardsTraversalCacheSize = 0;

// TODO: Figure out a way to measure variance between the samples and a way to conditionally add more samples to the mix.
for (int indexthing = 0; indexthing < 1; indexthing++) {
	cameraPos = cameraBackup;

	float3 ray = (float3)(coords.x - (int)frameWidth / 2 + randFloat(), -coords.y + (int)frameHeight / 2 - randFloat(), -rayOriginZ);
	ray = normalize(ray);
	ray = multiplyMatWithFloat3(cameraRotationMat, ray);
/*
float3 colorCollector;


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
			if (true) { 
				
			float3 normal = normalize(closestPoint - entityHeap[closestIndex].position);
			float multiplier = fmax(dot(normal, (float3)(0, 1, 0)), 0);
			//float multiplier = 1;
			float3 color = (float3)(multiplier, multiplier, multiplier);
			 colorCollector = color;
	write_imageui(frame, coords, (uint4)(255 * fmin(colorCollector.x, 1), 255 * fmin(colorCollector.y, 1), 255 * fmin(colorCollector.z, 1), 255));
	return;
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
			break;
		}
}
	//write_imageui(frame, coords, (uint4)(fabs(ray.x) * 255, fabs(ray.y) * 255, fabs(ray.z) * 255, 1));


*/

	if (kdTreeNodeHeapLength == 0) { write_imageui(frame, coords, sampleSkybox(ray)); return; }

	if (rayIntersectAABB(cameraPos, ray, kdTreePosition, kdTreePosition + kdTreeSize) == -1) { write_imageui(frame, coords, sampleSkybox(ray)); return; }

	ulong previousKDTreeNodeIndex = 0;
	ulong currentKDTreeNodeIndex = 0;
	float3 leftKDTreeNodeSize;
	float3 rightKDTreeNodePosition;
	float3 rightKDTreeNodeSize;

	char splitDimension = extractDimensionValue(kdTreeNodeHeap[0].childrenIndex);
	ulong noDimChildrenIndex = 1;					// NOTE: It follows from this, that the first child of the first node MUST BE LOCATED AT INDEX 1.

	float3 colorSum = (float3)(0, 0, 0);
	float3 colorProduct = (float3)(1, 1, 1);
	char maxBounces = 10;			// TODO: Handle this through parameter.

	while (true) {

		/*

		NOTE: Why do we use position + size for kd-tree bounds and why do we use percentages for split positions?
			- I thought through it and came to the conclusion that position + end position would be more efficient computationally,
				and that we should use absolute values for the split locations.
			- In theory, this should work nicely, except for the fact that you can't traverse the tree upwards like that, at least not without
				a cache. If you're split is absolute, you can't undo it to get back up the tree, whereas you can when your split is
				percentage-wise. Everything would work if we just used a cache, but that would mean that the depth of our tree is limited by
				the cache, which is in private memory and can't grow that large.
			- Solution is to use percentages for the splits. We can still have a cache to make things a lot faster, but we don't need it
				and the tree can grow as large as it wants.
			- If we're using percentages though, it's more efficient to express the bounds as position + size instead of position + end position,
				so that's why we use the types that we use.

		*/

		if (kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount == -1) {
upwardsTraversalLoop:
			/*

			NOTE: Something I didn't know about switch cases in C/C++ is that the standard defines that if no case is hit, none
			of the statements in the switch case will be executed. That means execution will just skip to after the switch case and start
			executing there. It's equivalent to having an implicit "default: break;" in every switch block.
			This kind of sucks since because of it, every switch case theoretically has to have a branch that checks if the input value
			is outside of the bounds of the cases. If it is, it'll just skip past the switch as I said.
			It is technically inefficient, but there are reasons for it. It protects you from shooting yourself in the foot when writing
			switches. In practice, the compiler can sometimes look at your program and guarantee that a specific variable will never be anything
			other than one of three values for example. In those cases, I assume the branch gets optimized out, but this static analysis has
			it's limits of course.
			I always thought that if you didn't hit any cases, it would just be undefined behaviour, it could do anything, maybe
			start from the top and execute everything in the switch or whatever else is most practical for the compiler at that moment.
			I think it's kind of a shame that it doesn't do this, since it's very easy to still be safe with switch cases. Just write
			"default: break;" at the bottom of every switch you write and you would still be fine.
			Instead I have to deal with an unremovable overhead, just because people can't be bothered to write "default: break;".
			Doesn't this go against C++'s zero overhead principle or something?

			*/

			switch (splitDimension) {
			case 0:			// TODO: See if this could be better/moved somewhere else or something.
				leftKDTreeNodeSize = (float3)(kdTreeSize.x * kdTreeNodeHeap[currentKDTreeNodeIndex].split, kdTreeSize.y, kdTreeSize.z);
			 	rightKDTreeNodeSize = (float3)(kdTreeSize.x * (1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split), kdTreeSize.y, kdTreeSize.z);
				rightKDTreeNodePosition = (float3)(kdTreePosition.x + leftKDTreeNodeSize.x, kdTreePosition.y, kdTreePosition.z);
				break;
			case 1:
				leftKDTreeNodeSize = (float3)(kdTreeSize.x, kdTreeSize.y * kdTreeNodeHeap[currentKDTreeNodeIndex].split, kdTreeSize.z);
			 	rightKDTreeNodeSize = (float3)(kdTreeSize.x, kdTreeSize.y * (1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split), kdTreeSize.z);
				rightKDTreeNodePosition = (float3)(kdTreePosition.x, kdTreePosition.y + leftKDTreeNodeSize.y, kdTreePosition.z);
				break;
			case 2:
				leftKDTreeNodeSize = (float3)(kdTreeSize.x, kdTreeSize.y, kdTreeSize.z * kdTreeNodeHeap[currentKDTreeNodeIndex].split);
			 	rightKDTreeNodeSize = (float3)(kdTreeSize.x, kdTreeSize.y, kdTreeSize.z * (1 - kdTreeNodeHeap[currentKDTreeNodeIndex].split));
				rightKDTreeNodePosition = (float3)(kdTreePosition.x, kdTreePosition.y, kdTreePosition.z + leftKDTreeNodeSize.z);
				break;
			}

			float rightDist = rayIntersectAABB(cameraPos, ray, rightKDTreeNodePosition, rightKDTreeNodePosition + rightKDTreeNodeSize);

			if (rightDist == -1) {
				if (previousKDTreeNodeIndex == noDimChildrenIndex) {
					if (currentKDTreeNodeIndex == 0) { RENDER; break; }

					previousKDTreeNodeIndex = currentKDTreeNodeIndex;
					currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
					splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					RECONSTRUCT_PARENT;
					goto upwardsTraversalLoop;
				}

				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

				kdTreeSize = leftKDTreeNodeSize;
				currentKDTreeNodeIndex = noDimChildrenIndex;
				/*
				NOTE: We set the splitDimension and noDimChildrenIndex variables here instead of setting them at the upwardsTraversalLoop label
				because of a trade-off consideration. Setting them up there would mean setting them to something else when we do upwards
				traversal like we do above. That would be inefficient. Doing it like we do here means that upwards traversal from leaf nodes
				(the code for that is a ways under us) means setting them to something else. So we have to choose which process we want to
				be make inefficient. I choose this way, because the path that is now made efficient is taken more often AFAIK,
				making the net efficiency gain better this way.
				*/
				splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				continue;
			}

			float leftDist = rayIntersectAABB(cameraPos, ray, kdTreePosition, kdTreePosition + leftKDTreeNodeSize);

			if (leftDist == -1) {
				if (previousKDTreeNodeIndex == noDimChildrenIndex + 1) {			// TODO: Store in variable?
					if (currentKDTreeNodeIndex == 0) { RENDER; break; }

					previousKDTreeNodeIndex = currentKDTreeNodeIndex;
					currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
					splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					RECONSTRUCT_PARENT;
					goto upwardsTraversalLoop;
				}

				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

				kdTreePosition = rightKDTreeNodePosition;
				kdTreeSize = rightKDTreeNodeSize;
				currentKDTreeNodeIndex = noDimChildrenIndex + 1;
				splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				continue;
			}

			/*
			NOTE: I didn't find anything useful on the internet about this, so I'm just going to use common sense.
			It would make a tremendous amount of sense that using boolean logic to combine conditions before doing one actual test with a
			potential branch is better than having nested tests on all the conditions. The first situation is way way way better for the branch
			predictor AFAIK. Now obviously, the compiler can almost definitely optimize this, so it's completely fine to do whatever is most
			readable for the specific situation in most cases.
			And obviously, if you've got things that you want to do in any case where A is true, and then more things you want to do when B
			is true as well, you're forced to use nested branching AFAIK because a single if statement doesn't support that.
			(There are more cases where you have to nest, that was just an example.)

			NOTE: Often, the order in which you write your boolean logic is important for performance. Knowing which test order is more
			efficient requires not only static analysis, but knowledge of what input data your program is going to be given on average and
			what values the variables will have on average. Usually, this can only be done by the human writing the code, which means
			that often, you can't rely on the compiler to convert your nested branches into the best possible version of the boolean logic
			equivalent. Maybe the compiler could look at the rest of your program and see what you're optimizing for, then accordingly optimize
			for the same thing in this case, but that seems a little far-fetched, especially because the compiler could just as well assume
			that the orderings in the rest of your program are just arbitrary and don't mean anything, or even that it has a responsibility to
			fix the orderings to clean up your mess.
			*/

			if (leftDist < rightDist) {
				if (previousKDTreeNodeIndex == noDimChildrenIndex) {
				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

					kdTreePosition = rightKDTreeNodePosition;
					kdTreeSize = rightKDTreeNodeSize;
					currentKDTreeNodeIndex = noDimChildrenIndex + 1;
					splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					continue;
				}
				if (previousKDTreeNodeIndex == noDimChildrenIndex + 1) {
					if (currentKDTreeNodeIndex == 0) { RENDER; break; }

					previousKDTreeNodeIndex = currentKDTreeNodeIndex;
					currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
					splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
					RECONSTRUCT_PARENT;
					goto upwardsTraversalLoop;
				}

				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

				kdTreeSize = leftKDTreeNodeSize;				// TODO: This could be avoided by storing left... in kdTreeSize from getgo. It would need math to be done in the above switch case though, it might be more efficient that way given the probabilities, but I'm not sure, I sort of don't think so because of global illumination.
				currentKDTreeNodeIndex = noDimChildrenIndex;
				splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				continue;
			}

			if (previousKDTreeNodeIndex == noDimChildrenIndex + 1) {
				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

				kdTreeSize = leftKDTreeNodeSize;			// TODO: Again, this could be made more efficient as above, not sure about it though.
				currentKDTreeNodeIndex = noDimChildrenIndex;
				splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				continue;
			}
			if (previousKDTreeNodeIndex == noDimChildrenIndex) {
				if (currentKDTreeNodeIndex == 0) { RENDER; break; }

				previousKDTreeNodeIndex = currentKDTreeNodeIndex;
				currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
				splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
				RECONSTRUCT_PARENT;
				goto upwardsTraversalLoop;
			}

				upwardsTraversalCache[upwardsTraversalCacheSize] = kdTreeSize.x;
				upwardsTraversalCacheSize++;

			kdTreePosition = rightKDTreeNodePosition;
			kdTreeSize = rightKDTreeNodeSize;
			currentKDTreeNodeIndex = noDimChildrenIndex + 1;
			splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
			noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
			continue;
		}

		if (kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount == 0) {
			//write_imageui(frame, coords, (uint4)((currentKDTreeNodeIndex * 100) % 256, 0, 0, 255));
			//return;
		}
		if (kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount != 0) {
			float closestDistance = -1;
			float3 closestHitPoint;
			ulong closestEntityIndex;
			for (ulong i = kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex; i < kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex + kdTreeNodeHeap[currentKDTreeNodeIndex].objectCount; i++) {
				//write_imageui(frame, coords, (uint4)(0, 0, (currentKDTreeNodeIndex * 100) % 256, 255));
				//return;

				float3 pos = entityHeap[leafObjectHeap[i]].position;
				float radius = entityHeap[leafObjectHeap[i]].scale.x;
				float3 offset = (float3)(radius, radius, radius);
				if (rayIntersectAABB(cameraPos, ray, pos - offset, pos + offset) == -1) { continue; }

				float dist;
				bool didItHit;
				float3 hitPoint = intersectWithSphere(cameraPos, ray, entityHeap[leafObjectHeap[i]].position, entityHeap[leafObjectHeap[i]].scale.x, &didItHit, &dist);
				if (didItHit) {
					if (closestDistance == -1 || dist < closestDistance) {
						closestDistance = dist;
						closestHitPoint = hitPoint;
						closestEntityIndex = leafObjectHeap[i];
					}
				}
			}
			if (closestDistance != -1) {
				if (maxBounces > 0) {
					colorProduct *= materialHeap[entityHeap[closestEntityIndex].material].color;
					// TODO: Add point lights somehow. You need to traverse the kd tree for them, which is problematic.
					colorSum += 0 * colorProduct;
					float3 normal = normalize(closestHitPoint - entityHeap[closestEntityIndex].position);
					float dotIncomingRayNormal = dot(ray, normal);			// NOTE: Assumes ray is normalized.
					float3 reflectedRay = ray - dotIncomingRayNormal * 2 * normal;
					float3 diffuseRay = (float3)(randFloat() * 2 - 1, randFloat() * 2 - 1, randFloat() * 2 - 1);
					diffuseRay = normalize(diffuseRay);
					float dotUnadjustedDiffuseRayNormal = dot(diffuseRay, normal);
					if (dotUnadjustedDiffuseRayNormal < 0) { diffuseRay -= dotUnadjustedDiffuseRayNormal * 2 * normal; }
					float3 diffReflectedDiffuse = reflectedRay - diffuseRay;
					ray = diffuseRay + diffReflectedDiffuse * materialHeap[entityHeap[closestEntityIndex].material].reflectivity;
					ray = normalize(ray);
					cameraPos = closestHitPoint;
					maxBounces--;
				} else {
					RENDER;
					break;
				}
			}
		}

		previousKDTreeNodeIndex = currentKDTreeNodeIndex;
		currentKDTreeNodeIndex = kdTreeNodeHeap[currentKDTreeNodeIndex].parentIndex;
		splitDimension = extractDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
		noDimChildrenIndex = removeDimensionValue(kdTreeNodeHeap[currentKDTreeNodeIndex].childrenIndex);
		RECONSTRUCT_PARENT;
		goto upwardsTraversalLoop;
	}
}
//renderColorSum /= 4;
write_imageui(frame, coords, (uint4)(fmin(renderColorSum.x, 1) * 255, fmin(renderColorSum.y, 1) * 255, fmin(renderColorSum.z, 1) * 255, 255));
}