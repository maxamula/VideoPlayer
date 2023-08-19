#include "pch.h"
#include "audio.h"

namespace VideoPanel
{
	Audio::Audio()
	{
		XAudio2Create(&m_audio);
		m_audio->CreateMasteringVoice(&m_masteringVoice);
	}

	Audio::~Audio()
	{
		m_masteringVoice->DestroyVoice();
	}
}