#pragma once
#include <Windows.h>
#include <stdint.h>
#include <unordered_map>
#include "SDL.h"
#include "Stopwatch.h"

struct Viewport {
	float xMin;
	float yMin;
	float xMax;
	float yMax;
};

[[noreturn]] inline void SdlError() {
	throw std::runtime_error(SDL_GetError());
}

struct Application {
	Application() :
		clientWidth(1280),
		clientHeight(720) {
		constexpr uint32_t WINDOW_FLAGS = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;

		if (SDL_Init(SDL_INIT_EVERYTHING))
			goto error;

		if (!(win = SDL_CreateWindow(WINDOW_TITLE.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, clientWidth, clientHeight, WINDOW_FLAGS)))
			goto error;

		hWnd = FindWindowA(nullptr, WINDOW_TITLE.c_str());

		return;

	error:
		Cleanup();
		SdlError();
	}

	virtual ~Application() {
		Cleanup();
	}

	void Run() {

		Stopwatch<> stopwatch;
		float curTime = stopwatch.Time();
		float t = 0.0f;
		while (!quit) {

			SDL_GetWindowSize(win, &clientWidth, &clientHeight);

			Update();

			// Fixed update loop
			float dt = stopwatch.Time() - curTime;
			bool secondChanged = (int)curTime != (int)(curTime + dt);
			curTime += dt;
			t += dt;
			while (t >= FIXED_DELTA_TIME) {

				// This section is run as a fixed update because scrollDelta is used in FixedUpdate()
				if (GetKeyDown(SDL_Scancode::SDL_SCANCODE_F11) ||
					(GetKey(SDL_Scancode::SDL_SCANCODE_RALT) && GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN))) {
					// Toggle fullscreen
					fullscreen = !fullscreen;
					SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
				}
				ProcessInput();
				PollEvents();





				FixedUpdate();
				t -= FIXED_DELTA_TIME;
			}

			// Show fps in window title
			if (secondChanged) {
				SDL_SetWindowTitle(win, (WINDOW_TITLE + " - " + std::to_string(fps) + "fps").c_str());
				fps = 0;
			}

			Render();
		}
	}

	virtual void Update() = 0;
	virtual void Render() = 0;
	virtual void OnWindowResize(int w, int h) {}

protected:

	Viewport GetViewport() const {
		float aspect = (float)clientWidth / (float)clientHeight;

		float h = 1.0f / zoom;
		float w = aspect * h;

		Viewport vp{};
		vp.xMin = xCam - w / 2.0f;
		vp.yMin = yCam - h / 2.0f;
		vp.xMax = vp.xMin + w;
		vp.yMax = vp.yMin + h;
		return vp;
	}

	SDL_Window* win = nullptr;
	HWND hWnd = NULL;
	int clientWidth;
	int clientHeight;
	int fps = 0;

private:

	void Cleanup() {
		if (win)
			SDL_DestroyWindow(win);

		SDL_Quit();
	}

	void FixedUpdate() {

		// Update camera position
		float camSpeed = FIXED_DELTA_TIME * 0.01f / zoom;
		if (GetKey(SDL_Scancode::SDL_SCANCODE_A))
			xCamVel -= camSpeed;
		if (GetKey(SDL_Scancode::SDL_SCANCODE_D))
			xCamVel += camSpeed;
		if (GetKey(SDL_Scancode::SDL_SCANCODE_W))
			yCamVel -= camSpeed;
		if (GetKey(SDL_Scancode::SDL_SCANCODE_S))
			yCamVel += camSpeed;
		xCam += xCamVel;
		yCam += yCamVel;
		xCamVel *= 0.95f;
		yCamVel *= 0.95f;

		// Update zoom
		zoomVel += scrollDelta * FIXED_DELTA_TIME * 0.1f * zoom;
		zoomVel *= 0.95f;
		zoom += zoomVel;
		if (zoom < 0.1f)
			zoom = 0.1f;
	}

	void PollEvents() {
		scrollDelta = 0.0f;

		SDL_Event ev{};
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_EventType::SDL_QUIT:
				quit = true;
				break;
			case SDL_EventType::SDL_MOUSEWHEEL:
				if (std::abs(ev.wheel.y) < 50) {
					if (ev.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
						scrollDelta -= ev.wheel.y;
					else
						scrollDelta += ev.wheel.y;
				}
				break;
			case SDL_EventType::SDL_KEYDOWN:
				keyPhases[ev.key.keysym.scancode] = KeyPhase::JustHeld;
				break;
			case SDL_EventType::SDL_KEYUP:
				keyPhases[ev.key.keysym.scancode] = KeyPhase::JustReleased;
				break;
			case SDL_EventType::SDL_WINDOWEVENT:
				if (ev.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_SIZE_CHANGED)
					OnWindowResize(ev.window.data1, ev.window.data2);
				break;
			}
		}
	}

	void ProcessInput() {
		for (auto& key : keyPhases) {
			if (key.second == KeyPhase::JustHeld)
				key.second = KeyPhase::Held;
			else if (key.second == KeyPhase::JustReleased)
				key.second = KeyPhase::NotHeld;
		}
	}

	bool GetKey(SDL_Scancode code) const {
		auto it = keyPhases.find(code);
		return it != keyPhases.end() && (it->second == KeyPhase::JustHeld || it->second == KeyPhase::Held);
	}

	bool GetKeyDown(SDL_Scancode code) const {
		auto it = keyPhases.find(code);
		return it != keyPhases.end() && it->second == KeyPhase::JustHeld;
	}

	bool GetKeyUp(SDL_Scancode code) const {
		auto it = keyPhases.find(code);
		return it != keyPhases.end() && it->second == KeyPhase::JustReleased;
	}

	static constexpr float FIXED_DELTA_TIME = 1.0f / 200.0f;
	inline static const std::string WINDOW_TITLE = "Mandelbrot Set";

	bool quit = false;
	bool fullscreen = false;

	// Input properties
	float scrollDelta = 0.0f;
	enum class KeyPhase {
		NotHeld,
		JustHeld,
		Held,
		JustReleased,
	};
	std::unordered_map<SDL_Scancode, KeyPhase> keyPhases;

	// View properties
	float zoom = 0.3f;
	float zoomVel = 0.0f;
	float xCam = 0.0f;
	float yCam = 0.0f;
	float xCamVel = 0.0f;
	float yCamVel = 0.0f;
};
