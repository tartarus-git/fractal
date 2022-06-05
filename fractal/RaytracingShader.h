#pragma once

#include "Shader.h"

class RaytracingShader : public Shader {
public:
	virtual void setBeforeAverageFrameData(cl_mem computeBeforeAverageFrame, uint32_t beforeAverageFrameWidth, uint32_t beforeAverageFrameHeight) = 0;

	virtual void setCameraPosition(nmath::Vector3f position) = 0;
	virtual void setCameraRotation(nmath::Vector3f rotation) = 0;
	virtual void setRayOrigin(float rayOrigin) = 0;

	virtual void setEntityHeap(cl_mem computeEntityHeap, uint64_t computeEntityHeapLength) = 0;
	virtual void setKDTree(nmath::Vector3f position, nmath::Vector3f size, cl_mem computeKDTreeNodeHeap, uint64_t computeKDTreeNodeHeapLength) = 0;
	virtual void setLightHeap(cl_mem computeLightHeap, uint64_t computeLightHeapLength) = 0;

	virtual void setMaterialHeap(cl_mem computeMaterialHeap, uint64_t computeMaterialHeapLength) = 0;
	virtual void setMaterialHeapOffset(uint64_t computeMaterialHeapOffset) = 0;
};
