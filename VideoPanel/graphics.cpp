#include "pch.h"
#include "graphics.h"
#include "shaders.h"
#include "renderpass.h"
#include "postprocess.h"
#include "upload.h"

namespace VideoPanel::GFX
{
	// Global decl
	// direct3d stuff
	ID3D12Device8* device = nullptr;
	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGIAdapter4* dxgiAdapter = nullptr;
	IDXGIOutput* dxgiOutput = nullptr;

	CommandQueue g_cmdQueue;

	DescriptorHeap g_rtvHeap;
	DescriptorHeap g_dsvHeap;
	DescriptorHeap g_srvHeap;
	DescriptorHeap g_samplerHeap;


	void InitD3D()
	{
		// enable debug layer if in debug mode
#if defined(_DEBUG)
		{
			ID3D12Debug* debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
			debugController->EnableDebugLayer();
		}
#endif
	// Shutdown if initialized
	if (device)
		ShutdownD3D();
	// Select adapter
	{
		// Create factory
		CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
		// Enumerate adapters
		IDXGIAdapter1* adapter1 = nullptr;
		DXGI_ADAPTER_DESC1 adapterDesc{};
		for (uint32 i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			// Check if adapter is compatible
			adapter1->GetDesc1(&adapterDesc);
			if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;
			// Check if adapter supports d3d12
			if (SUCCEEDED(D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
			{
				// Get adapter
				ThrowIfFailed(adapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter)));
				ThrowIfFailed(adapter1->EnumOutputs(0, &dxgiOutput));
				break;
			}
		}
	}
	SET_NAME(device, L"MAIN DEVICE");

	new (&g_cmdQueue) CommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

	new (&g_rtvHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 16);
	new (&g_srvHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 64000);
	new (&g_samplerHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 16);

	// Initialize gfx modules
	try
	{
		Upload::Initialize();
		Shaders::Initialize();
		Render::Initialize();
	}
	catch (...)
	{
		ShutdownD3D();
		abort();
	}
	}

	void ShutdownD3D()
	{
		// Release all resources
		Render::Shutdown();
		Shaders::Shutdown();
		Upload::Shutdown();
		RELEASE(device);
		RELEASE(dxgiFactory);
		RELEASE(dxgiAdapter);
		RELEASE(dxgiOutput);
		g_cmdQueue.Release();
		g_rtvHeap.Release();
		g_srvHeap.Release();
	}

	using namespace DirectX;
}