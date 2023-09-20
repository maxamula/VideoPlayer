#pragma once
#include "pch.h"

namespace VideoPanel
{
	class Audio
	{
	public:
		static Audio& Instance()
		{
			static Audio audio;
			return audio;
		}

		inline IXAudio2* GetAudio() const { return m_audio.Get(); }
		inline IXAudio2MasteringVoice* GetMasteringVoice() const { return m_masteringVoice; }

	private:
		Audio();
		~Audio();
		ComPtr<IXAudio2> m_audio = nullptr;
		IXAudio2MasteringVoice* m_masteringVoice = nullptr;
	};
}
