#include <Windows.h>
#include <cstdint>
#include <array>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <optional>
#include <memory>
#include "SDL.h"
#include "Stopwatch.h"
#include "Graphics.h"
#include "Task.h"
#include "Mandelbrot.h"

struct Colour {
	uint8_t r, g, b;
};

static const Colour PALETTE[16] = {
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

struct Application {

	static constexpr float FIXED_DELTA_TIME = 1.0f / 200.0f;

	Application(bool cpu) :
		cpu(cpu),
		clientWidth(1280),
		clientHeight(720)
	{
		SDL_Init(SDL_INIT_EVERYTHING);

		constexpr uint32_t WINDOW_FLAGS = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN;
		if (cpu) {
			if (SDL_CreateWindowAndRenderer(clientWidth, clientHeight, WINDOW_FLAGS, &win, &ren))
				throw std::runtime_error(SDL_GetError());
			SDL_SetWindowTitle(win, "Mandelbrot Set");
		} else {
			if (!(win = SDL_CreateWindow("Mandelbrot Set", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, clientWidth, clientHeight, WINDOW_FLAGS)))
				throw std::runtime_error(SDL_GetError());
			hWnd = FindWindowA(nullptr, "Mandelbrot Set");
			InitGpuModeGraphics();
		}
	}

	~Application() {
		if (tex) SDL_DestroyTexture(tex);
		if (ren) SDL_DestroyRenderer(ren);
		if (win) SDL_DestroyWindow(win);
		SDL_Quit();
	}

	void Run() {

		Stopwatch<> stopwatch;
		float curTime = stopwatch.Time();
		float t = 0.0f;
		while (!quit) {
			Update();

			float dt = stopwatch.Time() - curTime;
			curTime += dt;
			t += dt;
			while (t >= FIXED_DELTA_TIME) {
				ProcessInput();
				PollEvents();
				FixedUpdate();
				t -= FIXED_DELTA_TIME;
			}
			Render();
		}
	}

private:

	void Update() {
		SDL_GetWindowSize(win, &clientWidth, &clientHeight);

		// Alt+Enter to toggle fullscreen
		if (GetKey(SDL_Scancode::SDL_SCANCODE_RALT) && GetKeyDown(SDL_Scancode::SDL_SCANCODE_RETURN)) {
			fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(win, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
		}

		// In gpu mode, draw mandelbrot pattern now.
		// In cpu mode, schedule a mandelbrot task to calculate the pattern asynchronously.
		GenerateMandelbrot();
		
		// In cpu mode, check if the mandelbrot task has finished and draw it
		Mandelbrot result;
		if (cpu && mandelbrotTask && mandelbrotTask->PollCompletion(result)) {
			CreateMandelbrotTexture(result);
			mandelbrotTask.reset();
		}
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
				switch (ev.window.event) {
				case SDL_WindowEventID::SDL_WINDOWEVENT_SIZE_CHANGED:
					// In gpu mode, reinitialize graphics with the new window size
					if (!cpu) {
						clientWidth = ev.window.data1;
						clientHeight = ev.window.data2;
						InitGpuModeGraphics();
					}
					break;
				}
				break;
			}
		}
	}

	void Render() {
		if (cpu) {
			SDL_RenderClear(ren);
			if (tex)
				SDL_RenderCopy(ren, tex, nullptr, nullptr);
			SDL_RenderPresent(ren);
		} else {
			gfx->Present();
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

	void InitGpuModeGraphics() {
		gfx.reset();
		gfx = std::make_unique<Graphics>(hWnd, clientWidth, clientHeight, true, true);
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

	// In gpu mode, draw mandelbrot pattern now.
	// In cpu mode, schedule a mandelbrot task to calculate the pattern asynchronously.
	void GenerateMandelbrot() {

		// In cpu mode, skip drawing if nothing has changed
		if (cpu && lastDrawSettings) {
			float zoomDelta = std::abs(zoom - lastDrawSettings->zoom);
			float xCamDelta = std::abs(xCam - lastDrawSettings->xCam);
			float yCamDelta = std::abs(yCam - lastDrawSettings->yCam);
			if (zoom < 250.0f && zoomDelta + xCamDelta + yCamDelta < 0.001f)
				return;
		}

		float aspect = (float)clientWidth / (float)clientHeight;

		float h = 1.0f / zoom;
		float w = aspect * h;

		float xMin = xCam - w / 2.0f;
		float yMin = yCam - h / 2.0f;
		float xMax = xMin + w;
		float yMax = yMin + h;

		if (!cpu) {
			gfx->DrawMandelbrot(xMin, xMax, yMin, yMax);
		} else {
			if (mandelbrotTask)
				return;
			lastDrawSettings = DrawSettings{ zoom, xCam, yCam, xMin, xMax, yMin, yMax, };
			mandelbrotTask = Mandelbrot::ParallelComputeAreaAsync(xMin, xMax, yMin, yMax, clientWidth, clientHeight, std::thread::hardware_concurrency());
		}
	}

	// Create an SDL_Texture from a Mandelbrot object
	void CreateMandelbrotTexture(const Mandelbrot& m) {

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

	// Application properties
	bool cpu = false; // Use cpu instead of gpu
	SDL_Window* win = nullptr;
	bool quit = false;
	bool fullscreen = false;
	int clientWidth = 1;
	int clientHeight = 1;

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

	// Properties used to draw the previous frame
	struct DrawSettings {
		float zoom;
		float xCam;
		float yCam;
		float xMin;
		float xMax;
		float yMin;
		float yMax;
	};
	std::optional<DrawSettings> lastDrawSettings;

	// Custom graphics (gpu mode)
	HWND hWnd = nullptr;
	std::unique_ptr<Graphics> gfx;

	// SDL graphics (cpu mode)
	SDL_Renderer* ren = nullptr;
	std::optional<Task<Mandelbrot>> mandelbrotTask;
	SDL_Texture* tex = nullptr;
};

int APIENTRY WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR cmdArgs, _In_ int) {

	bool cpu = std::string(cmdArgs) == "-cpu";

	try {
		Application(cpu).Run();
	} catch (std::runtime_error& err) {
		MessageBoxA(NULL, err.what(), "Error", MB_OK | MB_ICONERROR);
		return -1;
	}

	return 0;
}
