#pragma once
#include "common.h"
#include "media.h"
#include <vector>
#include <unordered_map>
#include <deque>
#include <imgui.h>

namespace media
{
	struct VIDEO_SURFACE
	{
		static const uint64_t PLAYBACK_START = 0;
		// render target interfaces
		IDXGISwapChain3* pSwap = nullptr;
		ID2D1RenderTarget* p2dTarget = nullptr;
		ID3D11RenderTargetView* pTarget = nullptr;
		HANDLE hResizeEvent = NULL;
		uint16_t width = 0, height = 0;

		// Video playback
		IMFSourceReader* pReader = nullptr;
		ID2D1Bitmap* pLastFrame = nullptr;
		LONGLONG currentPos = 0;
		LONGLONG duration = 0;

		PLAYER_STATE state = PLAYER_STATE_IDLE;

		ImGuiContext* overlayContext = nullptr;
		uint32_t overlayDuration = 2000;
	};

	class SurfaceManager
	{
	public:
		uint32_t Add(const VIDEO_SURFACE& item);
		void Remove(const uint32_t id);
		VIDEO_SURFACE& Get(const uint32_t id);
		bool IsValid(const uint32_t id) const;

		inline std::unordered_map<uint32_t, VIDEO_SURFACE>::const_iterator begin() { return m_items.begin(); }
		inline std::unordered_map<uint32_t, VIDEO_SURFACE>::const_iterator end() { return m_items.end(); }
	private:
		uint32_t m_nextId = 1;
		uint32_t m_lastDeallocatedId = 0xffffffff;
		std::deque<uint32_t> m_freelist;
		std::unordered_map<uint32_t, VIDEO_SURFACE> m_items;
	};

	//class VideoSurface
	//{
	//public:
	//	static VideoSurface* CreateInstance(HWND hWnd, uint16_t width, uint16_t height);

	//	// IUnknown
	//	HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
	//	ULONG __stdcall AddRef() override;
	//	ULONG __stdcall Release() override;

	//	HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
	//	HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
	//	HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

	//	HRESULT SetInput(const wchar_t* szPath) noexcept;

	//	void ReadNext() const;
	//private:
	//	VideoSurface(HWND hWnd, uint16_t width, uint16_t height);

	//	static std::vector<VideoSurface>
	//	HRESULT _Render(ID2D1Bitmap* pFrame) const noexcept;
	//	ComPtr<IMFSourceReader> m_pReader = nullptr;
	//	ComPtr<IDXGISwapChain3> m_pSwap = nullptr;
	//	ComPtr<ID2D1RenderTarget> m_p2DRenderTarget = nullptr;
	//	uint16_t m_width = 0;
	//	uint16_t m_height = 0;
	//	LONG m_refs = 1;
	//};
}
