#include "DXGraphics.h"
#include <cmath>
#include <d3dcompiler.h>
#include <stdexcept>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
using namespace Microsoft::WRL;

static const char* VERTEX_SHADER_SRC = R"(
cbuffer CBuffer
{
	uint width;
	uint height;
	uint _pad0;
	uint _pad1;
};
	
struct Output
{
	float2 cplx : Complex;
	float4 pos : SV_POSITION;
};
	
Output main(float2 pos : Position, float2 cplx : Complex)
{
	Output output;
	output.pos = float4(
		pos.x / (float)width * 2.0f - 1.0f,
		-(pos.y / (float)height * 2.0f - 1.0f),
		0.5f,
		1
	);
	output.cplx = cplx;
    
	return output;
}
)";

static const char* PIXEL_SHADER_SRC = R"(
static const float4 PALETTE[16] = {
	float4(  0 / 255.0f,   0 / 255.0f,   0 / 255.0f, 1.0f),
	float4( 25 / 255.0f,   7 / 255.0f,  26 / 255.0f, 1.0f),
	float4(  9 / 255.0f,   1 / 255.0f,  47 / 255.0f, 1.0f),
	float4(  4 / 255.0f,   4 / 255.0f,  73 / 255.0f, 1.0f),
	float4(  0 / 255.0f,   7 / 255.0f, 100 / 255.0f, 1.0f),
	float4( 12 / 255.0f,  44 / 255.0f, 138 / 255.0f, 1.0f),
	float4( 24 / 255.0f,  82 / 255.0f, 177 / 255.0f, 1.0f),
	float4( 57 / 255.0f, 125 / 255.0f, 209 / 255.0f, 1.0f),
	float4(134 / 255.0f, 181 / 255.0f, 229 / 255.0f, 1.0f),
	float4(211 / 255.0f, 236 / 255.0f, 248 / 255.0f, 1.0f),
	float4(241 / 255.0f, 233 / 255.0f, 191 / 255.0f, 1.0f),
	float4(248 / 255.0f, 201 / 255.0f,  95 / 255.0f, 1.0f),
	float4(255 / 255.0f, 170 / 255.0f,   0 / 255.0f, 1.0f),
	float4(204 / 255.0f, 128 / 255.0f,   0 / 255.0f, 1.0f),
	float4(153 / 255.0f,  87 / 255.0f,   0 / 255.0f, 1.0f),
	float4(106 / 255.0f,  52 / 255.0f,   3 / 255.0f, 1.0f),
};
	
struct Input
{
	float2 cplx : Complex;
};
	
float4 main(Input input) : SV_TARGET
{
	float2 z = float2(0.0f, 0.0f);
	float2 c = input.cplx;
	uint i;
	for (i = 0; i < 100; i++) {
		float2 sq = float2(
			z.x * z.x - z.y * z.y,
			z.x * z.y * 2.0f
		);
		z = sq + c;
		if (dot(z, z) > 4.0f)
			break;
	}

	return PALETTE[i % 16];
}
)";

struct CBuffer {
	int width;
	int height;
	int _pad[2];
};

struct Vertex {
	float x, y;
	float cx, cy;
};

class InputElementDesc {
public:
	void AddElement(const char* name, DXGI_FORMAT format, UINT offset);
	const std::vector<D3D11_INPUT_ELEMENT_DESC>& Get() const { return ilDesc; }
private:
	std::vector<D3D11_INPUT_ELEMENT_DESC> ilDesc;
};

void InputElementDesc::AddElement(const char* name, DXGI_FORMAT format, UINT offset) {
	ilDesc.push_back({
		name,
		0,
		format,
		0,
		offset,
		D3D11_INPUT_PER_VERTEX_DATA,
		0
		});
}

static std::string HrHex(HRESULT hr) {
	std::string s;
	for (size_t i = 0; i < sizeof(HRESULT) * 2; i++) {
		s = "0123456789ABCDEF"[hr & 0xF] + s;
		hr >>= 4;
	}
	return "0x" + s;
}

static void AssertHResult(HRESULT hr, const std::string& errMsg) {
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DRIVER_INTERNAL_ERROR)
		throw std::runtime_error("Device removed.\n(Code " + HrHex(hr) + ")");
	if (FAILED(hr))
		throw std::runtime_error(errMsg + ".\n(Code " + HrHex(hr) + ")");
}

Shader::Shader(ID3D11Device* device, const std::string& vtxSrc, const std::string& pxSrc, const std::vector<D3D11_INPUT_ELEMENT_DESC>& ilDesc) {
	
	// Compile pixel shader
	ComPtr<ID3DBlob> psBlob;
	AssertHResult(D3DCompile(
		pxSrc.c_str(),
		pxSrc.size(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
#ifdef _DEBUG
		D3DCOMPILE_DEBUG |
#else
		D3DCOMPILE_OPTIMIZATION_LEVEL3 |
#endif
		D3DCOMPILE_ENABLE_STRICTNESS,
		NULL,
		&psBlob,
		nullptr
	), "Failed to compile pixel shader");
	AssertHResult(device->CreatePixelShader(
		psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		nullptr,
		&this->pxShader
	), "Failed to create pixel shader");

	// Compile vertex shader
	ComPtr<ID3DBlob> vsBlob;
	AssertHResult(D3DCompile(
		vtxSrc.c_str(),
		vtxSrc.size(),
		nullptr,
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
#ifdef _DEBUG
		D3DCOMPILE_DEBUG |
#else
		D3DCOMPILE_OPTIMIZATION_LEVEL3 |
#endif
		D3DCOMPILE_ENABLE_STRICTNESS,
		NULL,
		&vsBlob,
		nullptr
	), "Failed to compile vertex shader");
	AssertHResult(device->CreateVertexShader(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		nullptr,
		&this->vtxShader
	), "Failed to create vertex shader");

	// Bind input layout
	AssertHResult(device->CreateInputLayout(
		ilDesc.data(),
		static_cast<UINT>(std::size(ilDesc)),
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		&this->inputLayout
	), "Failed to create input layout");
}

void Shader::Bind(ID3D11DeviceContext* context) {
	context->PSSetShader(
		this->pxShader.Get(),
		nullptr,
		0
	);
	context->VSSetShader(
		this->vtxShader.Get(),
		nullptr,
		0
	);
	context->IASetInputLayout(this->inputLayout.Get());
}

DXGraphics::DXGraphics(HWND hWnd, int width, int height, bool vsync, bool pointFiltering) :
	hWnd(hWnd),
	width(width),
	height(height),
	vsync(vsync)
{
	// Create and bind device, context, and swap chian
	DXGI_SWAP_CHAIN_DESC scDesc{};
	scDesc.BufferCount = 2;
	scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scDesc.BufferDesc.Width = this->width;
	scDesc.BufferDesc.Height = this->height;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.OutputWindow = hWnd;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Windowed = true;
	scDesc.Flags = NULL;
	AssertHResult(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
#ifdef _DEBUG
		D3D11_CREATE_DEVICE_DEBUG,
#else
		NULL,
#endif
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		& scDesc,
		& this->swapChain,
		& this->device,
		nullptr,
		& this->context
	), "Failed to create device, swap chain, and context");

	// Disable internal alt-enter
	ComPtr<IDXGIDevice> dxgiDevice;
	AssertHResult(this->device->QueryInterface(
		__uuidof(IDXGIDevice),
		&dxgiDevice
	), "Failed to get dxgi device");
	ComPtr<IDXGIAdapter> adapter;
	AssertHResult(dxgiDevice->GetParent(
		__uuidof(IDXGIAdapter),
		&adapter
	), "Failed to get dxgi adapter");
	ComPtr<IDXGIFactory> factory;
	AssertHResult(adapter->GetParent(
		__uuidof(IDXGIFactory),
		&factory
	), "Failed to get dxgi factory");
	AssertHResult(factory->MakeWindowAssociation(
		hWnd,
		DXGI_MWA_NO_ALT_ENTER
	), "Failed to disable alt-enter");

	// Create and bind render target view
	ComPtr<ID3D11Texture2D> backBuffer;
	AssertHResult(this->swapChain->GetBuffer(
		0,
		__uuidof(ID3D11Texture2D),
		&backBuffer
	), "Failed to get back buffer of swap chain");
	AssertHResult(this->device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&this->renderTargetView
	), "Failed to create render target view");
	this->context->OMSetRenderTargets(
		1,
		this->renderTargetView.GetAddressOf(),
		nullptr
	);

	// Create shaders
	InputElementDesc mandelbrotIlDesc;
	mandelbrotIlDesc.AddElement("Position", DXGI_FORMAT_R32G32_FLOAT, 0);
	mandelbrotIlDesc.AddElement("Complex", DXGI_FORMAT_R32G32_FLOAT, 8);
	this->mandelbrotShader = std::make_unique<Shader>(
		this->device.Get(),
		VERTEX_SHADER_SRC,
		PIXEL_SHADER_SRC,
		mandelbrotIlDesc.Get()
	);
	mandelbrotShader->Bind(this->context.Get());

	// Create vertex buffer
	Vertex vertices[6]{};
	D3D11_SUBRESOURCE_DATA vbData{};
	vbData.pSysMem = vertices;
	D3D11_BUFFER_DESC vbDesc{};
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.ByteWidth = (UINT)(sizeof(Vertex) * std::size(vertices));
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbDesc.StructureByteStride = sizeof(Vertex);
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	AssertHResult(this->device->CreateBuffer(
		&vbDesc,
		&vbData,
		&this->vertexBuffer
	), "Failed to create vertex buffer");

	// Set viewport
	D3D11_VIEWPORT vp{};
	vp.Width = static_cast<float>(this->width);
	vp.Height = static_cast<float>(this->height);
	vp.MaxDepth = 1.0f;
	vp.MinDepth = 0.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	this->context->RSSetViewports(1, &vp);

	// Set topology
	this->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create and bind constant buffer
	CBuffer* mem = (CBuffer*)_aligned_malloc(sizeof(CBuffer), 16);
	if (!mem)
		throw std::bad_alloc();
	std::shared_ptr<CBuffer> cbuffer(mem, [](auto p) { _aligned_free(p); });
	cbuffer->width = width;
	cbuffer->height = height;
	D3D11_SUBRESOURCE_DATA cbData{};
	cbData.pSysMem = cbuffer.get();
	D3D11_BUFFER_DESC cbDesc{};
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	static_assert(sizeof(CBuffer) % 16 == 0, "CBuffer must be a multiple of 16 bytes");
	cbDesc.ByteWidth = sizeof(CBuffer);
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	AssertHResult(this->device->CreateBuffer(
		&cbDesc,
		&cbData,
		&this->constantBuffer
	), "Failed to create constant buffer");
	this->context->VSSetConstantBuffers(
		0,
		1,
		this->constantBuffer.GetAddressOf()
	);

	D3D11_BLEND_DESC blendDesc{};
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_DEST_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	ComPtr<ID3D11BlendState> blendState;
	AssertHResult(device->CreateBlendState(
		&blendDesc,
		&blendState
	), "Failed to create blend state");
	context->OMSetBlendState(
		blendState.Get(),
		nullptr,
		0xFFFFFFFF
	);
}

void DXGraphics::Present() {
	if (!this->context)
		return;

	// Display frame
	AssertHResult(this->swapChain->Present(
		this->vsync,
		NULL
	), "Errors while presenting swap chain");
	this->context->OMSetRenderTargets(
		1,
		this->renderTargetView.GetAddressOf(),
		nullptr
	);

	float bg[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	this->context->ClearRenderTargetView(
		this->renderTargetView.Get(),
		bg
	);
}

void DXGraphics::DrawMandelbrot(float cxMin, float cxMax, float cyMin, float cyMax) const {
	if (!width || !height)
		return;

	// Generate vertex data
	Vertex vertices[6]{};
	vertices[0].x = 0.0f; vertices[0].y = 0.0f; vertices[0].cx = cxMin; vertices[0].cy = cyMin;
	vertices[1].x = 1.0f; vertices[1].y = 0.0f;	vertices[1].cx = cxMax; vertices[1].cy = cyMin;
	vertices[2].x = 0.0f; vertices[2].y = 1.0f;	vertices[2].cx = cxMin; vertices[2].cy = cyMax;
	vertices[3].x = 1.0f; vertices[3].y = 0.0f;	vertices[3].cx = cxMax; vertices[3].cy = cyMin;
	vertices[4].x = 1.0f; vertices[4].y = 1.0f;	vertices[4].cx = cxMax; vertices[4].cy = cyMax;
	vertices[5].x = 0.0f; vertices[5].y = 1.0f;	vertices[5].cx = cxMin; vertices[5].cy = cyMax;
	for (auto& v : vertices) {
		v.x *= width;
		v.y *= height;
	}
	
	// Update vertex buffer
	D3D11_MAPPED_SUBRESOURCE map{};
	AssertHResult(this->context->Map(
		this->vertexBuffer.Get(),
		0,
		D3D11_MAP_WRITE_DISCARD,
		NULL,
		&map
	), "Failed to map vertex buffer");

	memcpy(map.pData, vertices, std::size(vertices) * sizeof(Vertex));
	this->context->Unmap(this->vertexBuffer.Get(), 0);

	// Bind vertex buffer
	ID3D11Buffer* buffers[]
	{
		this->vertexBuffer.Get(),
	};
	const UINT stride[] = { sizeof(Vertex) };
	const UINT offset[] = { 0 };
	this->context->IASetVertexBuffers(
		0,
		(UINT)std::size(buffers),
		buffers,
		stride,
		offset
	);

	// Draw call
	this->context->Draw(6, 0);
}
