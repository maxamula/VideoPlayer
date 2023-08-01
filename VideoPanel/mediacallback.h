#pragma once
#include "pch.h"
#include <concurrent_queue.h>

#define MEDIAQUEUE_LIMIT 64

namespace VideoPanel
{
	ref class VideoPanel;

	struct VIDEOFRAME_DATA
	{
		ComPtr<ID2D1Bitmap> bitmap = nullptr;
		unsigned long flags = 0;
		uint64 pos = 0;
	};

	struct AUDIOFRAME_DATA
	{
		XAUDIO2_BUFFER buf{};
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
		void Goto(uint64 time);
		void TryQueueSample();

		inline ComPtr<IMFSourceReader> GetReader() { return m_reader; }
		inline IXAudio2SourceVoice* GetVoice() { return m_sourceVoice; }
		inline ComPtr<ID2D1Bitmap> GetCurrentFrame() { return m_currentVideo.bitmap; }
		inline uint64 GetPosition() { return m_position; }
		inline concurrency::critical_section& GetCritSec() { return m_mfcritsec; }
		inline uint64 GetDuration() { return m_duration; }
	private:
		MediaCallback() = default;
		~MediaCallback();
		ComPtr<ID2D1Bitmap> _ProcessVideoFrame(IMFMediaType* pMediaType, IMFSample* pSample);
		
		LONG m_refs = 1;

		ComPtr<IMFSourceReader> m_reader = nullptr;
		IXAudio2SourceVoice* m_sourceVoice = nullptr;

		// Video		
		concurrency::concurrent_queue<VIDEOFRAME_DATA> m_videoQueue{};
		VIDEOFRAME_DATA m_currentVideo{};
		
		// Audio
		Concurrency::concurrent_queue<AUDIOFRAME_DATA> m_audioQueue{};
		AUDIOFRAME_DATA m_currentAudio{};
		std::atomic<bool> m_bEmptyAudioQueue{true};
		

		uint64 m_position = 0;
		uint64 m_duration = 0;

		concurrency::critical_section m_mfcritsec;
		HANDLE m_hFlushEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);

		VideoPanel^ m_panel;
	};
}