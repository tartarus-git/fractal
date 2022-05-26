#include "Renderer.h"

#include "ErrorCode.h"

#include <new>

// TODO: At end of dev, check how resilient to stupid programming and usage this class is. Like how much does it fall apart when you screw with the mem variables willy nilly.

cl_image_format Renderer::frameFormat;
unsigned char Renderer::frameBPP;

size_t Renderer::computeMaterialHeapOffset = 0;

size_t Renderer::computeGlobalSize[2];
size_t Renderer::computeLocalSize[2];
size_t Renderer::computeFrameOrigin[3];
size_t Renderer::computeFrameRegion[3];

cl_platform_id Renderer::computePlatform;
cl_device_id Renderer::computeDevice;
cl_context Renderer::computeContext;
cl_command_queue Renderer::computeCommandQueue;

char* Renderer::frame;
uint32_t Renderer::frameWidth;
uint32_t Renderer::frameHeight;
cl_mem Renderer::computeFrame;
bool Renderer::computeFrameAllocated = false;

ResourceHeap Renderer::resources;
cl_mem Renderer::computeMaterialHeap;
size_t Renderer::computeMaterialHeapLength = 0;

Scene Renderer::scene;
cl_mem Renderer::computeScene;
size_t Renderer::computeSceneLength = 0;

Camera Renderer::camera;

Shader* Renderer::shader;

bool Renderer::initFrame(uint32_t frameWidth, uint32_t frameHeight) {
	frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
	if (!frame) { return false; }
	Renderer::frameWidth = frameWidth;
	Renderer::frameHeight = frameHeight;
	return true;
}

bool Renderer::allocateFrameOnDevice() {
	cl_int err;
	computeFrame = clCreateImage2D(computeContext, CL_MEM_WRITE_ONLY, &frameFormat, frameWidth, frameHeight, 0, nullptr, &err);
	if (!computeFrame) { return false; }
	shader->setFrameData(computeFrame, frameWidth, frameHeight);
	computeGlobalSize[0] = frameWidth + (shader->computeKernelWorkGroupSize - (frameWidth % shader->computeKernelWorkGroupSize));
	computeGlobalSize[1] = frameHeight;				// TODO: Having the same data twice hanging around the class is stupid, fix this somehow.
	computeFrameRegion[0] = frameWidth;
	computeFrameRegion[1] = frameHeight;
	return true;
}

ErrorCode Renderer::init(Shader* shader, uint32_t frameWidth, uint32_t frameHeight, ImageChannelOrderType frameChannelOrder) {
	if (!initFrame(frameWidth, frameHeight)) { return ErrorCode::FRAME_INIT_FAILED_INSUFFICIENT_HOST_MEM; }

	switch (initOpenCLBindings()) {
	case CL_SUCCESS: break;
	case CL_EXT_DLL_LOAD_FAILURE: delete[] frame; freeOpenCLLib(); return ErrorCode::OPENCL_DLL_LOAD_FAILED;
	case CL_EXT_DLL_FUNC_BIND_FAILURE: delete[] frame; freeOpenCLLib(); return ErrorCode::OPENCL_DLL_FUNC_BIND_FAILED;
	}

	switch (initOpenCLVarsForBestDevice(VersionIdentifier(3, 0), computePlatform, computeDevice, computeContext, computeCommandQueue)) {
	case CL_SUCCESS: break;
	case CL_EXT_NO_PLATFORMS_FOUND: delete[] frame; freeOpenCLLib(); return ErrorCode::NO_ACCELERATION_PLATFORMS_FOUND;
	case CL_EXT_INSUFFICIENT_HOST_MEM: delete[] frame; freeOpenCLLib(); return ErrorCode::DEVICE_DISCOVERY_FAILED_INSUFFICIENT_HOST_MEM;
	case CL_EXT_NO_DEVICES_FOUND_ON_PLATFORM: delete[] frame; freeOpenCLLib(); return ErrorCode::EMPTY_ACCELERATION_PLATFORM_ENCOUNTERED;
	case CL_EXT_NO_DEVICES_FOUND: delete[] frame; freeOpenCLLib(); return ErrorCode::NO_DEVICES_FOUND;
	}

	ErrorCode err = shader->init(computeContext, computeDevice);
	if (err != ErrorCode::SUCCESS) {
		clReleaseCommandQueue(computeCommandQueue);
		clReleaseContext(computeContext);
		delete[] frame;
		freeOpenCLLib();
		return err;
	}
	computeLocalSize[0] = shader->computeKernelWorkGroupSize;
	computeLocalSize[1] = 1;
	computeFrameOrigin[0] = 0;
	computeFrameOrigin[1] = 0;
	computeFrameOrigin[2] = 0;
	computeFrameRegion[2] = 1;

	Renderer::shader = shader;

	if (!allocateFrameOnDevice()) {
		clReleaseCommandQueue(computeCommandQueue);
		clReleaseContext(computeContext);
		delete[] frame;
		freeOpenCLLib();
		return ErrorCode::DEVICE_FRAME_ALLOCATION_FAILED;
	}

	switch (frameChannelOrder) {
	case ImageChannelOrderType::RGBA: frameFormat.image_channel_order = CL_RGBA; frameBPP = 4; break;
	case ImageChannelOrderType::BGRA: frameFormat.image_channel_order = CL_BGRA; frameBPP = 4; break;
	case ImageChannelOrderType::ARGB: frameFormat.image_channel_order = CL_ARGB; frameBPP = 4; break;
	case ImageChannelOrderType::RGB: frameFormat.image_channel_order = CL_RGB; frameBPP = 3; break;
	}
	frameFormat.image_channel_data_type = CL_UNSIGNED_INT8;

	return ErrorCode::SUCCESS;
}

ErrorCode Renderer::resizeFrame(uint32_t newFrameWidth, uint32_t newFrameHeight) {
	uint32_t oldFrameWidth = frameWidth;
	uint32_t oldFrameHeight = frameHeight;
	delete[] frame;
	if (!initFrame(newFrameWidth, newFrameHeight)) {
		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { return ErrorCode::FRAME_FALLBACK_REALLOCATION_FAILED; }
		return ErrorCode::FRAME_RESIZE_FAILED_INSUFFICIENT_HOST_MEM;
	}
	clReleaseMemObject(computeFrame);
	if (!allocateFrameOnDevice()) {
		delete[] frame;
		frameWidth = oldFrameWidth;
		frameHeight = oldFrameHeight;
		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { return ErrorCode::FRAME_FALLBACK_REALLOCATION_FAILED; }
		if (!allocateFrameOnDevice()) { computeFrameAllocated = false; return ErrorCode::FRAME_FALLBACK_DEVICE_REALLOCATION_FAILED; }
		return ErrorCode::FRAME_RESIZE_FAILED_INSUFFICIENT_DEVICE_MEM;
	}
	return ErrorCode::SUCCESS;				// TODO: subsampling and lights as part of scene. Add heaps to scene for kd tree to reference.
}

void Renderer::loadResources(ResourceHeap&& resources) {
	Renderer::resources = std::move(resources);
}

ErrorCode Renderer::transferResources() {
	if (resources.materialHeapLength == 0) { return ErrorCode::INVALID_MATERIAL_HEAP_LENGTH; }
	if (computeMaterialHeapLength != 0) {
		if (resources.materialHeapLength == computeMaterialHeapLength) {
			if (clEnqueueWriteBuffer(computeCommandQueue, computeMaterialHeap, true, 0, computeMaterialHeapLength * sizeof(Material), resources.materialHeap, 0, nullptr, nullptr) != CL_SUCCESS) {
				return ErrorCode::DEVICE_MATERIAL_HEAP_WRITE_FAILED;
			}
			return ErrorCode::SUCCESS;
		}
		if (clReleaseMemObject(computeMaterialHeap) != CL_SUCCESS) { return ErrorCode::DEVICE_RELEASE_MATERIAL_HEAP_FAILED; }
	}
	cl_int err;
	computeMaterialHeap = clCreateBuffer(computeContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, resources.materialHeapLength * sizeof(Material), resources.materialHeap, &err);
	if (!computeMaterialHeap) { computeMaterialHeapLength = 0; return ErrorCode::DEVICE_MATERIAL_HEAP_REALLOCATION_AND_WRITE_FAILED; }
	computeMaterialHeapLength = resources.materialHeapLength;
	shader->setMaterialHeap(computeMaterialHeap, computeMaterialHeapLength);

	if (resources.materialHeapOffset != computeMaterialHeapOffset) {
		shader->setMaterialHeapOffset(resources.materialHeapOffset);
		computeMaterialHeapOffset = resources.materialHeapOffset;
	}

	return ErrorCode::SUCCESS;
}

void Renderer::loadScene(Scene&& scene) {
	Renderer::scene = std::move(scene);
}

ErrorCode Renderer::transferScene() {
	if (scene.length == 0) { return ErrorCode::INVALID_SCENE_LENGTH; }
	if (computeSceneLength != 0) {
		if (scene.length == computeSceneLength) {
			if (clEnqueueWriteBuffer(computeCommandQueue, computeScene, true, 0, computeSceneLength * sizeof(Entity), scene.entities, 0, nullptr, nullptr) != CL_SUCCESS) {
				return ErrorCode::DEVICE_SCENE_WRITE_FAILED;
			}
			return ErrorCode::SUCCESS;
		}
		if (clReleaseMemObject(computeScene) != CL_SUCCESS) { return ErrorCode::DEVICE_RELEASE_SCENE_FAILED; }
	}
	cl_int err;
	computeScene = clCreateBuffer(computeContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.length * sizeof(Entity), scene.entities, &err);
	if (!computeScene) { computeSceneLength = 0; return ErrorCode::DEVICE_SCENE_REALLOCATION_AND_WRITE_FAILED; }
	computeSceneLength = scene.length;
	shader->setScene(computeScene, computeSceneLength);
	return ErrorCode::SUCCESS;
}

void Renderer::loadCamera(const Camera& camera) { Renderer::camera = camera; }
void Renderer::transferCameraPosition() { shader->setCameraPosition(camera.position); }
void Renderer::transferCameraRotation() { shader->setCameraRotation(camera.rotation); }
void Renderer::transferCameraFOV() { shader-> setCameraFOV(camera.FOV); }

ErrorCode Renderer::render() {
	switch(clEnqueueNDRangeKernel(computeCommandQueue, shader->computeKernel, 2, nullptr, computeGlobalSize, computeLocalSize, 0, nullptr, nullptr)) {
	case CL_SUCCESS: break;
	case CL_INVALID_KERNEL_ARGS: return ErrorCode::DEVICE_RENDER_FAILED_KERNEL_ARGS_UNSPECIFIED;
	case CL_OUT_OF_RESOURCES: return ErrorCode::DEVICE_RENDER_FAILED_INSUFFICIENT_MEM;
	default: return ErrorCode::DEVICE_RENDER_FAILED;
	}
	if (clFinish(computeCommandQueue) != CL_SUCCESS) {
		return ErrorCode::DEVICE_WAIT_FOR_RENDER_FAILED;
	}
	if (clEnqueueReadImage(computeCommandQueue, computeFrame, true, computeFrameOrigin, computeFrameRegion, 0, 0, frame, 0, nullptr, nullptr) != CL_SUCCESS) {
		return ErrorCode::READ_DEVICE_FRAME_FAILED;
	}
	return ErrorCode::SUCCESS;
}

bool Renderer::release() {
	delete[] frame;
	bool successful = true;
	if (computeSceneLength != 0 && clReleaseMemObject(computeScene) == CL_SUCCESS) { computeSceneLength = 0; } else { successful = false; }
	if (computeMaterialHeapLength != 0 && clReleaseMemObject(computeMaterialHeap) == CL_SUCCESS) { computeMaterialHeapLength = 0; } else { successful = false; }
	if (computeFrameAllocated && clReleaseMemObject(computeFrame) == CL_SUCCESS) { computeFrameAllocated = false; } else { successful = false; }
	if (clReleaseCommandQueue(computeCommandQueue) != CL_SUCCESS) { successful = false; }
	if (clReleaseContext(computeContext) != CL_SUCCESS) { successful = false; }
	if (!freeOpenCLLib()) { successful = false; }
	return successful;
}
