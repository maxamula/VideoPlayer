#include "videosurface.h"
#include "media.h"
#include <shlwapi.h>

#include <wincodec.h>

namespace media
{
	namespace
	{
		HRESULT CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource)
		{
			HRESULT hr = S_OK;
			ComPtr<IMFSourceResolver> pResolver = nullptr;
			hr = MFCreateSourceResolver(&pResolver);
			if (FAILED(hr))
				return hr;
			if (!ppMediaSource)
				return E_POINTER;
			MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
			IUnknown* pSource = nullptr;
			if (pResolver->CreateObjectFromURL(szwFilePath, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &pSource) != S_OK)
				return E_INVALIDARG;
			assert(ObjectType != MF_OBJECT_INVALID);
			hr = pSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
			return hr;
		}
	}

	/*VideoSurface* VideoSurface::CreateInstance(HWND hWnd, uint16_t width, uint16_t height)
	{
		if (!hWnd)
			return nullptr;
		if (!width && !height)
		{
			RECT rect;
			if (GetWindowRect(hWnd, &rect))
			{
				width = rect.right - rect.left;
				height = rect.bottom - rect.top;
			}
			else
				return nullptr;
		}
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
		HRESULT hr = hrStatus;
		ComPtr<IMFMediaType> pMediaType = nullptr;
		hr = m_pReader->GetCurrentMediaType(dwStreamIndex, &pMediaType);

		GUID pixelFormat;
		pMediaType->GetGUID(MF_MT_SUBTYPE, &pixelFormat);

		UINT32 width = 0, height = 0;
		MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);

		ComPtr<IMFMediaBuffer> buffer;
		pSample->ConvertToContiguousBuffer(&buffer);
		ComPtr<IMFDXGIBuffer> dxgiBuffer;
		buffer.As(&dxgiBuffer);
		BYTE* pBuffer = nullptr;
		DWORD bufferSize = 0;
		buffer->Lock(&pBuffer, nullptr, &bufferSize);

		D2D1_BITMAP_PROPERTIES bitmapProperties;
		bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
		bitmapProperties.dpiX = 96.0f;
		bitmapProperties.dpiY = 96.0f;

		ComPtr<ID2D1Bitmap> pBitmap = nullptr;
		hr = m_p2DRenderTarget->CreateBitmap(D2D1::SizeU(width, height), pBuffer, 1280*4, bitmapProperties, &pBitmap);
		buffer->Unlock();
		
		return _Render(pBitmap.Get());
	}

	HRESULT __stdcall VideoSurface::OnFlush(DWORD dwStreamIndex)
	{
		return S_OK;
	}

	HRESULT __stdcall VideoSurface::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return S_OK;
	}

	HRESULT VideoSurface::SetInput(const wchar_t* szPath) noexcept
	{
		HRESULT hr = S_OK;
		ComPtr<IMFMediaSource> pSource;
		hr = CreateMediaSource(szPath, &pSource);
		if (FAILED(hr)) return hr;
		ComPtr<IMFAttributes> pAttributes = nullptr;
		hr = MFCreateAttributes(&pAttributes, 1);
		if (FAILED(hr)) return hr;
		hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
		if (FAILED(hr)) return hr;
		hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
		if (FAILED(hr)) return hr;
		hr = MFCreateSourceReaderFromMediaSource(pSource.Get(), pAttributes.Get(), &m_pReader);

		ComPtr<IMFMediaType> pNewMediaType = nullptr;
		hr = MFCreateMediaType(&pNewMediaType);

		hr = pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		hr = pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);

		hr = m_pReader->SetCurrentMediaType(
			(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			NULL,
			pNewMediaType.Get()
		);

		hr = m_pReader->SetStreamSelection(
			(DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
			TRUE
		);
		return hr;
	}

	void VideoSurface::ReadNext() const
	{
		m_pReader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
	}

	VideoSurface::VideoSurface(HWND hWnd, uint16_t width, uint16_t height)
		: m_width(width), m_height(height)
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

		ComPtr<IDXGISurface> pDxgiSurface = nullptr;
		assert(m_pSwap->GetBuffer(0, IID_PPV_ARGS(&pDxgiSurface)) == S_OK);
		D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT, D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));
		assert(SUCCEEDED(g_pD2DFactory->CreateDxgiSurfaceRenderTarget(pDxgiSurface.Get(), &renderTargetProperties, m_p2DRenderTarget.GetAddressOf())));
	}
	HRESULT VideoSurface::_Render(ID2D1Bitmap* pFrame) const noexcept
	{
		HRESULT hr = S_OK;
		assert(m_pSwap.Get() && m_p2DRenderTarget.Get());
		m_p2DRenderTarget->BeginDraw();
		m_p2DRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, m_width, m_height);
		m_p2DRenderTarget->DrawBitmap(pFrame, &destRect);
		hr = m_p2DRenderTarget->EndDraw();
		if (FAILED(hr)) return hr;
		hr = m_pSwap->Present(0, 0);
		return hr;
	}*/

	uint32_t SurfaceManager::Add(const VIDEO_SURFACE& item)
	{
		uint32_t id;
		if (m_freelist.empty())
		{
			id = m_nextId++;
		}
		else
		{
			id = m_freelist.front();
			m_freelist.pop_front();
		}
		m_items.insert(std::make_pair(id, item));
		return id;
	}

	void SurfaceManager::Remove(const uint32_t id)
	{
		auto it = m_items.find(id);
		if (it != m_items.end() && id < m_nextId)
		{
			if (id == m_nextId - 1)
			{
				m_nextId--;
				return;
			}
			m_items.erase(it);
			m_freelist.push_back(id);
		}
		else throw std::out_of_range::exception();
	}

	VIDEO_SURFACE& SurfaceManager::Get(const uint32_t id)
	{
		auto it = m_items.find(id);
		if (it != m_items.end())
			return it->second;
		else
			throw std::out_of_range("Invalid ID");
	}

	bool SurfaceManager::IsValid(const uint32_t id) const
	{
		return id != 0xffffffff &&
			id < m_nextId && m_items.find(id) != m_items.end();
	}
}