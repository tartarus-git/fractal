#pragma once

#include "logging/debugOutput.h"
#include "Shader.h"

#include <cstdint>

class AveragingShader : public Shader {
	friend class Renderer;

	ErrorCode init(cl_context context, cl_device_id device) override {
		std::string buildLog;
		ErrorCode err = setupFromFile(context, device, "averager.cl", "doAverage", buildLog);
		if (err == ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG) {
			debuglogger::out << buildLog << '\n';
		}
		return err;
	}

	bool release() override {
		return releaseBaseVars();
	}

public:

	void setBeforeAverageFrameData(cl_mem computeBeforeAverageFrame, cl_uint beforeAverageFrameWidth, cl_uint beforeAverageFrameHeight) {
		clSetKernelArg(computeKernel, 0, sizeof(cl_mem), &computeBeforeAverageFrame);
		clSetKernelArg(computeKernel, 1, sizeof(cl_uint), &beforeAverageFrameWidth);
		clSetKernelArg(computeKernel, 2, sizeof(cl_uint), &beforeAverageFrameHeight);
	}

	void setSamplesPerPixelSideLength(uint16_t samplesPerPixelSideLength) {
		clSetKernelArg(computeKernel, 3, sizeof(uint16_t), &samplesPerPixelSideLength);
	}

	void setFrameData(cl_mem computeFrame, cl_uint frameWidth, cl_uint frameHeight) {
		clSetKernelArg(computeKernel, 4, sizeof(cl_mem), &computeFrame);
		clSetKernelArg(computeKernel, 5, sizeof(cl_uint), &frameWidth);
		clSetKernelArg(computeKernel, 6, sizeof(cl_uint), &frameHeight);
	}
};
