#include "audio.h"

namespace media
{
	void AudioHandler::SetSource(IXAudio2SourceVoice* source)
	{
		ClearSamplesQueue();
		m_currentBuffer = {};
		m_state = AUDIO_STATE_IDLE;
		m_sourceVoice = source;
		m_bQueueEnd = true;
	}

	void AudioHandler::AddSample(XAUDIO2_BUFFER& buf)
	{
		if (m_bQueueEnd)
		{
			if (m_state == AUDIO_STATE_PLAYING)
			{
				m_currentBuffer = buf;
				m_sourceVoice->SubmitSourceBuffer(&m_currentBuffer);
				m_sourceVoice->Start();
			}
			else
			{
				m_samples.push(buf);
			}
			m_bQueueEnd = false;
		}
		else
		{
			m_samples.push(buf);
		}
	}

	void AudioHandler::Play()
	{
		assert(m_sourceVoice);
		if (m_state != AUDIO_STATE_PLAYING)
		{
			XAUDIO2_BUFFER buf{};
			if (m_samples.try_pop(buf))
			{
				m_currentBuffer = buf;
				m_sourceVoice->SubmitSourceBuffer(&m_currentBuffer);
				m_sourceVoice->Start();
			}
			m_state = AUDIO_STATE_PLAYING;
		}
	}

	void AudioHandler::OnBufferEnd(void* pBufferContext)
	{
		free((void*)m_currentBuffer.pAudioData);
		m_currentBuffer = {};
		if (m_state == AUDIO_STATE_PLAYING)
		{
			if (m_samples.try_pop(m_currentBuffer))
			{
				m_sourceVoice->SubmitSourceBuffer(&m_currentBuffer);
				m_sourceVoice->Start();
			}
			else
			{
				m_bQueueEnd = true;
			}
		}
	}
	void AudioHandler::ClearSamplesQueue()
	{
		XAUDIO2_BUFFER buf{};
		while (m_samples.try_pop(buf))
		{
			free((void*)buf.pAudioData);
		}
	}
}