#include "Shader.h"

#include <string>

ErrorCode Shader::setupFromString(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeString, const char* computeKernelName, std::string& buildLog) {
	switch (setupComputeKernelFromString(computeContext, computeDevice, sourceCodeString, computeKernelName, computeProgram, computeKernel, computeKernelWorkGroupSize, buildLog)) {
	case CL_SUCCESS: return ErrorCode::SUCCESS;
	case CL_EXT_CREATE_PROGRAM_FAILED: return ErrorCode::SHADER_CREATE_PROGRAM_FAILED;
	case CL_EXT_INSUFFICIENT_HOST_MEM: return ErrorCode::SHADER_SETUP_FAILED_INSUFFICIENT_HOST_MEM;
	case CL_EXT_GET_BUILD_LOG_FAILED: return ErrorCode::SHADER_GET_BUILD_LOG_FAILED;
	case CL_EXT_BUILD_FAILED_WITH_BUILD_LOG: return ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG;
	case CL_EXT_BUILD_FAILED_WITHOUT_BUILD_LOG: return ErrorCode::SHADER_BUILD_FAILED_WITHOUT_BUILD_LOG;
	case CL_EXT_CREATE_KERNEL_FAILED: return ErrorCode::SHADER_CREATE_KERNEL_FAILED;
	case CL_EXT_GET_KERNEL_WORK_GROUP_INFO_FAILED: return ErrorCode::SHADER_GET_KERNEL_WORK_GROUP_INFO_FAILED;
	}
}

ErrorCode Shader::setupFromFile(cl_context computeContext, cl_device_id computeDevice, const char* sourceCodeFile, const char* computeKernelName, std::string& buildLog) {
	switch (setupComputeKernelFromFile(computeContext, computeDevice, sourceCodeFile, computeKernelName, computeProgram, computeKernel, computeKernelWorkGroupSize, buildLog)) {
	case CL_SUCCESS: return ErrorCode::SUCCESS;
	case CL_EXT_FILE_OPEN_FAILED: return ErrorCode::SHADER_OPEN_SOURCE_CODE_FILE_FAILED;
	case CL_EXT_CREATE_PROGRAM_FAILED: return ErrorCode::SHADER_CREATE_PROGRAM_FAILED;
	case CL_EXT_INSUFFICIENT_HOST_MEM: return ErrorCode::SHADER_SETUP_FAILED_INSUFFICIENT_HOST_MEM;
	case CL_EXT_GET_BUILD_LOG_FAILED: return ErrorCode::SHADER_GET_BUILD_LOG_FAILED;
	case CL_EXT_BUILD_FAILED_WITH_BUILD_LOG: return ErrorCode::SHADER_BUILD_FAILED_WITH_BUILD_LOG;
	case CL_EXT_BUILD_FAILED_WITHOUT_BUILD_LOG: return ErrorCode::SHADER_BUILD_FAILED_WITHOUT_BUILD_LOG;
	case CL_EXT_CREATE_KERNEL_FAILED: return ErrorCode::SHADER_CREATE_KERNEL_FAILED;
	case CL_EXT_GET_KERNEL_WORK_GROUP_INFO_FAILED: return ErrorCode::SHADER_GET_KERNEL_WORK_GROUP_INFO_FAILED;
	}
}
