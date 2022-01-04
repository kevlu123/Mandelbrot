#pragma once
#include "Application.h"

struct SdlGfxApp : public Application {

	SdlGfxApp(bool vsync) {
		uint32_t renFlags = vsync ? SDL_RendererFlags::SDL_RENDERER_PRESENTVSYNC : NULL;
		if (!(ren = SDL_CreateRenderer(win, -1, renFlags))) {
			Cleanup();
			SdlError();
		}
	}

	virtual ~SdlGfxApp() {
		Cleanup();
	}

	void Render() override {
		SDL_RenderClear(ren);
		if (tex)
			SDL_RenderCopy(ren, tex, nullptr, nullptr);
		SDL_RenderPresent(ren);
	}

protected:

	SDL_Texture* tex = nullptr;
	SDL_Renderer* ren = nullptr;

private:

	void Cleanup() {
		if (tex) SDL_DestroyTexture(tex);
		if (ren) SDL_DestroyRenderer(ren);
	}

};
