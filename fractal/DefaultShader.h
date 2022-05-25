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
		// TODO: set args in the kernel.
	}

	void setScene(cl_mem computeScene, size_t computeSceneLength) override {
		// TODO: set args in the kernel.
	}

	void setMaterialHeap(cl_mem computeMaterialHeap, size_t computeMaterialHeapLength) override {
		// TODO: set args in the kernel.
	}

	void setMaterialHeapOffset(size_t computeMaterialHeapOffset) override {
		// TODO: set args in the kernel.
	}
};
