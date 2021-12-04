#pragma once
#include <d3d11.h>
#include <wrl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <DirectXMath.h>
#include <memory>
#include <unordered_map>

class Shader {
public:
	Shader(ID3D11Device* device, const std::string& vtxSrc, const std::string& pxSrc, const std::vector<D3D11_INPUT_ELEMENT_DESC>& ilDesc);
	void Bind(ID3D11DeviceContext* context);
private:
	Microsoft::WRL::ComPtr<ID3D11VertexShader> vtxShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pxShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
};

class Graphics {
public:
	Graphics(HWND hWnd, int width, int height, bool vsync, bool pointFiltering);
	void DrawMandelbrot(float cxMin, float cxMax, float cyMin, float cyMax) const;
	void Present();
private:
	std::unique_ptr<Shader> mandelbrotShader;
	Microsoft::WRL::ComPtr<ID3D11Device> device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
	const HWND hWnd;
	int width;
	int height;
	bool vsync;
};
