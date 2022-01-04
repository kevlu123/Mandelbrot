#pragma once
#include "Application.h"
#include "DXGraphics.h"

struct GpuApp : public Application {

	GpuApp(bool vsync) :
		vsync(vsync) {
		RecreateGraphics();
	}

	void Update() override {
		Viewport vp = GetViewport();
		gfx->DrawMandelbrot(vp.xMin, vp.xMax, vp.yMin, vp.yMax);
		fps++;
	}

	void Render() override {
		gfx->Present();
	}

	void OnWindowResize(int w, int h) override {
		clientWidth = w;
		clientHeight = h;
		RecreateGraphics();
	}

private:

	void RecreateGraphics() {
		gfx.reset();
		gfx = std::make_unique<DXGraphics>(hWnd, clientWidth, clientHeight, vsync, true);
	}

	const bool vsync;
	std::unique_ptr<DXGraphics> gfx;
};
