#pragma once
#include "common.h"
#include <concurrent_queue.h>

namespace media
{
    enum AUDIO_STATE : uint8_t
    {
        AUDIO_STATE_PLAYING,
        AUDIO_STATE_IDLE
    };

	class AudioHandler : public IXAudio2VoiceCallback
	{
    public:
        AudioHandler() = default;
        ~AudioHandler() { ClearSamplesQueue(); }

        void SetSource(IXAudio2SourceVoice* source);
        void AddSample(XAUDIO2_BUFFER& buf);
        void ClearSamplesQueue();
        void Play();

        //Called when the voice has just finished playing a contiguous audio stream.
        void __declspec(nothrow) OnStreamEnd() 
        {
            //SetEvent(hBufferEndEvent);
        }

        //Unused methods are stubs
        void __declspec(nothrow) OnVoiceProcessingPassEnd() { }
        void __declspec(nothrow) OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
        void __declspec(nothrow) OnBufferEnd(void* pBufferContext);
        void __declspec(nothrow) OnBufferStart(void* pBufferContext) {    }
        void __declspec(nothrow) OnLoopEnd(void* pBufferContext) {    }
        void __declspec(nothrow) OnVoiceError(void* pBufferContext, HRESULT Error) { }
    private:
        

        Concurrency::concurrent_queue<XAUDIO2_BUFFER> m_samples{};
        XAUDIO2_BUFFER m_currentBuffer{};
        std::atomic<AUDIO_STATE> m_state = AUDIO_STATE_IDLE;
        IXAudio2SourceVoice* m_sourceVoice = nullptr;
        std::atomic<bool> m_bQueueEnd = true;
	};
}
