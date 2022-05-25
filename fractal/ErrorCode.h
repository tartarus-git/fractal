#pragma once

#include <cstdint>

struct ErrorCode {
public:
	enum {
		SUCCESS,
		FRAME_INIT_FAILED_INSUFFICIENT_HOST_MEM,
		FRAME_FALLBACK_REALLOCATION_FAILED,
		FRAME_RESIZE_FAILED_INSUFFICIENT_HOST_MEM,
		FRAME_FALLBACK_DEVICE_REALLOCATION_FAILED,
		FRAME_RESIZE_FAILED_INSUFFICIENT_DEVICE_MEM,
		OPENCL_DLL_LOAD_FAILED,
		OPENCL_DLL_FUNC_BIND_FAILED,
		NO_ACCELERATION_PLATFORMS_FOUND,
		DEVICE_DISCOVERY_FAILED_INSUFFICIENT_HOST_MEM,
		EMPTY_ACCELERATION_PLATFORM_ENCOUNTERED,
		NO_DEVICES_FOUND,
		DEVICE_FRAME_ALLOCATION_FAILED,
		INVALID_SCENE_LENGTH,
		DEVICE_SCENE_WRITE_FAILED,
		DEVICE_RELEASE_SCENE_FAILED,
		DEVICE_SCENE_REALLOCATION_AND_WRITE_FAILED,
		INVALID_MATERIAL_HEAP_LENGTH,
		DEVICE_MATERIAL_HEAP_WRITE_FAILED,
		DEVICE_RELEASE_MATERIAL_HEAP_FAILED,
		DEVICE_MATERIAL_HEAP_REALLOCATION_AND_WRITE_FAILED,
		SHADER_OPEN_SOURCE_CODE_FILE_FAILED,
		SHADER_CREATE_PROGRAM_FAILED,
		SHADER_SETUP_FAILED_INSUFFICIENT_HOST_MEM,
		SHADER_GET_BUILD_LOG_FAILED,
		SHADER_BUILD_FAILED_WITH_BUILD_LOG,
		SHADER_BUILD_FAILED_WITHOUT_BUILD_LOG,
		SHADER_CREATE_KERNEL_FAILED,
		SHADER_GET_KERNEL_WORK_GROUP_INFO_FAILED,
		DEVICE_RENDER_FAILED_KERNEL_ARGS_UNSPECIFIED,
		DEVICE_RENDER_FAILED_INSUFFICIENT_MEM,
		DEVICE_RENDER_FAILED,
		DEVICE_WAIT_FOR_RENDER_FAILED,
		READ_DEVICE_FRAME_FAILED
	};

private:
	int16_t err;

public:
	constexpr ErrorCode(int16_t err) : err(err) { };									// NOTE: This is here because we need it to function as a converting constructor.
	constexpr explicit operator int16_t() const { return err; }

	constexpr bool operator==(const ErrorCode& right) const { return err == right.err; }
};
