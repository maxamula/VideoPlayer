#pragma once
#include "common.h"
#include "d3ddescriptorheap.h"

namespace media
{
	class VideoSurface : public IMFSourceReaderCallback
	{
	public:
		struct RENDER_TARGET
		{
			ID3D12Resource* resource = nullptr;
			DESCRIPTOR_HANDLE allocation{};
		};

	public:

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;	// Only x64 supported __stdcall is not necessary
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;

	private:
		IMFSourceReader* pSourceReader = nullptr;
		IDXGISwapChain4* m_pSwap = nullptr;
		D3D12_VIEWPORT m_viewport{};
		D3D12_RECT m_scissiors{};
		RENDER_TARGET m_renderTargets[BACKBUFFER_COUNT]{};
		uint8_t m_backBufferIndex = 0;
		HWND m_hWnd = NULL;
		uint16_t m_width = 0;
		uint16_t m_height = 0;
	};
}