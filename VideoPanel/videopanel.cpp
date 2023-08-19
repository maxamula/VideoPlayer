#include "pch.h"
#include "videopanel.h"
#include "graphics.h"
#include "renderpass.h"
#include "audio.h"
#include <ppltasks.h>
#include <string>
#ifdef _DEBUG
#include <iostream>
#endif
using namespace Concurrency;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Storage;

using namespace Windows::System::Threading;

namespace VideoPanel
{
	void VideoPanel::Initialize()
	{
#ifdef _DEBUG
		AllocConsole();
		freopen("CONOUT$", "w", stdout);;
#endif
		MFStartup(MF_VERSION);
		GFX::InitD3D();
		Beep(200, 200);
	}

	void VideoPanel::Shutdown()
	{
		throw ref new Platform::NotImplementedException();
	}

	VideoPanel::VideoPanel()
		: m_mediaCallback(MediaCallback::GetInstance(this))
	{

	}

	void VideoPanel::Open(IRandomAccessStream^ filestream)
	{
		m_mediaCallback->Open(filestream);
		_OnPropertyChanged("Duration");
		_OnPropertyChanged("Position");
		_OnPropertyChanged("Volume");
		State = PlayerState::Playing;
	}

	void VideoPanel::_Update()
	{
		if (m_state == PlayerState::Invalid)
			return;
		//_OnPropertyChanged("Position");
		if (m_state == PlayerState::Idle)
		{
			m_surface.Present();
			return;
		}
		if (m_state == PlayerState::Paused)
		{
			m_surface.Present();
			return;
		}

		if (m_state == PlayerState::Playing)
		{
			std::shared_ptr<GFX::Texture> texture = m_mediaCallback->GetCurrentFrame();
			
			if (texture)
			{
				GFX::Render::Render(GFX::g_cmdQueue.GetCommandList(), texture->SRVAllocation().GetIndex(), m_surface.CurrentRenderTarget(), m_surface.GetViewport(), m_surface.GetScissors());
			}
			m_surface.Present();
		}
	}

	void VideoPanel::_OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
	{
		std::thread t([&](uint32_t w, uint32_t h) {
			critical_section::scoped_lock lock(m_critsec);
			critical_section::scoped_lock mflock(m_mediaCallback->GetCritSec());
			RendererBase::_ChangeSize(w, h);
		}, e->NewSize.Width, e->NewSize.Height);
		t.detach();
	}

	void VideoPanel::_OnPropertyChanged(Platform::String^ propertyName)
	{
		Windows::ApplicationModel::Core::CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
			Windows::UI::Core::CoreDispatcherPriority::Normal,
			ref new Windows::UI::Core::DispatchedHandler([this, propertyName]()
				{
					PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
				}));
	}

	/*void VideoPanel::_DrawFrame(ID2D1Bitmap* bmp)
	{
		if (bmp)
		{
			D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, m_width, m_height);
			m_d2dRenderTarget->DrawBitmap(bmp, &destRect);
		}
	}*/

	uint64 VideoPanel::Position::get()
	{
		return m_mediaCallback->GetPosition();
	}

	void VideoPanel::Position::set(uint64 value)
	{
		m_mediaCallback->Goto(value);
		_OnPropertyChanged("Position");
	}

	uint64 VideoPanel::Duration::get()
	{
		return m_mediaCallback->GetDuration();
	}

	void VideoPanel::Duration::set(uint64 value)
	{	}

	PlayerState VideoPanel::State::get()
	{
		return m_state;
	}

	void VideoPanel::State::set(PlayerState value)
	{
		if (m_state == value)
			return;
		auto prevState = m_state;
		switch (value)
		{
		case PlayerState::Idle:
			m_state = PlayerState::Idle;
			m_mediaCallback->Stop();
			break;
		case PlayerState::Playing:
			m_state = PlayerState::Playing;
			m_mediaCallback->Start();
			break;
		case PlayerState::Paused:
			m_state = PlayerState::Paused;
			m_mediaCallback->Stop();
			break;
		}
		_OnPropertyChanged("State");
	}

	float VideoPanel::Volume::get()
	{
		float volume = 0;
		if(m_mediaCallback->GetVoice())
			m_mediaCallback->GetVoice()->GetVolume(&volume);
		return volume;
	}

	void VideoPanel::Volume::set(float value)
	{
		if (value > 1.0f)
			return;
		if (m_mediaCallback->GetVoice())
		{
			m_mediaCallback->GetVoice()->SetVolume(value);
			Audio::Instance().GetAudio()->CommitChanges(XAUDIO2_COMMIT_ALL);
		}	
		_OnPropertyChanged("Volume");
	}
}
