#include "d3dmanager.h"

#if defined(DEBUG) || defined(_DEBUG)
#ifndef D2D1_DEBUG_LEVEL
#define D2D1_DEBUG_LEVEL D2D1_DEBUG_LEVEL_INFORMATION
#endif
#endif

namespace media
{
	D3DManager::D3DManager()
	{
		assert(_Initialize() == S_OK);
	}

	void D3DManager::SetDX11RenderTargetView(ID3D11RenderTargetView** ppRtv)
	{
		m_context->OMSetRenderTargets(1, ppRtv, nullptr);
	}

	HRESULT D3DManager::CreateRenderTarget(HWND hWnd, uint16_t width, uint16_t height, IDXGISwapChain3** ppSwapchain, ID2D1RenderTarget** pp2dRenderTraget, ID3D11RenderTargetView** ppRenderTarget)
	{
		assert(hWnd != NULL && hWnd != INVALID_HANDLE_VALUE);
		HRESULT hr = S_OK;

		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.Stereo = false;
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = 2;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		ComPtr<IDXGISwapChain1> pSwap = nullptr;
		hr = m_dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), hWnd, &scDesc, nullptr, nullptr, &pSwap);
		if (FAILED(hr)) return hr;
		hr = pSwap->QueryInterface(IID_PPV_ARGS(ppSwapchain));
		if (FAILED(hr)) return hr;

		ComPtr<IDXGISurface> pDxgiSurface = nullptr;
		hr = pSwap->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface));
		if (FAILED(hr)) return hr;
		D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
		hr = m_2dFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, pp2dRenderTraget);
		if (FAILED(hr)) return hr;

		ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
		hr = pSwap->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf));
		if (FAILED(hr)) return hr;
		hr = m_device->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, ppRenderTarget);
		return hr;
	}

	HRESULT D3DManager::ResizeSwapchain(IDXGISwapChain3* pSwapchain, uint16_t width, uint16_t height, ID2D1RenderTarget** pp2dRenderTraget, ID3D11RenderTargetView** ppRenderTarget)
	{
		HRESULT hr = pSwapchain->ResizeBuffers(NUM_BACKBUFFERS, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
		if (FAILED(hr)) return hr;
		ComPtr<IDXGISurface> pDxgiSurface = nullptr;
		hr = pSwapchain->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface));
		if (FAILED(hr)) return hr;
		D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
		hr = m_2dFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, pp2dRenderTraget);
		if (FAILED(hr)) return hr;

		ComPtr<ID3D11Texture2D> pd3dBackBuf = nullptr;
		hr = pSwapchain->GetBuffer(0, IID_PPV_ARGS(&pd3dBackBuf));
		if (FAILED(hr)) return hr;
		hr = m_device->CreateRenderTargetView(pd3dBackBuf.Get(), nullptr, ppRenderTarget);
		return hr;
	}

	HRESULT D3DManager::_Initialize() noexcept
	{
		HRESULT hr = S_OK;
		hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (FAILED(hr)) return hr;
		hr = MFStartup(MF_VERSION);
		if (FAILED(hr)) return hr;

		{
			// Create factory
			hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&m_dxgiFactory));
			if (FAILED(hr)) return hr;
			// Enumerate adapters and find suitable
			IDXGIAdapter1* adapter1 = nullptr;
			for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; ++i)
			{
#ifdef _DEBUG
				UINT flags = D3D11_CREATE_DEVICE_DEBUG | D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#else
				UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif
				const D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
				// Check if adapter supports d3d12
				if (SUCCEEDED(D3D11CreateDevice(adapter1, D3D_DRIVER_TYPE_UNKNOWN, NULL, flags, &featureLevel, 1, D3D11_SDK_VERSION, &m_device, NULL, &m_context)))
				{
					hr = adapter1->QueryInterface(IID_PPV_ARGS(&m_dxgiAdapter)); // Get adapter
					if (FAILED(hr)) return hr;
					break;
				}
			}
		}

		// D2D stuff here
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_2dFactory.GetAddressOf());
		if (FAILED(hr)) return hr;
		D2D1_CREATION_PROPERTIES d2dCreationProps = { D2D1_THREADING_MODE_SINGLE_THREADED, D2D1_DEBUG_LEVEL_INFORMATION };
		ComPtr<IDXGIDevice> pDxgiDevice = nullptr;
		hr = m_device->QueryInterface(IID_PPV_ARGS(&pDxgiDevice));
		if (FAILED(hr)) return hr;
		hr = D2D1CreateDevice(pDxgiDevice.Get(), d2dCreationProps, &m_2dDevice);
		if (FAILED(hr)) return hr;
		hr = m_2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_2dContext);
		return hr;
	}
}