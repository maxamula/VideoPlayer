#pragma once
#include "pch.h"
#include "graphics.h"
#include "gpumem.h"
#include <concurrent_queue.h>
#include <variant>
#include <memory>
#include <chrono>

#define MEDIAQUEUE_LIMIT 5

namespace VideoPanel
{
	ref class VideoPanel;

	enum SAMPLE_TYPE : uint8
	{
		SAMPLE_TYPE_VIDEO,
		SAMPLE_TYPE_AUDIO,
		SAMPLE_TYPE_INVALID
	};
	
	struct SAMPLE_DATA
	{
		SAMPLE_TYPE type = SAMPLE_TYPE_INVALID;
		std::variant<std::shared_ptr<GFX::Texture>, XAUDIO2_BUFFER> data;
		unsigned long flags = 0;
		uint64 pos = 0;
	};

	class MediaCallback : public IMFSourceReaderCallback, public IXAudio2VoiceCallback
	{
	public:
		static MediaCallback* GetInstance(VideoPanel^ videopanel);

		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		// IMFSourceReaderCallback
		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		// IXAudio2VoiceCallback
		void __declspec(nothrow) OnBufferEnd(void* pBufferContext);
		inline void __declspec(nothrow) OnVoiceProcessingPassEnd() { }
		inline void __declspec(nothrow) OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
		inline void __declspec(nothrow) OnBufferStart(void* pBufferContext) {    }
		inline void __declspec(nothrow) OnLoopEnd(void* pBufferContext) {    }
		inline void __declspec(nothrow) OnVoiceError(void* pBufferContext, HRESULT Error) { }
		inline void __declspec(nothrow) OnStreamEnd() { }

		void Open(Windows::Storage::Streams::IRandomAccessStream^ filestream);
		void Start();
		void Pause();
		void Stop();
		void Flush(uint64 time);

		inline void GetVideoSize(uint32* pWidth, uint32* pHeight) { if (pWidth) *pWidth = m_videoWidth; if (pHeight) *pHeight = m_videoHeight; }
		inline ComPtr<IMFSourceReader> GetReader() { return m_reader; }
		inline IXAudio2SourceVoice* GetVoice() { return m_sourceVoice; }
		inline std::shared_ptr<GFX::Texture> GetCurrentFrame() { return m_displayedFrame; }
		inline uint64 GetPosition() { return m_position; }
		inline uint64 GetDuration() { return m_duration; }
		void _ProcessMediaQueue() noexcept;
	private:
		MediaCallback() = default;
		~MediaCallback();
		void _DeferredFrameRequest();
		
		LONG m_refs = 1;

		ComPtr<IMFSourceReader> m_reader = nullptr;
		IXAudio2SourceVoice* m_sourceVoice = nullptr;

		// Timing
		ComPtr<IMFPresentationClock> m_clock = nullptr;

		concurrency::concurrent_queue<SAMPLE_DATA> m_mediaQueue{};
		SAMPLE_DATA m_current{};
		
		std::shared_ptr<GFX::Texture> m_displayedFrame = nullptr;

		std::atomic<bool> m_bDeferredFrameRequest{false};
		std::atomic<uint64> m_seeking{uint64_invalid};

		uint64 m_duration = 0;
		uint64 m_position = 0;
		uint32 m_videoWidth = 0;
		uint32 m_videoHeight = 0;

		concurrency::critical_section m_mfcritsec;
		HANDLE m_hFlushEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

		Windows::Foundation::IAsyncAction^ m_queueWorker;

		VideoPanel^ m_panel;
	};
}