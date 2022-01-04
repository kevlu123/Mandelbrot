#include <Windows.h>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

#include "CpuApp.h"
#include "GpuApp.h"
#include "ClApp.h"

enum class Backend {
	Cpu,	// C++
	Gpu,	// D3D11
	ClCpu,  // OpenCL cpu
	ClGpu,	// OpenCL gpu
};

static std::string Lowercase(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), std::tolower);
	return s;
}

static std::vector<std::string> SplitString(std::string s, char splitOn = ' ') {
	std::vector<std::string> ret;
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(splitOn)) != std::string::npos) {
		token = s.substr(0, pos);
		ret.push_back(token);
		s.erase(0, pos + 1);
	}
	ret.push_back(s);
	return ret;
}

int APIENTRY WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR cmdArgs, _In_ int) {

	auto args = SplitString(Lowercase(cmdArgs));
	auto ContainsArg = [&](const std::string& arg) {
		return std::find(args.begin(), args.end(), arg) != args.end();
	};

	bool sync = ContainsArg("-sync");
	bool vsync = ContainsArg("-vsync");

	Backend backend;
	if (ContainsArg("-cpu"))		backend = Backend::Cpu;
	else if (ContainsArg("-gpu"))	backend = Backend::Gpu;
	else if (ContainsArg("-clcpu")) backend = Backend::ClCpu;
	else							backend = Backend::ClGpu;

	try {
		std::unique_ptr<Application> app;
		switch (backend) {
		case Backend::Cpu:		app = std::make_unique<CpuApp>(vsync, sync);	break;
		case Backend::Gpu:		app = std::make_unique<GpuApp>(vsync);			break;
		case Backend::ClCpu:	app = std::make_unique<ClCpuApp>(vsync);		break;
		case Backend::ClGpu:	app = std::make_unique<ClGpuApp>(vsync);		break;
		}
		app->Run();
	} catch (std::runtime_error& err) {
		MessageBoxA(NULL, err.what(), "Error", MB_OK | MB_ICONERROR);
		return -1;
	}

	return 0;
}
