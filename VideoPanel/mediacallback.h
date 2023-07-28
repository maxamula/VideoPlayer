#pragma once
#include "pch.h"

#include <concurrent_queue.h>

namespace VideoPanel
{
	ref class VideoPanel;

	struct VIDEOFRAME_DATA
	{
		ComPtr<ID2D1Bitmap> bitmap = nullptr;
		unsigned long flags = 0;
		uint64 pos = 0;
	};

	class MediaCallback : public IMFSourceReaderCallback
	{
	public:
		static MediaCallback* GetInstance(VideoPanel^ videopanel);

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		void Open(Windows::Storage::Streams::IRandomAccessStream^ filestream);
		void TryPresent();
		void TryQueueSample();

		inline ComPtr<IMFSourceReader> GetReader() { return m_reader; }
		inline ComPtr<ID2D1Bitmap> GetCurrentFrame() { std::lock_guard<std::mutex> lock(m_frameMutex); return m_current.bitmap; }
		inline uint64 GetPosition() { std::lock_guard<std::mutex> lock(m_frameMutex); return m_current.pos; }
		inline concurrency::critical_section& GetCritSec() { return m_mfcritsec; }
		inline uint64 GetDuration() { return m_duration; }
		inline bool IsBusy() { return m_bBusy; }
	private:
		MediaCallback() = default;
		LONG m_refs = 1;
		std::atomic<bool> m_bBusy = false;

		ComPtr<IMFSourceReader> m_reader = nullptr;

		// Frame processing
		std::mutex m_frameMutex{};
		VIDEOFRAME_DATA m_current{};
		concurrency::critical_section m_mfcritsec;
		concurrency::concurrent_queue<VIDEOFRAME_DATA> m_frames{};
		uint64 m_duration = 0;
		VideoPanel^ m_panel;
	};
}