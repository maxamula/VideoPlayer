#include "videosurface.h"
#include "media.h"
#include "srcresolver.h"

namespace media
{


	VideoSurface::VideoSurface(HWND hWnd, uint16_t width, uint16_t height)
		: m_hWnd(hWnd), m_width(width), m_height(height)
	{
		assert(hWnd != NULL && hWnd != INVALID_HANDLE_VALUE);
		_Release();
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

		ComPtr<IDXGISwapChain1> pSwap = nullptr;
		if (dxgiFactory->CreateSwapChainForHwnd(g_pDevice, hWnd, &scDesc, nullptr, nullptr, &pSwap) != S_OK)
			_Release();
		assert(pSwap->QueryInterface(IID_PPV_ARGS(&m_pSwap)) == S_OK);
		m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex();
		try
		{
			_SetupBackbuffers();
		}
		catch (...)
		{
			_Release();
		}
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

	HRESULT __stdcall VideoSurface::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		Beep(1000, 1000);
		ComPtr<ID2D1RenderTarget> pTarget = m_backbuffers[m_backBufferIndex].pD2DRenderTarget;
		pTarget->BeginDraw();
		pTarget->Clear(D2D1::ColorF(D2D1::ColorF::Blue));
		static float pos = 0.0f;
		D2D1_RECT_F rect = D2D1::RectF(pos, 0.0f, 50.0f, 50.0f);
		pos += 1.0f;

		// Create a Direct2D brush for the rectangle fill color
		ComPtr<ID2D1SolidColorBrush> brush;
		pTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &brush);

		// Fill the rectangle
		pTarget->FillRectangle(rect, brush.Get());

		pTarget->EndDraw();
		_Present();
		return S_OK;
	}

	HRESULT __stdcall VideoSurface::OnFlush(DWORD dwStreamIndex)
	{
		return S_OK;
	}

	HRESULT __stdcall VideoSurface::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return S_OK;
	}

	void VideoSurface::Resize(uint16_t width, uint16_t height)
	{
		// TODO resize swapchain
	}

	void VideoSurface::Open(const wchar_t* szPath)
	{
		// Create source
		SourceResolver resolver;
		THROW_IF_FAILED(resolver.CreateMediaSource(szPath, &m_pSource));
		// Create reader
		ComPtr<IMFAttributes> pAttributes = nullptr;
		THROW_IF_FAILED(MFCreateAttributes(&pAttributes, 1));
		THROW_IF_FAILED(pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this));
		THROW_IF_FAILED(MFCreateSourceReaderFromMediaSource(m_pSource.Get(), pAttributes.Get(), &m_pSourceReader));
	}

	void VideoSurface::StartPlaying()
	{
		if (!m_pSourceReader)
			return;
		// EROR HERE
		HRESULT hr = m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	}

	void VideoSurface::_SetupBackbuffers()
	{
		assert(m_pSwap != NULL);
		for (int i = 0; i < NUM_BACKBUFFERS; ++i)
		{
			THROW_IF_FAILED(m_pSwap->GetBuffer(i, IID_PPV_ARGS(&m_backbuffers[i].pDXGISurface)));
			D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
			g_pD2DFactory->CreateDxgiSurfaceRenderTarget(m_backbuffers[i].pDXGISurface.Get(), &renderTargetProperties, &m_backbuffers[i].pD2DRenderTarget);
		}
	}
}