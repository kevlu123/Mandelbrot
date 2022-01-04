#pragma once
#include "SdlApp.h"

#define CL_TARGET_OPENCL_VERSION 120
#include "CL/CL.h"

inline const char* CL_SOURCE = R"(
constant const char PALETTE[16 * 4] = {
	  0,   0,   0, 255,
	 25,   7,  26, 255,
	  9,   1,  47, 255,
	  4,   4,  73, 255,
	  0,   7, 100, 255,
	 12,  44, 138, 255,
	 24,  82, 177, 255,
	 57, 125, 209, 255,
	134, 181, 229, 255,
	211, 236, 248, 255,
	241, 233, 191, 255,
	248, 201,  95, 255,
	255, 170,   0, 255,
	204, 128,   0, 255,
	153,  87,   0, 255,
	106,  52,   3, 255,
};

kernel void mandelbrot(float xMin, float dx, float yMin, float dy, int xPx, global char* out) {
	int id = (int)get_global_id(0);
	int x = id % xPx;
	int y = id / xPx;

	float2 z = (float2)(0, 0);
	float2 c = (float2)(xMin + dx * x, yMin + dy * y);
	unsigned int i;
	for (i = 0; i < 100; i++) {
		float2 sq = (float2)(
			z.x * z.x - z.y * z.y,
			z.x * z.y * 2.0f
		);
		z = sq + c;
		if (z.x * z.x + z.y * z.y > 4.0f)
			break;
	}

	i %= 16;
	out[id * 4 + 0] = PALETTE[i * 4 + 0];
	out[id * 4 + 1] = PALETTE[i * 4 + 1];
	out[id * 4 + 2] = PALETTE[i * 4 + 2];
	out[id * 4 + 3] = 255;
}
)";

[[noreturn]] inline void ClError(cl_int error) {

	const char* s = nullptr;
	switch (error) {
		// run-time and JIT compiler errors
	case 0: s = "CL_SUCCESS"; break;
	case -1: s = "CL_DEVICE_NOT_FOUND"; break;
	case -2: s = "CL_DEVICE_NOT_AVAILABLE"; break;
	case -3: s = "CL_COMPILER_NOT_AVAILABLE"; break;
	case -4: s = "CL_MEM_OBJECT_ALLOCATION_FAILURE"; break;
	case -5: s = "CL_OUT_OF_RESOURCES"; break;
	case -6: s = "CL_OUT_OF_HOST_MEMORY"; break;
	case -7: s = "CL_PROFILING_INFO_NOT_AVAILABLE"; break;
	case -8: s = "CL_MEM_COPY_OVERLAP"; break;
	case -9: s = "CL_IMAGE_FORMAT_MISMATCH"; break;
	case -10: s = "CL_IMAGE_FORMAT_NOT_SUPPORTED"; break;
	case -11: s = "CL_BUILD_PROGRAM_FAILURE"; break;
	case -12: s = "CL_MAP_FAILURE"; break;
	case -13: s = "CL_MISALIGNED_SUB_BUFFER_OFFSET"; break;
	case -14: s = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"; break;
	case -15: s = "CL_COMPILE_PROGRAM_FAILURE"; break;
	case -16: s = "CL_LINKER_NOT_AVAILABLE"; break;
	case -17: s = "CL_LINK_PROGRAM_FAILURE"; break;
	case -18: s = "CL_DEVICE_PARTITION_FAILED"; break;
	case -19: s = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"; break;

		// compile-time errors
	case -30: s = "CL_INVALID_VALUE"; break;
	case -31: s = "CL_INVALID_DEVICE_TYPE"; break;
	case -32: s = "CL_INVALID_PLATFORM"; break;
	case -33: s = "CL_INVALID_DEVICE"; break;
	case -34: s = "CL_INVALID_CONTEXT"; break;
	case -35: s = "CL_INVALID_QUEUE_PROPERTIES"; break;
	case -36: s = "CL_INVALID_COMMAND_QUEUE"; break;
	case -37: s = "CL_INVALID_HOST_PTR"; break;
	case -38: s = "CL_INVALID_MEM_OBJECT"; break;
	case -39: s = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
	case -40: s = "CL_INVALID_IMAGE_SIZE"; break;
	case -41: s = "CL_INVALID_SAMPLER"; break;
	case -42: s = "CL_INVALID_BINARY"; break;
	case -43: s = "CL_INVALID_BUILD_OPTIONS"; break;
	case -44: s = "CL_INVALID_PROGRAM"; break;
	case -45: s = "CL_INVALID_PROGRAM_EXECUTABLE"; break;
	case -46: s = "CL_INVALID_KERNEL_NAME"; break;
	case -47: s = "CL_INVALID_KERNEL_DEFINITION"; break;
	case -48: s = "CL_INVALID_KERNEL"; break;
	case -49: s = "CL_INVALID_ARG_INDEX"; break;
	case -50: s = "CL_INVALID_ARG_VALUE"; break;
	case -51: s = "CL_INVALID_ARG_SIZE"; break;
	case -52: s = "CL_INVALID_KERNEL_ARGS"; break;
	case -53: s = "CL_INVALID_WORK_DIMENSION"; break;
	case -54: s = "CL_INVALID_WORK_GROUP_SIZE"; break;
	case -55: s = "CL_INVALID_WORK_ITEM_SIZE"; break;
	case -56: s = "CL_INVALID_GLOBAL_OFFSET"; break;
	case -57: s = "CL_INVALID_EVENT_WAIT_LIST"; break;
	case -58: s = "CL_INVALID_EVENT"; break;
	case -59: s = "CL_INVALID_OPERATION"; break;
	case -60: s = "CL_INVALID_GL_OBJECT"; break;
	case -61: s = "CL_INVALID_BUFFER_SIZE"; break;
	case -62: s = "CL_INVALID_MIP_LEVEL"; break;
	case -63: s = "CL_INVALID_GLOBAL_WORK_SIZE"; break;
	case -64: s = "CL_INVALID_PROPERTY"; break;
	case -65: s = "CL_INVALID_IMAGE_DESCRIPTOR"; break;
	case -66: s = "CL_INVALID_COMPILER_OPTIONS"; break;
	case -67: s = "CL_INVALID_LINKER_OPTIONS"; break;
	case -68: s = "CL_INVALID_DEVICE_PARTITION_COUNT"; break;

		// extension errors
	case -1000: s = "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR"; break;
	case -1001: s = "CL_PLATFORM_NOT_FOUND_KHR"; break;
	case -1002: s = "CL_INVALID_D3D10_DEVICE_KHR"; break;
	case -1003: s = "CL_INVALID_D3D10_RESOURCE_KHR"; break;
	case -1004: s = "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR"; break;
	case -1005: s = "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR"; break;
	default: s = "Unknown OpenCL error"; break;
	}

	throw std::runtime_error(s);
}

struct ClApp : public SdlGfxApp {
	ClApp(bool vsync, cl_device_type deviceType) :
		SdlGfxApp(vsync) {
		cl_int ec = CL_SUCCESS;

		cl_uint platformCount = 0;
		cl_uint deviceCount = 0;

		cl_platform_id platforms[4]{}; // Max 4 platforms
		if (ec = clGetPlatformIDs((cl_uint)std::size(platforms), platforms, &platformCount))
			goto error;

		for (cl_uint i = 0; i < platformCount; i++) {
			if (!(ec = clGetDeviceIDs(platforms[i], deviceType, 1, &device, &deviceCount))) {
				platform = platforms[i];
				break;
			}
		}
		if (ec)
			goto error;

		context = clCreateContext(NULL, 1, &device, NULL, NULL, &ec);
		if (ec)
			goto error;

		commandQueue = clCreateCommandQueue(context, device, NULL, &ec);
		if (ec)
			goto error;

		program = clCreateProgramWithSource(context, 1, &CL_SOURCE, NULL, &ec);
		if (ec)
			goto error;

		if (ec = clBuildProgram(program, 1, &device, NULL, NULL, NULL))
			goto error;

		kernel = clCreateKernel(program, "mandelbrot", &ec);
		if (ec)
			goto error;

		if (ec = RecreateOutputBuffer())
			goto error;

		return;

	error:
		Cleanup();
		ClError(ec);
	}

	virtual ~ClApp() {
		Cleanup();
	}

	void Update() override {

		std::vector<uint8_t> pixels = RunKernel();
		UpdateTexture(pixels);
		fps++;

		if (windowResized) {
			windowResized = false;
			cl_int ec = CL_SUCCESS;
			if (ec = RecreateOutputBuffer())
				ClError(ec);
		}
	}

	void OnWindowResize(int w, int h) override {
		windowResized = true;
	}

private:

	void Cleanup() {
		if (commandQueue) {
			clFlush(commandQueue);
			clFinish(commandQueue);
		}

		if (kernel) clReleaseKernel(kernel);
		if (program) clReleaseProgram(program);
		if (outBuffer) clReleaseMemObject(outBuffer);
		if (commandQueue) clReleaseCommandQueue(commandQueue);
		if (context) clReleaseContext(context);
	}

	std::vector<uint8_t> RunKernel() {

		cl_int ec = CL_SUCCESS;

		// Set arguments
		Viewport vp = GetViewport();
		float dx = (vp.xMax - vp.xMin) / calcWidth;
		float dy = (vp.yMax - vp.yMin) / calcHeight;
		if (ec = clSetKernelArg(kernel, 0, sizeof(float), &vp.xMin))
			ClError(ec);
		if (ec = clSetKernelArg(kernel, 1, sizeof(float), &dx))
			ClError(ec);
		if (ec = clSetKernelArg(kernel, 2, sizeof(float), &vp.yMin))
			ClError(ec);
		if (ec = clSetKernelArg(kernel, 3, sizeof(float), &dy))
			ClError(ec);
		if (ec = clSetKernelArg(kernel, 4, sizeof(int), &calcWidth))
			ClError(ec);
		if (ec = clSetKernelArg(kernel, 5, sizeof(cl_mem), &outBuffer))
			ClError(ec);

		// Run kernel
		size_t globalWorkSize = (size_t)calcWidth * calcHeight;
		if (ec = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, &globalWorkSize, &LOCAL_WORK_SIZE, 0, NULL, NULL))
			ClError(ec);
		clFlush(commandQueue);

		// Read result
		std::vector<uint8_t> pxs((size_t)calcWidth * calcHeight * 4);
		if (ec = clEnqueueReadBuffer(commandQueue, outBuffer, CL_TRUE, 0, pxs.size(), pxs.data(), 0, NULL, NULL))
			ClError(ec);

		return pxs;
	}

	void UpdateTexture(const std::vector<uint8_t>& pixels) {

		if (tex) SDL_DestroyTexture(tex);

		tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, calcWidth, calcHeight);

		SDL_UpdateTexture(tex, nullptr, pixels.data(), 4 * calcWidth);
	}

	cl_int RecreateOutputBuffer() {

		calcWidth = clientWidth;
		calcHeight = clientHeight;

		// Pad width until (calcWidth * calcHeight) is a multiple of LOCAL_WORK_SIZE
		while ((size_t)calcWidth * calcHeight % LOCAL_WORK_SIZE)
			calcWidth++;

		if (outBuffer)
			clReleaseMemObject(outBuffer);

		cl_int ec = CL_SUCCESS;
		size_t outBufSize = (size_t)calcWidth * calcHeight * 4;
		outBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, outBufSize, NULL, &ec);
		return ec;
	}

	static constexpr size_t LOCAL_WORK_SIZE = 64;
	cl_platform_id platform = NULL;
	cl_device_id device = NULL;
	cl_context context = NULL;
	cl_command_queue commandQueue = NULL;
	cl_program program = NULL;
	cl_kernel kernel = NULL;
	cl_mem outBuffer = NULL;
	int calcWidth = 0;
	int calcHeight = 0;
	bool windowResized = false;
};

struct ClCpuApp : public ClApp {
	ClCpuApp(bool vsync) :
		ClApp(vsync, CL_DEVICE_TYPE_CPU) {
	}
};

struct ClGpuApp : public ClApp {
	ClGpuApp(bool vsync) :
		ClApp(vsync, CL_DEVICE_TYPE_GPU) {
	}
};
