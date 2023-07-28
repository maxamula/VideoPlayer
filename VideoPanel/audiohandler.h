#pragma once
#include "pch.h"
#include <concurrent_queue.h>

namespace VideoPanel
{
    enum AUDIO_STATE : uint8_t
    {
        AUDIO_STATE_PLAYING,
        AUDIO_STATE_IDLE
    };

    struct AUDIOFRAME_DATA
    {
        XAUDIO2_BUFFER buf{};
        uint64 pos = 0;
    };

    class AudioHandler : public IXAudio2VoiceCallback
    {
    public:
        AudioHandler() = default;
        ~AudioHandler() { ClearSamplesQueue(); }

        void SetSource(IXAudio2SourceVoice* source);
        void AddSample(AUDIOFRAME_DATA& buf);
        void ClearSamplesQueue();
        void Play();
        inline uint64 GetCurrentTimeStamp() { return m_currentBuffer.pos; };

        void __declspec(nothrow) OnBufferEnd(void* pBufferContext);

        void __declspec(nothrow) OnVoiceProcessingPassEnd() { }
        void __declspec(nothrow) OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    } 
        void __declspec(nothrow) OnBufferStart(void* pBufferContext) {    }
        void __declspec(nothrow) OnLoopEnd(void* pBufferContext) {    }
        void __declspec(nothrow) OnVoiceError(void* pBufferContext, HRESULT Error) { }
        void __declspec(nothrow) OnStreamEnd() { }
    private:
        Concurrency::concurrent_queue<AUDIOFRAME_DATA> m_samples{};
        AUDIOFRAME_DATA m_currentBuffer{};
        std::atomic<AUDIO_STATE> m_state = AUDIO_STATE_IDLE;
        IXAudio2SourceVoice* m_sourceVoice = nullptr;
        std::atomic<bool> m_bQueueEnd = true;
    };
}