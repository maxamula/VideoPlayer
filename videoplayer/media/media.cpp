#include "media.h"

namespace media
{
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;

	ID2D1Factory* g_pD2DFactory = nullptr;
	ID2D1Device* g_pD2DDevice = nullptr;
	ID2D1DeviceContext* g_pD2DContext = nullptr;

	IDXGIFactory7* dxgiFactory = nullptr;
	IDXGIAdapter4* dxgiAdapter = nullptr;

	void Initialize()
	{
		// Init COM
		THROW_IF_FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
		// Initialize MMF
		THROW_IF_FAILED(MFStartup(MF_VERSION) == S_OK);

		if (g_pDevice)
			Shutdown();
		{
			// Create factory
			THROW_IF_FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory)));
			// Enumerate adapters and find suitable
			IDXGIAdapter1* adapter1 = nullptr;
			for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
#ifdef _DEBUG
				UINT flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
				UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
				const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
				// Check if adapter supports d3d12
				if (SUCCEEDED(D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &featureLevel, 1, D3D11_SDK_VERSION, &g_pDevice, NULL, &g_pContext)))
				{		
					THROW_IF_FAILED(adapter1->QueryInterface(IID_PPV_ARGS(&dxgiAdapter))); // Get adapter
					break;
				}
			}
		}

		// D2D stuff here
		THROW_IF_FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_pD2DFactory));
		D2D1_CREATION_PROPERTIES d2dCreationProps = { D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_INFORMATION };
		IDXGIDevice* pDxgiDevice = nullptr;
		THROW_IF_FAILED(g_pDevice->QueryInterface(IID_PPV_ARGS(&pDxgiDevice)));
		THROW_IF_FAILED(D2D1CreateDevice(pDxgiDevice, d2dCreationProps, &g_pD2DDevice));
		THROW_IF_FAILED(g_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &g_pD2DContext));
		SAFE_RELEASE(pDxgiDevice);
	}

	void Shutdown()
	{

	}
}