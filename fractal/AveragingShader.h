#pragma once

#include "Shader.h"

class AveragingShader : public Shader {
	ErrorCode init(cl_context context, cl_device_id device) override {
		std::string buildLog;
		ErrorCode err = setupFromFile(context, device, "averager.cl", "doAverage", buildLog);
		if (err == ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG) {
			debuglogger::out << buildLog << '\n';
		}
		return err;
	}

public:
	// TODO: set functions.
};
