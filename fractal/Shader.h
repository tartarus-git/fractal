#pragma once

#include "cl_bindings_and_helpers.h"

#include "ErrorCode.h"

#include "Camera.h"

#include "Scene.h"
#include "ResourceHeap.h"

#include <string>

class Shader
{
	friend class Renderer;							// NOTE: friend is simple, it just allows the following class or function (friend ErrorCode Renderer::render(); for example) access to the private and protected members of this class.

	virtual ErrorCode init(cl_context context, cl_device_id device) = 0;

protected:
	ErrorCode setupFromString(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeString, const char* computeKernelName, std::string& buildLog);
	ErrorCode setupFromFile(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeFile, const char* computeKernelName, std::string& buildLog);

public:
	cl_program computeProgram;
	cl_kernel computeKernel;
	size_t computeKernelWorkGroupSize;

	virtual void setFrameData(cl_mem computeFrame, uint32_t frameWidth, uint32_t frameHeight) = 0;

	virtual void setCameraPosition(nmath::Vector3f position) = 0;
	virtual void setCameraRotation(nmath::Vector3f rotation) = 0;
	virtual void setCameraFOV(float FOV) = 0;

	virtual void setScene(cl_mem computeScene, uint64_t computeSceneLength) = 0;

	virtual void setMaterialHeap(cl_mem computeMaterialHeap, uint64_t computeMaterialHeapLength) = 0;
	virtual void setMaterialHeapOffset(uint64_t computeMaterialHeapOffset) = 0;
};
