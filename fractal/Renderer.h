#pragma once

#include "Scene.h"
#include "ResourceHeap.h"

#include "Camera.h"

#include "ErrorCode.h"

#include "cl_bindings_and_helpers.h"

#include "Shader.h"

#include <cstdint>

class Renderer
{
	static const cl_image_format frameFormat;																// NOTE: Can't just be const, needs to be static const even though that shouldn't really make a difference. Probably enforced just to make you be explicit.

	static size_t computeMaterialHeapOffset;

	static size_t computeGlobalSize[2];
	static size_t computeLocalSize[2];
	static size_t computeFrameOrigin[3];
	static size_t computeFrameRegion[3];

	static bool initFrame(uint32_t frameWidth, uint32_t frameHeight);

	static bool allocateFrameOnDevice();

public:
	static cl_platform_id computePlatform;
	static cl_device_id computeDevice;
	static cl_context computeContext;
	static cl_command_queue computeCommandQueue;

	static char* frame;
	static uint32_t frameWidth;
	static uint32_t frameHeight;
	static cl_mem computeFrame;
	static bool computeFrameAllocated;

	static ResourceHeap resources;
	static cl_mem computeMaterialHeap;
	static size_t computeMaterialHeapLength;

	static Scene scene;
	static cl_mem computeScene;
	static size_t computeSceneLength;

	static Camera camera;

	static Shader* shader;

	static ErrorCode init(Shader* shader, uint32_t frameWidth, uint32_t frameHeight);

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
