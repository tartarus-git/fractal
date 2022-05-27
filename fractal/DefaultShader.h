#pragma once

#include "Shader.h"
#include "RaytracingShader.h"

#include "cl_bindings_and_helpers.h"
#include <string>

#include "logging/debugOutput.h"

#include "nmath/matrices/Matrix4f.h"

class DefaultShader : public RaytracingShader
{
	ErrorCode init(cl_context context, cl_device_id device) override {
		std::string buildLog;
		ErrorCode err = setupFromFile(context, device, "raytracer.cl", "traceRays", buildLog);
		if (err == ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG) {
			debuglogger::out << buildLog << '\n';
		}
		return err;
	}

	bool release() override {
		return releaseBaseVars();
	}

public:

	// SIDE-NOTE: Difference between nothing, virtual and override while inheriting from virtual classes:
	// You can override virtual functions just fine without writing virtual or override, they are both kind of just syntactic sugar.
	// Virtual is possible in case the user wants to denote that this has something to do with a virtual function. The downside with virtual is that you don't know if it's overriding a virtual function or creating a new virtual function.
	// It also means that if you have an error somewhere in the function name or parameters, it won't throw any error, it just won't override and will instead create a new virtual function, since it has a new signature that hasn't been encountered before but also has the virtual tag.
	// Similar situation also arises when you don't use any tags, if the function doesn't exists, it'll just create a new one without saying anything to you.
	// The no-tag situation I can understand, but why they decided to allow you to use virtual in the case where your overriding I have no idea.
	// Override tag is the best one out of the three possibilities: It throws an error if the function in question isn't overriding anything, forcing you to override something and removing potential bugs. Definitely use override tag.
	// (You can also use virtual and the override tag, which has all the advantages of override plus the syntactic sugar of the virtual keyword, but I don't like that).

	void setBeforeAverageFrameData(cl_mem computeBeforeAverageFrame, cl_uint beforeAverageFrameWidth, cl_uint beforeAverageFrameHeight) override {
		clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &computeBeforeAverageFrame);
		clSetKernelArg(computeKernel, 1, sizeof(cl_uint), &beforeAverageFrameWidth);
		clSetKernelArg(computeKernel, 2, sizeof(cl_uint), &beforeAverageFrameHeight);
	}

	void setCameraPosition(nmath::Vector3f position) override {
		clSetKernelArg(computeKernel, 3, sizeof(nmath::Vector3f), &position);
	}

	void setCameraRotation(nmath::Vector3f rotation) override {
		nmath::Matrix4f rotationMatrix = nmath::Matrix4f::createRotation(rotation);
		clSetKernelArg(computeKernel, 4, sizeof(nmath::Matrix4f), &rotationMatrix);
	}

	void setRayOrigin(float rayOrigin) override {
		clSetKernelArg(computeKernel, 5, sizeof(float), &rayOrigin);
	}

	void setEntityHeap(cl_mem computeEntityHeap, uint64_t computeEntityHeapLength) override {
		clSetKernelArg(computeKernel, 6, sizeof(cl_mem), &computeEntityHeap);
		clSetKernelArg(computeKernel, 7, sizeof(uint64_t), &computeEntityHeapLength);
	}

	void setLightHeap(cl_mem computeLightHeap, uint64_t computeLightHeapLength) override {
		clSetKernelArg(computeKernel, 8, sizeof(cl_mem), &computeLightHeap);
		clSetKernelArg(computeKernel, 9, sizeof(uint64_t), &computeLightHeapLength);
	}

	void setMaterialHeap(cl_mem computeMaterialHeap, uint64_t computeMaterialHeapLength) override {
		clSetKernelArg(computeKernel, 10, sizeof(cl_mem), &computeMaterialHeap);
		clSetKernelArg(computeKernel, 11, sizeof(uint64_t), &computeMaterialHeapLength);
	}

	void setMaterialHeapOffset(uint64_t computeMaterialHeapOffset) override {
		clSetKernelArg(computeKernel, 12, sizeof(uint64_t), &computeMaterialHeapOffset);
	}
};
