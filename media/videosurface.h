#pragma once
#include "common.h"
#include <vector>

namespace media
{
	class VideoSurface : public IMFSourceReaderCallback
	{
	public:
		static VideoSurface* CreateInstance(HWND hWnd, uint16_t width, uint16_t height);

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		HRESULT SetInput(const wchar_t* szPath) noexcept;

		void ReadNext() const;
	private:
		VideoSurface(HWND hWnd, uint16_t width, uint16_t height);
		HRESULT _Render(ID2D1Bitmap* pFrame) const noexcept;
		ComPtr<IMFSourceReader> m_pReader = nullptr;
		ComPtr<IDXGISwapChain3> m_pSwap = nullptr;
		ComPtr<ID2D1RenderTarget> m_p2DRenderTarget = nullptr;
		uint16_t m_width = 0;
		uint16_t m_height = 0;
		LONG m_refs = 1;
	};
}
