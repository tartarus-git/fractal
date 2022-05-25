#pragma once

#include "Shader.h"

#include "cl_bindings_and_helpers.h"
#include <string>

#include "logging/debugOutput.h"

class DefaultShader : public Shader
{				// TODO: Make init invisible to user by handling it through friend stuff.
public:
	// TODO: Find out diff between override virtual and nothng and write it down in a NOTE.
	ErrorCode init(cl_context context, cl_device_id device) override {
		std::string buildLog;
		ErrorCode err = setupFromFile(context, device, "raytracer.cl", "traceRays", buildLog);
		if (err == ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG) {
			debuglogger::out << buildLog << '\n';
		}
		return err;
	}

	void setFrameData(cl_mem computeFrame, cl_uint frameWidth, cl_uint frameHeight) override {
		clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &computeFrame);
		clSetKernelArg(computeKernel, 1, sizeof(cl_uint), &frameWidth);
		clSetKernelArg(computeKernel, 2, sizeof(cl_uint), &frameHeight);
	}

	void setCameraPosition(nmath::Vector3f position) override {
		clSetKernelArg(computeKernel, 3, sizeof(nmath::Vector3f), &position);
	}

	void setCameraRotation(nmath::Vector3f rotation) override {
		clSetKernelArg(computeKernel, 4, sizeof(nmath::Vector3f), &rotation);
	}

	void setCameraFOV(float FOV) override {
		clSetKernelArg(computeKernel, 5, sizeof(nmath::Vector3f), &FOV);
	}

	void setScene(cl_mem computeScene, uint64_t computeSceneLength) override {
		clSetKernelArg(computeKernel, 6, sizeof(cl_mem), &computeScene);
		clSetKernelArg(computeKernel, 7, sizeof(uint64_t), &computeSceneLength);
	}

	void setMaterialHeap(cl_mem computeMaterialHeap, uint64_t computeMaterialHeapLength) override {
		clSetKernelArg(computeKernel, 8, sizeof(cl_mem), &computeMaterialHeap);
		clSetKernelArg(computeKernel, 9, sizeof(uint64_t), &computeMaterialHeapLength);
	}

	void setMaterialHeapOffset(uint64_t computeMaterialHeapOffset) override {
		clSetKernelArg(computeKernel, 10, sizeof(uint64_t), &computeMaterialHeapOffset);
	}
};
