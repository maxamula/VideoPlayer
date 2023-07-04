#include "videosurface.h"
#include "media.h"
#include "srcresolver.h"
#include "d2dconvert.h"
#include <shlwapi.h>

namespace media
{
	VideoSurface::VideoSurface(HWND hWnd, uint16_t width, uint16_t height)
		: m_hWnd(hWnd), m_width(width), m_height(height)
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
		hr = dxgiFactory->CreateSwapChainForHwnd(g_pDevice, hWnd, &scDesc, nullptr, nullptr, &pSwap);
		assert(SUCCEEDED(hr));
		hr = pSwap->QueryInterface(IID_PPV_ARGS(&m_pSwap)) == S_OK;
		assert(SUCCEEDED(hr));
		m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex();

		ComPtr<IDXGISurface> pDxgiSurface = nullptr;
		assert(m_pSwap->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface)) == S_OK);
		D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
		assert(SUCCEEDED(g_pD2DFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, m_p2DRenderTarget.GetAddressOf())));
	}

	VideoSurface* VideoSurface::CreateInstance(HWND hWnd, uint16_t width, uint16_t height)
	{
		return new VideoSurface(hWnd, width, height);
	}

	HRESULT __stdcall VideoSurface::QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] =
		{
			QITABENT(VideoSurface, IMFSourceReaderCallback),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
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
			delete this;
		}
		return uCount;
	}


	HRESULT __stdcall VideoSurface::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		HRESULT hr = S_OK;
		ComPtr<IMFMediaType> pMediaType = nullptr;
		
		// Determine media type (TODO audio)
		hr = m_pSourceReader->GetCurrentMediaType(dwStreamIndex, &pMediaType);

		GUID pixelFormat;
		pMediaType->GetGUID(MF_MT_SUBTYPE, &pixelFormat);

		UINT32 width = 0, height = 0;
		MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);


		ComPtr<IMFMediaBuffer> mediaBuffer = nullptr;
		pSample->ConvertToContiguousBuffer(&mediaBuffer);

		BYTE* pBuffer = nullptr;
		DWORD bufferSize = 0;
		mediaBuffer->Lock(&pBuffer, nullptr, &bufferSize);

		D2D1_BITMAP_PROPERTIES bitmapProperties;
		bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
		bitmapProperties.dpiX = 96.0f;
		bitmapProperties.dpiY = 96.0f;

		ComPtr<ID2D1Bitmap> pBitmap = nullptr;
		m_p2DRenderTarget->CreateBitmap(D2D1::SizeU(1280, 10), pBuffer, bufferSize, bitmapProperties, &pBitmap);

		mediaBuffer->Unlock();


		// DRAW
		m_p2DRenderTarget->BeginDraw();
		m_p2DRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Blue));
		m_p2DRenderTarget->DrawBitmap(pBitmap.Get());
		
		m_p2DRenderTarget->EndDraw();
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
		StartPlaying();
		m_state = PLAYER_STATE_PLAYING;
	}

	void VideoSurface::StartPlaying()
	{
		if (!m_pSourceReader)
			return;
		// EROR HERE
		HRESULT hr = m_pSourceReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, NULL, NULL, NULL, NULL);
	}
}