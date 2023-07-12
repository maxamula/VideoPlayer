#pragma once
#include "common.h"
#include <concurrent_queue.h>

namespace media
{
	enum PLAYER_STATE : uint8_t
	{
		PLAYER_STATE_INVALID,
		PLAYER_STATE_IDLE,
		PLAYER_STATE_PLAYING,
		PLAYER_STATE_PAUSED,
		PLAYER_STATE_HALT
	};

	class VideoSurface : IMFSourceReaderCallback
	{
		friend class FrameHandler;
	public:
		static VideoSurface* Create(HWND hWnd, uint16_t width, uint16_t height);
		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		LRESULT HandleWin32Msg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		HRESULT OpenSource(const wchar_t* szPath);
		HRESULT Resize(uint16_t width, uint16_t height);

		void Play();
		void Pause();

		void Update();

		inline PLAYER_STATE GetState() const { return m_state; }

	private:
		VideoSurface() = default;
		HRESULT _Halt(DWORD timeout);	// Sets renderer to invalid state 
		void _DrawOverlay(LONGLONG& timestamp);
		void _GotoPos(LONGLONG time);
		void _DrawVideoFrame(ID2D1Bitmap* frame);

		static const uint64_t PLAYBACK_START = 0;
		// render target interfaces
		ComPtr<IDXGISwapChain3> m_swap = nullptr;
		ComPtr<ID2D1RenderTarget> m_2dTarget = nullptr;
		ComPtr<ID3D11RenderTargetView> m_target = nullptr;
		HANDLE m_hHaltRenderer = NULL;
		uint16_t m_width = 0, m_height = 0;

		// Video playback
		ComPtr<IMFSourceReader> m_reader = nullptr;
		std::atomic<bool> m_bProcessingFrame{false};

		std::mutex m_frameMutex{};
		ComPtr<ID2D1Bitmap> m_lastFrame = nullptr;
		DWORD m_frameFlags = 0;
		LONGLONG m_currentPos = 0;

		LONGLONG m_duration = 0;

		PLAYER_STATE m_state = PLAYER_STATE_IDLE;

		void* m_overlayContext = nullptr;
		uint32_t m_overlayDuration = 2000;

		LONG m_refs = 1;
	};
}
