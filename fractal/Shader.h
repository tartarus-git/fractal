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

	// GARANTEE: You can use pretty much all of the variables (OpenCL constructs and frame buffers) that we've established in this release function, since all those things get released after the shaders.
	virtual bool release() = 0;

protected:
	ErrorCode setupFromString(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeString, const char* computeKernelName, std::string& buildLog);
	ErrorCode setupFromFile(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeFile, const char* computeKernelName, std::string& buildLog);

	bool releaseBaseVars();

public:
	cl_program computeProgram;
	cl_kernel computeKernel;
	size_t computeKernelWorkGroupSize;
};
