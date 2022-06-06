#pragma once

#include "Scene.h"
#include "ResourceHeap.h"

#include "Camera.h"

#include "ErrorCode.h"

#include "cl_bindings_and_helpers.h"

#include "RaytracingShader.h"
#include "AveragingShader.h"

#include <cstdint>

enum class ImageChannelOrderType {
	RGBA,
	BGRA,
	ARGB,
	RGB
};

class Renderer
{
	static cl_image_format frameFormat;																// NOTE: Can't just be const, needs to be static const even though that shouldn't really make a difference. Probably enforced just to make you be explicit.
	static unsigned char frameBPP;
	static uint16_t samplesPerPixelSideLength;

	static float baseRayOrigin;

	static size_t computeMaterialHeapOffset;

	static size_t computeFrameOrigin[3];

	static size_t computeBeforeAverageFrameGlobalSize[2];
	static size_t computeBeforeAverageFrameLocalSize[2];
	static size_t computeBeforeAverageFrameRegion[3];

	static size_t computeFrameGlobalSize[2];
	static size_t computeFrameLocalSize[2];
	static size_t computeFrameRegion[3];

	static AveragingShader averagingShader;																					// NOTE: Since I never use AveragingShader's vtable for anything, I assume it gets optimized out, allowing this to be used without overhead.

	static bool initFrameBuffers(uint32_t frameWidth, uint32_t frameHeight);

	static bool allocateBeforeAverageFrameBufferOnDevice();
	static bool allocateFrameBuffersOnDevice();

	static void transferRayOrigin();

public:
	static cl_platform_id computePlatform;
	static cl_device_id computeDevice;
	static cl_context computeContext;
	static cl_command_queue computeCommandQueue;

	static char* beforeAverageFrame;
	static uint32_t beforeAverageFrameWidth;
	static uint32_t beforeAverageFrameHeight;
	static cl_mem computeBeforeAverageFrame;
	static bool computeBeforeAverageFrameAllocated;

	static char* frame;
	static uint32_t frameWidth;
	static uint32_t frameHeight;
	static cl_mem computeFrame;
	static bool computeFrameAllocated;

	static ResourceHeap resources;
	static cl_mem computeMaterialHeap;
	static size_t computeMaterialHeapLength;

	static Scene scene;
	static cl_mem computeEntityHeap;
	static size_t computeEntityHeapLength;
	static cl_mem computeKDTreeNodeHeap;
	static size_t computeKDTreeNodeHeapLength;
	static cl_mem computeLeafObjectHeap;
	static size_t computeLeafObjectHeapLength;
	static cl_mem computeLightHeap;
	static size_t computeLightHeapLength;

	static Camera camera;

	static RaytracingShader* raytracingShader;

	static ErrorCode init(RaytracingShader* raytracingShader, uint16_t samplesPerPixelSideLength, uint32_t frameWidth, uint32_t frameHeight, ImageChannelOrderType frameChannelOrder);

	static ErrorCode resizeFrame(uint32_t newFrameWidth, uint32_t newFrameHeight);							// SIDE-NOTE: class members are implicitly inline. Also, the static modifier doesn't mess with the linkage, it just changes the access pattern (induces classic static behaviour) when used on members.

	static void loadCamera(const Camera& camera);
	static void transferCameraPosition();
	static void transferCameraRotation();
	static void transferCameraFOV();

	static void loadResources(ResourceHeap&& resources);

	// WARNING: A valid state is not garanteed if this function fails.
	static ErrorCode transferResources();

	static void loadScene(Scene&& scene);

	// WARNING: A valid state is not garanteed if this function fails.
	static ErrorCode transferScene();

	static ErrorCode render();

	// WARNING: A valid state is not garanteed if this function fails. This function will try it's best to release all the resources. Even if one step fails, it'll try to release everything as good as possible, so don't worry about that.
	static bool release();
};
