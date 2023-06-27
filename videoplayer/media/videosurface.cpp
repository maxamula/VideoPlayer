#include "videosurface.h"
#include "media.h"

namespace media
{


	VideoSurface::VideoSurface(HWND hWnd, uint16_t width, uint16_t height)
		: m_hWnd(hWnd), m_width(width), m_height(height)
	{
		assert(hWnd != NULL && hWnd != INVALID_HANDLE_VALUE);
		Release();
		DXGI_SWAP_CHAIN_DESC1 scDesc = {};
		scDesc.Width = width;
		scDesc.Height = height;
		scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		scDesc.Stereo = false;
		scDesc.SampleDesc.Count = 1;
		scDesc.SampleDesc.Quality = 0;
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = NUM_BACKBUFFERS;
		scDesc.Scaling = DXGI_SCALING_STRETCH;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

		IDXGISwapChain1* pSwap = NULL;
		HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(g_pDevice, hWnd, &scDesc, NULL, NULL, &pSwap);
		assert(SUCCEEDED(hr));
		hr = pSwap->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&m_pSwap);
		assert(SUCCEEDED(hr));
		m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex();
		pSwap->Release();

		_SetupBackbuffers();
	}

	VideoSurface* VideoSurface::CreateInstance(HWND hWnd, uint16_t width, uint16_t height)
	{
		return new VideoSurface(hWnd, width, height);
	}

	HRESULT __stdcall VideoSurface::QueryInterface(REFIID riid, void** ppv)
	{
		return E_NOINTERFACE;
	}

	ULONG __stdcall VideoSurface::AddRef()
	{
		return InterlockedIncrement(&m_refs);
	}

	ULONG __stdcall VideoSurface::Release()
	{
		ULONG uCount = InterlockedDecrement(&m_refs);
		if (uCount == 0)
		{
			_Release();
			delete this;
		}
		return uCount;
	}

	void VideoSurface::_Release()
	{

	}

	HRESULT __stdcall VideoSurface::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall VideoSurface::OnFlush(DWORD dwStreamIndex)
	{
		return E_NOTIMPL;
	}

	HRESULT __stdcall VideoSurface::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return E_NOTIMPL;
	}

	void VideoSurface::Resize(uint16_t width, uint16_t height)
	{
		
	}

	void VideoSurface::Test()
	{
		ID2D1RenderTarget* d2dRenderTarget = m_backbuffers[m_backBufferIndex].pD2DRenderTarget;
		d2dRenderTarget->BeginDraw();
		//d2dRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Red));
		static float pos = 0.0f;
		D2D1_RECT_F rect = D2D1::RectF(pos, 0.0f, 50.0f, 50.0f);
		pos += 1.0f;

		// Create a Direct2D brush for the rectangle fill color
		ID2D1SolidColorBrush* brush;
		d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &brush);

		// Fill the rectangle
		d2dRenderTarget->FillRectangle(rect, brush);

		// Release the brush
		brush->Release();
		d2dRenderTarget->EndDraw();
		_Present();
	}

	void VideoSurface::_SetupBackbuffers()
	{
		assert(m_pSwap != NULL);
		for (int i = 0; i < NUM_BACKBUFFERS; ++i)
		{
			HRESULT hr = m_pSwap->GetBuffer(i, IID_PPV_ARGS(&m_backbuffers[i].pDXGISurface));
			D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
			g_pD2DFactory->CreateDxgiSurfaceRenderTarget(m_backbuffers[i].pDXGISurface, &renderTargetProperties, &m_backbuffers[i].pD2DRenderTarget);
		}
	}
}