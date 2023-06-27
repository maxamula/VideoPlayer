#pragma once
#include "common.h"

namespace media
{
	class VideoSurface : public IMFSourceReaderCallback
	{
	public:
		struct BACKBUFFER
		{
			IDXGISurface* pDXGISurface = nullptr;
			ID2D1RenderTarget* pD2DRenderTarget = nullptr;
		};
	public:
		DISABLE_MOVE_COPY(VideoSurface);

		static VideoSurface* CreateInstance(HWND hWnd, uint16_t width, uint16_t height);

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;	// Only x64 supported __stdcall is not necessary
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		void Resize(uint16_t width, uint16_t height);
		
		void Test();

		// Accessors
		inline uint16_t GetWidth() const { return m_width; }
		inline uint16_t GetHeight() const { return m_height; }

	protected:
		VideoSurface(HWND hWnd, uint16_t width, uint16_t height);
		void _SetupBackbuffers();
		void _Release();					// Called to free d3d&d2d interfaces and resources if refs reach zero
		inline void _Present() { m_pSwap->Present(0, DXGI_PRESENT_DO_NOT_WAIT); m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex(); }

		IMFSourceReader* pSourceReader = nullptr;
		IDXGISwapChain3* m_pSwap = nullptr;
		BACKBUFFER m_backbuffers[NUM_BACKBUFFERS]{};
		uint8_t m_backBufferIndex = 0;
		HWND m_hWnd = NULL;
		uint16_t m_width = 0;
		uint16_t m_height = 0;
		uint64_t m_refs;

		// Унаследовано через IMFSourceReaderCallback
		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;
	};
}