#pragma once
#include "SdlApp.h"
#include "Mandelbrot.h"

struct CpuApp : public SdlGfxApp {
	CpuApp(bool vsync, bool sync) :
		SdlGfxApp(vsync),
		sync(sync) {
	}

	void Update() override {

		Viewport vp = GetViewport();

		Mandelbrot result;
		bool hasResult = false;

		if (sync) {
			result = ComputeMandelbrot(vp);
			hasResult = true;
		} else {
			if (!mandelbrotTask)
				mandelbrotTask = ComputeMandelbrotAsync(vp);

			if (mandelbrotTask->PollCompletion(result)) {
				hasResult = true;
				mandelbrotTask.reset();
			}
		}

		if (hasResult) {
			UpdateTexture(result);
			fps++;
		}
	}

private:

	Mandelbrot ComputeMandelbrot(Viewport vp) const {
		return Mandelbrot::ComputeArea(
			vp.xMin, vp.xMax,
			vp.yMin, vp.yMax,
			clientWidth, clientHeight
		);
	}

	Task<Mandelbrot> ComputeMandelbrotAsync(Viewport vp) const {
		return Mandelbrot::ParallelComputeAreaAsync(
			vp.xMin, vp.xMax,
			vp.yMin, vp.yMax,
			clientWidth, clientHeight,
			std::thread::hardware_concurrency()
		);
	}

	// Create an SDL_Texture from a Mandelbrot object
	void UpdateTexture(const Mandelbrot& m) {

		if (tex) SDL_DestroyTexture(tex);

		std::vector<uint8_t> px(m.width * m.height * 4, 0xFF);
		for (size_t i = 0; i < px.size(); i += 4) {
			const Colour& c = PALETTE[m.iterCounts[i / 4] % 16];
			px[i + 0] = c.r;
			px[i + 1] = c.g;
			px[i + 2] = c.b;
		}

		tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, m.width, m.height);

		SDL_UpdateTexture(tex, nullptr, px.data(), 4 * m.width);
	}

	const bool sync;
	std::optional<Task<Mandelbrot>> mandelbrotTask;

	struct Colour {
		uint8_t r, g, b;
	};

	inline static const Colour PALETTE[16] = {
		Colour{   0,   0,   0 },
		Colour{  25,   7,  26 },
		Colour{   9,   1,  47 },
		Colour{   4,   4,  73 },
		Colour{   0,   7, 100 },
		Colour{  12,  44, 138 },
		Colour{  24,  82, 177 },
		Colour{  57, 125, 209 },
		Colour{ 134, 181, 229 },
		Colour{ 211, 236, 248 },
		Colour{ 241, 233, 191 },
		Colour{ 248, 201,  95 },
		Colour{ 255, 170,   0 },
		Colour{ 204, 128,   0 },
		Colour{ 153,  87,   0 },
		Colour{ 106,  52,   3 },
	};
};
