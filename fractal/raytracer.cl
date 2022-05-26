typedef struct Material {
    float3 color;
} Material;

typedef struct Entity {
    float3 position;
    float3 rotation;
    float3 scale;
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

float3 intersectWithSphere(float3 rayOrigin, float3 ray, float3 spherePosition, float sphereRadius, bool* didItHit) {
    float t = intersectLineSphere(rayOrigin, ray, spherePosition, sphereRadius);
    if (t < 0) { *didItHit = false; return (float3)(0, 0, 0); }
    *didItHit = true;
    return (float3)(rayOrigin + ray * t);
}

float3 rotateVector(float3 vec, float3 rot) {
	float3 result;

    float x = vec.x;
    float y = vec.y;
    float z = vec.z;

	float cosine = cos(rot.y);
	float sine = sin(rot.y);
	result.y = cosine * y - sine * z;
	result.z = sine * y + cosine * z;

	cosine = cos(rot.x);
	sine = sin(rot.x);
	result.x = cosine * x - sine * result.z;
	result.z = sine * x + cosine * result.z;

	cosine = cos(rot.z);
	sine = sin(rot.z);
	float cachedX = result.x;
	result.x = cosine * result.x - sine * result.y;
	result.y = sine * cachedX + cosine * result.y;

	return result;
}

__kernel void traceRays(__write_only image2d_t frame, uint frameWidth, uint frameHeight, float3 cameraPos, float3 cameraRotation, float cameraFOV, 
                        __global Entity* scene, ulong sceneLength, __global Material* materialHeap, ulong materialHeapLength, 
                        ulong materialHeapOffset) {
    int x = get_global_id(0);
    if (x >= frameWidth) { return; }
    int2 coords = (int2)(x, get_global_id(1));

    float3 rayOrigin = (float3)(0, 0, frameHeight / 2.0f / tan(cameraFOV / 2.0f / 180 * 3.14159f));

    float3 ray = (float3)(coords.x - (int)frameWidth / 2, -coords.y + (int)frameHeight / 2, 0) - rayOrigin;
    ray = normalize(ray);
    
    ray = rotateVector(ray, cameraRotation);

    float3 colorCollector = (float3)(0, 0, 0);

int lastCollidedWithObject = 100;
for (int j = 0; j < 400; j++) {
    float3 closestPoint = (float3)(1000, 10000, 10000);
    int closestIndex = 0;
    bool noHits = true;

    for (int i = 0; i < sceneLength; i++) {
        if (i == lastCollidedWithObject) { continue; }
        bool didItHit = false;
        float3 hitPoint = intersectWithSphere(cameraPos, ray, scene[i].position, scene[i].scale.x, &didItHit);
        if (didItHit) {
            //write_imageui(frame, coords, (uint4)(255, 255, 0, 255)); return;
            noHits = false;
            if (length(hitPoint - cameraPos) < length(closestPoint - cameraPos)) { closestPoint = hitPoint; closestIndex = i;
            }
            //write_imageui(frame, coords, (uint4)(fabs(normal.x) * 255, fabs(normal.y) * 255, fabs(normal.z) * multiplier * 255, 1)); return;
        }
    }
    if (!noHits) {

            lastCollidedWithObject = closestIndex;
            if (closestIndex != 2) { 
                
            float3 normal = normalize(closestPoint - scene[closestIndex].position);
            float multiplier = fmax(dot(normal, (float3)(0, 1, 0)), 0);
            //float multiplier = 1;
            float3 color = (float3)(multiplier, multiplier, multiplier);
            colorCollector += color;
            break;
             }

            float3 normal = normalize(closestPoint - scene[closestIndex].position);
            //float multiplier = fmax(dot(normal, (float3)(0, 1, 0)), 0);
            //float multiplier = 1;
            //float3 color = (float3)(multiplier, multiplier, multiplier);
            //colorCollector += color;

            cameraPos = closestPoint;
            float dotval = dot(ray, normal);
            ray -= 2 * dotval * normal;
            colorCollector.z += 0.5f;

        continue; }
        else {
            colorCollector.y += 1;
            break;
        }
}
    //write_imageui(frame, coords, (uint4)(fabs(ray.x) * 255, fabs(ray.y) * 255, fabs(ray.z) * 255, 1));
    write_imageui(frame, coords, (uint4)(255 * fmin(colorCollector.x, 1), 255 * fmin(colorCollector.y, 1), 255 * fmin(colorCollector.z, 1), 1));
}