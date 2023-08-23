#include "pch.h"
#include "videopanel.h"
#include "graphics.h"
#include "renderpass.h"
#include "audio.h"
#include <ppltasks.h>
#include <string>

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
		GFX::ShutdownD3D();
		MFShutdown();
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
		if (m_state == PlayerState::Idle)
		{
			GFX::Render::Clear(GFX::g_cmdQueue.GetCommandList(), m_surface.CurrentRenderTarget());
			m_surface.Present();
			return;
		}
		if (m_state == PlayerState::Paused || m_state == PlayerState::Playing)
		{
			std::shared_ptr<GFX::Texture> texture = m_mediaCallback->GetCurrentFrame();
			D3D12_VIEWPORT vp{};
			if (m_bAspectRatio)
			{
				uint32 renderWidth = m_surface.GetWidth();
				uint32 renderHeight = m_surface.GetHeight();
				uint32 frameWidth = 0, frameHeight = 0;
				m_mediaCallback->GetVideoSize(&frameWidth, &frameHeight);

				float renderAspectRatio = static_cast<float>(renderWidth) / static_cast<float>(renderHeight);
				float frameAspectRatio = static_cast<float>(frameWidth) / static_cast<float>(frameHeight);

				float newViewportWidth, newViewportHeight;
				float xOffset = 0.0f;
				float yOffset = 0.0f;

				if (renderAspectRatio > frameAspectRatio) {
					// The render surface is wider, so we need to add black bars on the sides
					newViewportHeight = static_cast<float>(renderHeight);
					newViewportWidth = newViewportHeight * frameAspectRatio;
					xOffset = (static_cast<float>(renderWidth) - newViewportWidth) * 0.5f;
				}
				else {
					// The render surface is taller, so we need to add black bars on the top and bottom
					newViewportWidth = static_cast<float>(renderWidth);
					newViewportHeight = newViewportWidth / frameAspectRatio;
					yOffset = (static_cast<float>(renderHeight) - newViewportHeight) * 0.5f;
				}

				vp.TopLeftX = xOffset;
				vp.TopLeftY = yOffset;
				vp.Width = newViewportWidth;
				vp.Height = newViewportHeight;
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
			}
			else
				vp = m_surface.GetViewport();
			if (texture)
				GFX::Render::Render(GFX::g_cmdQueue.GetCommandList(),
					texture->SRVAllocation().GetIndex(),
					m_surface.CurrentRenderTarget(),
					vp,
					m_surface.GetScissors(),
					m_brightness);
			m_surface.Present();
		}
	}

	void VideoPanel::_OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
	{
		std::thread t([&](uint32_t w, uint32_t h) {
			critical_section::scoped_lock lock(m_critsec);
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

	uint64 VideoPanel::Position::get()
	{
		return m_mediaCallback->GetPosition();
	}

	void VideoPanel::Position::set(uint64 value)
	{
		m_mediaCallback->Flush(value);
		//_OnPropertyChanged("Position");
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
			m_mediaCallback->Pause();
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

	uint32 VideoPanel::Brightness::get()
	{
		return m_brightness;
	}

	void VideoPanel::Brightness::set(uint32 value)
	{
		m_brightness = value;
		_OnPropertyChanged("Brightness");
	}

	bool VideoPanel::KeepAspect::get()
	{
		return m_bAspectRatio;
	}

	void VideoPanel::KeepAspect::set(bool value)
	{
		m_bAspectRatio = value;
		_OnPropertyChanged("KeepAspect");
	}
}
