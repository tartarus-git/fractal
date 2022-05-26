#include "Renderer.h"

#include "ErrorCode.h"

#include <new>

// TODO: At end of dev, check how resilient to stupid programming and usage this class is. Like how much does it fall apart when you screw with the mem variables willy nilly.

cl_image_format Renderer::frameFormat;
unsigned char Renderer::frameBPP;
uint16_t Renderer::samplesPerPixelSideLength;

size_t Renderer::computeMaterialHeapOffset = -1;

size_t Renderer::computeFrameOrigin[3];

size_t Renderer::computeBeforeAverageFrameGlobalSize[2];
size_t Renderer::computeBeforeAverageFrameLocalSize[2];
size_t Renderer::computeBeforeAverageFrameRegion[3];

size_t Renderer::computeFrameGlobalSize[2];
size_t Renderer::computeFrameLocalSize[2];
size_t Renderer::computeFrameRegion[3];

AveragingShader Renderer::averagingShader;

cl_platform_id Renderer::computePlatform;
cl_device_id Renderer::computeDevice;
cl_context Renderer::computeContext;
cl_command_queue Renderer::computeCommandQueue;

char* Renderer::beforeAverageFrame;						// TODO: Get rid of this whole host side before average cache thing since we technically don't need it anymore.
uint32_t Renderer::beforeAverageFrameWidth;
uint32_t Renderer::beforeAverageFrameHeight;
cl_mem Renderer::computeBeforeAverageFrame;
bool Renderer::computeBeforeAverageFrameAllocated = false;

char* Renderer::frame;
uint32_t Renderer::frameWidth;
uint32_t Renderer::frameHeight;
cl_mem Renderer::computeFrame;
bool Renderer::computeFrameAllocated = false;

ResourceHeap Renderer::resources;
cl_mem Renderer::computeMaterialHeap;
size_t Renderer::computeMaterialHeapLength = 0;

Scene Renderer::scene;
cl_mem Renderer::computeEntityHeap;
size_t Renderer::computeEntityHeapLength = 0;
cl_mem Renderer::computeLightHeap;
size_t Renderer::computeLightHeapLength = 0;

Camera Renderer::camera;

RaytracingShader* Renderer::raytracingShader;

bool Renderer::initFrameBuffers(uint32_t frameWidth, uint32_t frameHeight) {
	beforeAverageFrameWidth = frameWidth * samplesPerPixelSideLength;
	beforeAverageFrameHeight = frameHeight * samplesPerPixelSideLength;
	beforeAverageFrame = new (std::nothrow) char[beforeAverageFrameWidth * frameBPP * beforeAverageFrameHeight];
	if (!beforeAverageFrame) { return false; }
	frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
	if (!frame) { delete[] beforeAverageFrame; return false; }
	Renderer::frameWidth = frameWidth;
	Renderer::frameHeight = frameHeight;
	return true;
}

bool Renderer::allocateBeforeAverageFrameBufferOnDevice() {
	cl_int err;
	computeBeforeAverageFrame = clCreateImage2D(computeContext, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS, &frameFormat, beforeAverageFrameWidth, beforeAverageFrameHeight, 0, nullptr, &err);
	if (!computeBeforeAverageFrame) { return false; }
	raytracingShader->setBeforeAverageFrameData(computeBeforeAverageFrame, beforeAverageFrameWidth, beforeAverageFrameHeight);
	averagingShader.setBeforeAverageFrameData(computeBeforeAverageFrame, beforeAverageFrameWidth, beforeAverageFrameHeight);
	computeBeforeAverageFrameGlobalSize[0] = beforeAverageFrameWidth + (raytracingShader->computeKernelWorkGroupSize - (beforeAverageFrameWidth % raytracingShader->computeKernelWorkGroupSize));
	computeBeforeAverageFrameGlobalSize[1] = beforeAverageFrameHeight;
	computeBeforeAverageFrameRegion[0] = beforeAverageFrameWidth;											// NOTE: Having the frame size be expressed multiple times in the class sucks, but the alternative is to spend a little tiny bit of processing power building together these structs every render call,
	computeBeforeAverageFrameRegion[1] = beforeAverageFrameHeight;										// NOTE: which I don't want to do. We could also define frameWidth and frameHeight as references to computeFrameRegion, but that would force me to use size_t, which I also don't want to do.
	return true;
}

bool Renderer::allocateFrameBuffersOnDevice() {
	if (!allocateBeforeAverageFrameBufferOnDevice()) { return false; }

	cl_int err;
	computeFrame = clCreateImage2D(computeContext, CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY, &frameFormat, frameWidth, frameHeight, 0, nullptr, &err);
	if (!computeFrame) { clReleaseMemObject(computeBeforeAverageFrame); return false; }
	averagingShader.setFrameData(computeFrame, frameWidth, frameHeight);
	computeFrameGlobalSize[0] = frameWidth + (averagingShader.computeKernelWorkGroupSize - (frameWidth % averagingShader.computeKernelWorkGroupSize));
	computeFrameGlobalSize[1] = frameHeight;
	computeFrameRegion[0] = frameWidth;											// NOTE: Having the frame size be expressed multiple times in the class sucks, but the alternative is to spend a little tiny bit of processing power building together these structs every render call,
	computeFrameRegion[1] = frameHeight;										// NOTE: which I don't want to do. We could also define frameWidth and frameHeight as references to computeFrameRegion, but that would force me to use size_t, which I also don't want to do.

	return true;
}

ErrorCode Renderer::init(RaytracingShader* raytracingShader, uint16_t samplesPerPixelSideLength, uint32_t frameWidth, uint32_t frameHeight, ImageChannelOrderType frameChannelOrder) {
	Renderer::samplesPerPixelSideLength = samplesPerPixelSideLength;

	switch (frameChannelOrder) {
	case ImageChannelOrderType::RGBA: frameFormat.image_channel_order = CL_RGBA; frameBPP = 4; break;
	case ImageChannelOrderType::BGRA: frameFormat.image_channel_order = CL_BGRA; frameBPP = 4; break;
	case ImageChannelOrderType::ARGB: frameFormat.image_channel_order = CL_ARGB; frameBPP = 4; break;
	case ImageChannelOrderType::RGB: frameFormat.image_channel_order = CL_RGB; frameBPP = 3; break;
	}
	frameFormat.image_channel_data_type = CL_UNSIGNED_INT8;

	if (!initFrameBuffers(frameWidth, frameHeight)) { return ErrorCode::FRAME_INIT_FAILED_INSUFFICIENT_HOST_MEM; }

	switch (initOpenCLBindings()) {
	case CL_SUCCESS: break;
	case CL_EXT_DLL_LOAD_FAILURE: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::OPENCL_DLL_LOAD_FAILED;
	case CL_EXT_DLL_FUNC_BIND_FAILURE: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::OPENCL_DLL_FUNC_BIND_FAILED;
	}

	switch (initOpenCLVarsForBestDevice(VersionIdentifier(3, 0), computePlatform, computeDevice, computeContext, computeCommandQueue)) {
	case CL_SUCCESS: break;
	case CL_EXT_NO_PLATFORMS_FOUND: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::NO_ACCELERATION_PLATFORMS_FOUND;
	case CL_EXT_INSUFFICIENT_HOST_MEM: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::DEVICE_DISCOVERY_FAILED_INSUFFICIENT_HOST_MEM;
	case CL_EXT_NO_DEVICES_FOUND_ON_PLATFORM: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::EMPTY_ACCELERATION_PLATFORM_ENCOUNTERED;
	case CL_EXT_NO_DEVICES_FOUND: delete[] beforeAverageFrame; delete[] frame; freeOpenCLLib(); return ErrorCode::NO_DEVICES_FOUND;
	}

	ErrorCode err = raytracingShader->init(computeContext, computeDevice);
	if (err != ErrorCode::SUCCESS) {
		clReleaseCommandQueue(computeCommandQueue);
		clReleaseContext(computeContext);
		delete[] beforeAverageFrame;
		delete[] frame;
		freeOpenCLLib();
		return err;
	}

	err = averagingShader.init(computeContext, computeDevice);
	if (err != ErrorCode::SUCCESS) {
		raytracingShader->release();
		clReleaseCommandQueue(computeCommandQueue);
		clReleaseContext(computeContext);
		delete[] beforeAverageFrame;
		delete[] frame;
		freeOpenCLLib();
		return err;
	}

	computeFrameOrigin[0] = 0;
	computeFrameOrigin[1] = 0;
	computeFrameOrigin[2] = 0;

	computeBeforeAverageFrameLocalSize[0] = raytracingShader->computeKernelWorkGroupSize;
	computeBeforeAverageFrameLocalSize[1] = 1;
	computeBeforeAverageFrameRegion[2] = 1;

	computeFrameLocalSize[0] = averagingShader.computeKernelWorkGroupSize;
	computeFrameLocalSize[1] = 1;
	computeFrameRegion[2] = 1;

	Renderer::raytracingShader = raytracingShader;

	if (!allocateFrameBuffersOnDevice()) {
		averagingShader.release();
		raytracingShader->release();
		clReleaseCommandQueue(computeCommandQueue);
		clReleaseContext(computeContext);
		delete[] beforeAverageFrame;
		delete[] frame;
		freeOpenCLLib();
		return ErrorCode::DEVICE_FRAME_ALLOCATION_FAILED;
	}

	averagingShader.setSamplesPerPixelSideLength(samplesPerPixelSideLength);

	return ErrorCode::SUCCESS;
}

ErrorCode Renderer::resizeFrame(uint32_t newFrameWidth, uint32_t newFrameHeight) {
	uint32_t oldFrameWidth = frameWidth;
	uint32_t oldFrameHeight = frameHeight;
	delete[] frame;
	if (!initFrameBuffers(newFrameWidth, newFrameHeight)) {
		beforeAverageFrameWidth = frameWidth * samplesPerPixelSideLength;
		beforeAverageFrameHeight = frameHeight * samplesPerPixelSideLength;

		beforeAverageFrame = new (std::nothrow) char[beforeAverageFrameWidth * frameBPP * frameHeight];
		if (!beforeAverageFrame) { return ErrorCode::FRAME_REINIT_FALLBACK_REALLOCATION_FAILED; }

		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { delete[] beforeAverageFrame; return ErrorCode::FRAME_REINIT_FALLBACK_REALLOCATION_FAILED; }

		return ErrorCode::FRAME_REINIT_FAILED_INSUFFICIENT_HOST_MEM;
	}
	if (clReleaseMemObject(computeBeforeAverageFrame) != CL_SUCCESS) {
		frameWidth = oldFrameWidth;
		frameHeight = oldFrameHeight;
		beforeAverageFrameWidth = frameWidth * samplesPerPixelSideLength;
		beforeAverageFrameHeight = frameHeight * samplesPerPixelSideLength;

		delete[] beforeAverageFrame;
		beforeAverageFrame = new (std::nothrow) char[beforeAverageFrameWidth * frameBPP * frameHeight];
		if (!beforeAverageFrame) { return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		delete[] frame;
		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { delete[] beforeAverageFrame; return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		return ErrorCode::DEVICE_RELEASE_FRAME_FAILED;
	}
	if (clReleaseMemObject(computeFrame) != CL_SUCCESS) {
		frameWidth = oldFrameWidth;
		frameHeight = oldFrameHeight;
		beforeAverageFrameWidth = frameWidth * samplesPerPixelSideLength;
		beforeAverageFrameHeight = frameHeight * samplesPerPixelSideLength;

		delete[] beforeAverageFrame;
		beforeAverageFrame = new (std::nothrow) char[beforeAverageFrameWidth * frameBPP * frameHeight];
		if (!beforeAverageFrame) { return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		delete[] frame;
		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { delete[] beforeAverageFrame; return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		if (clReleaseMemObject(computeBeforeAverageFrame) != CL_SUCCESS) {
			delete[] frame;
			delete[] beforeAverageFrame;
			return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_DEVICE_REALLOCATION_FAILED;
		}
		if (!allocateBeforeAverageFrameBufferOnDevice()) {
			delete[] frame;
			delete[] beforeAverageFrame;
			return ErrorCode::DEVICE_RELEASE_FRAME_FALLBACK_DEVICE_REALLOCATION_FAILED;
		}

		return ErrorCode::DEVICE_RELEASE_FRAME_FAILED;
	}
	if (!allocateFrameBuffersOnDevice()) {
		frameWidth = oldFrameWidth;
		frameHeight = oldFrameHeight;
		beforeAverageFrameWidth = frameWidth * samplesPerPixelSideLength;
		beforeAverageFrameHeight = frameHeight * samplesPerPixelSideLength;

		delete[] beforeAverageFrame;
		beforeAverageFrame = new (std::nothrow) char[beforeAverageFrameWidth * frameBPP * frameHeight];
		if (!beforeAverageFrame) { return ErrorCode::DEVICE_REALLOCATE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		delete[] frame;
		frame = new (std::nothrow) char[frameWidth * frameBPP * frameHeight];
		if (!frame) { delete[] beforeAverageFrame; return ErrorCode::DEVICE_REALLOCATE_FRAME_FALLBACK_REALLOCATION_FAILED; }

		if (!allocateFrameBuffersOnDevice()) { computeFrameAllocated = false; return ErrorCode::DEVICE_REALLOCATE_FRAME_FALLBACK_DEVICE_REALLOCATION_FAILED; }

		return ErrorCode::DEVICE_REALLOCATE_FRAME_FAILED_INSUFFICIENT_DEVICE_MEM;
	}
	return ErrorCode::SUCCESS;
}

void Renderer::loadResources(ResourceHeap&& resources) { Renderer::resources = std::move(resources); }

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
	raytracingShader->setMaterialHeap(computeMaterialHeap, computeMaterialHeapLength);

	if (resources.materialHeapOffset != computeMaterialHeapOffset) {
		raytracingShader->setMaterialHeapOffset(resources.materialHeapOffset);
		computeMaterialHeapOffset = resources.materialHeapOffset;
	}

	return ErrorCode::SUCCESS;
}

void Renderer::loadScene(Scene&& scene) { Renderer::scene = std::move(scene); }

ErrorCode Renderer::transferScene() {
	if (scene.entityHeapLength == 0) { return ErrorCode::INVALID_ENTITY_HEAP_LENGTH; }
	if (computeEntityHeapLength != 0) {
		if (scene.entityHeapLength == computeEntityHeapLength) {
			if (clEnqueueWriteBuffer(computeCommandQueue, computeEntityHeap, true, 0, computeEntityHeapLength * sizeof(Entity), scene.entityHeap, 0, nullptr, nullptr) != CL_SUCCESS) {
				return ErrorCode::DEVICE_ENTITY_HEAP_WRITE_FAILED;
			}
			return ErrorCode::SUCCESS;
		}
		if (clReleaseMemObject(computeEntityHeap) != CL_SUCCESS) { return ErrorCode::DEVICE_RELEASE_ENTITY_HEAP_FAILED; }
	}
	cl_int err;
	computeEntityHeap = clCreateBuffer(computeContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.entityHeapLength * sizeof(Entity), scene.entityHeap, &err);
	if (!computeEntityHeap) { computeEntityHeapLength = 0; return ErrorCode::DEVICE_ENTITY_HEAP_REALLOCATION_AND_WRITE_FAILED; }
	computeEntityHeapLength = scene.entityHeapLength;
	raytracingShader->setEntityHeap(computeEntityHeap, computeEntityHeapLength);

	if (scene.lightHeapLength == 0) { return ErrorCode::INVALID_LIGHT_HEAP_LENGTH; }
	if (computeLightHeapLength != 0) {
		if (scene.lightHeapLength == computeLightHeapLength) {
			if (clEnqueueWriteBuffer(computeCommandQueue, computeLightHeap, true, 0, computeLightHeapLength * sizeof(Light), scene.lightHeap, 0, nullptr, nullptr) != CL_SUCCESS) {
				return ErrorCode::DEVICE_LIGHT_HEAP_WRITE_FAILED;
			}
			return ErrorCode::SUCCESS;
		}
		if (clReleaseMemObject(computeLightHeap) != CL_SUCCESS) { return ErrorCode::DEVICE_RELEASE_LIGHT_HEAP_FAILED; }
	}
	computeLightHeap = clCreateBuffer(computeContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, scene.lightHeapLength * sizeof(Light), scene.lightHeap, &err);
	if (!computeLightHeap) { computeLightHeapLength = 0; return ErrorCode::DEVICE_LIGHT_HEAP_REALLOCATION_AND_WRITE_FAILED; }
	computeLightHeapLength = scene.lightHeapLength;
	raytracingShader->setLightHeap(computeLightHeap, computeLightHeapLength);

	return ErrorCode::SUCCESS;
}

void Renderer::loadCamera(const Camera& camera) { Renderer::camera = camera; }
void Renderer::transferCameraPosition() { raytracingShader->setCameraPosition(camera.position); }
void Renderer::transferCameraRotation() { raytracingShader->setCameraRotation(camera.rotation); }
void Renderer::transferCameraFOV() { raytracingShader->setCameraFOV(camera.FOV); }

ErrorCode Renderer::render() {
	switch(clEnqueueNDRangeKernel(computeCommandQueue, raytracingShader->computeKernel, 2, nullptr, computeBeforeAverageFrameGlobalSize, computeBeforeAverageFrameLocalSize, 0, nullptr, nullptr)) {
	case CL_SUCCESS: break;
	case CL_INVALID_KERNEL_ARGS: return ErrorCode::DEVICE_RENDER_FAILED_KERNEL_ARGS_UNSPECIFIED;
	case CL_OUT_OF_RESOURCES: return ErrorCode::DEVICE_RENDER_FAILED_INSUFFICIENT_MEM;
	default: return ErrorCode::DEVICE_RENDER_FAILED;
	}

	switch(clEnqueueNDRangeKernel(computeCommandQueue, averagingShader.computeKernel, 2, nullptr, computeFrameGlobalSize, computeFrameLocalSize, 0, nullptr, nullptr)) {
	case CL_SUCCESS: break;
	case CL_INVALID_KERNEL_ARGS: return ErrorCode::DEVICE_AVERAGE_FAILED_KERNEL_ARGS_UNSPECIFIED;					// TODO: Should I put some clFinish's before these returns?
	case CL_OUT_OF_RESOURCES: return ErrorCode::DEVICE_AVERAGE_FAILED_INSUFFICIENT_MEM;
	default: return ErrorCode::DEVICE_AVERAGE_FAILED;
	}

	if (clFinish(computeCommandQueue) != CL_SUCCESS) {						// TODO: I think this isn't necessary.
		return ErrorCode::DEVICE_WAIT_FOR_RENDER_AND_AVERAGE_FAILED;
	}
	cl_int err;
	if ((err = clEnqueueReadImage(computeCommandQueue, computeFrame, true, computeFrameOrigin, computeFrameRegion, 0, 0, frame, 0, nullptr, nullptr)) != CL_SUCCESS) {
		return ErrorCode::READ_DEVICE_FRAME_FAILED;
	}
	return ErrorCode::SUCCESS;
}

bool Renderer::release() {
	bool successful = true;
	if (!averagingShader.release()) { successful = false; }									// NOTE: We release the shaders as early as possible in the release schedule so their release functions can still play with all the data that they might need.
	if (!raytracingShader->release()) { successful = false; }
	if (computeLightHeapLength != 0 && clReleaseMemObject(computeLightHeap) == CL_SUCCESS) { computeLightHeapLength = 0; } else { successful = false; }
	if (computeEntityHeapLength != 0 && clReleaseMemObject(computeEntityHeap) == CL_SUCCESS) { computeEntityHeapLength = 0; } else { successful = false; }
	if (computeMaterialHeapLength != 0 && clReleaseMemObject(computeMaterialHeap) == CL_SUCCESS) { computeMaterialHeapLength = 0; } else { successful = false; }
	if (computeFrameAllocated && clReleaseMemObject(computeFrame) == CL_SUCCESS) { computeFrameAllocated = false; } else { successful = false; }
	if (clReleaseCommandQueue(computeCommandQueue) != CL_SUCCESS) { successful = false; }
	if (clReleaseContext(computeContext) != CL_SUCCESS) { successful = false; }
	delete[] beforeAverageFrame;
	delete[] frame;
	if (!freeOpenCLLib()) { successful = false; }
	return successful;
}
