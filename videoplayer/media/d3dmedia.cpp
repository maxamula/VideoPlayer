#include "d3dmedia.h"


namespace media
{
	ID3D12Device8* device = nullptr;
	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGIAdapter4* dxgiAdapter = nullptr;
	DXGI_ADAPTER_DESC1 g_adapterDesc{};

	DescriptorHeap g_rtvHeap;
	DescriptorHeap g_dsvHeap;
	DescriptorHeap g_srvHeap;
	DescriptorHeap g_uavHeap;

	void Initialize()
	{
		// Init COM
		THROW_IF_FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
		// Initialize MMF
		THROW_IF_FAILED(MFStartup(MF_VERSION) == S_OK);
#ifdef _DEBUG
		{
			ID3D12Debug* debugController;
			D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
			debugController->EnableDebugLayer();
		}
#endif
		if (device)
			Shutdown();
		{
			// Create factory
			THROW_IF_FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));
			// Enumerate adapters and find suitable
			IDXGIAdapter1* adapter1 = nullptr;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
				// Check if adapter supports d3d12
				if (SUCCEEDED(D3D12CreateDevice(adapter1, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
				{		
					adapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter)); // Get adapter
					break;
				}
			}
		}

		SET_NAME(device, L"MAIN_DEVICE");

		new (&g_rtvHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 512);
		new (&g_dsvHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 512);
		new (&g_srvHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 4096);
		new (&g_uavHeap) DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 512);

	}

	void Shutdown()
	{

	}
}