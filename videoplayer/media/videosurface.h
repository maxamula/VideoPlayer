#pragma once
#include "common.h"

namespace media
{
	class VideoSurface : public IMFSourceReaderCallback
	{
		enum PLAYER_STATE
		{
			PLAYER_STATE_IDLE,
			PLAYER_STATE_PLAYING,
			PLAYER_STATE_PAUSED
		};
	public:
		DISABLE_MOVE_COPY(VideoSurface);

		static VideoSurface* CreateInstance(HWND hWnd, uint16_t width, uint16_t height);

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;	// Only x64 supported __stdcall is not necessary
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		void Resize(uint16_t width, uint16_t height);

		void Open(const wchar_t* szPath);
		void StartPlaying();

		// Accessors
		inline uint16_t GetWidth() const { return m_width; }
		inline uint16_t GetHeight() const { return m_height; }
		inline bool HasVideo() const { return m_pSource && m_pSourceReader; }

	protected:
		VideoSurface(HWND hWnd, uint16_t width, uint16_t height);
		inline void _Present() { m_pSwap->Present(0, DXGI_PRESENT_DO_NOT_WAIT); m_backBufferIndex = m_pSwap->GetCurrentBackBufferIndex(); }

		ComPtr<IMFMediaSource> m_pSource = nullptr;
		ComPtr<IMFSourceReader> m_pSourceReader = nullptr;
		ComPtr<IDXGISwapChain3> m_pSwap = nullptr;

		ComPtr<ID2D1RenderTarget> m_p2DRenderTarget = nullptr;

		uint8_t m_backBufferIndex = 0;
		HWND m_hWnd = NULL;
		PLAYER_STATE m_state = PLAYER_STATE_IDLE;
		uint16_t m_width = 0;
		uint16_t m_height = 0;
		uint64_t m_refs = 1;		
	};
}