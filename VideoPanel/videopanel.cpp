#include "pch.h"
#include "videopanel.h"
#include "d3d.h"
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
		m_d2dRenderTarget->BeginDraw();
		ComPtr<ID2D1Bitmap> frame = m_mediaCallback->GetCurrentFrame();
		if (m_state == PlayerState::Idle)
		{
			m_d2dRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			m_d2dRenderTarget->EndDraw();
			m_swapChain->Present(0, 0);
			return;
		}
		if (m_state == PlayerState::Paused)
		{
			_DrawFrame(frame.Get());
			// Draw pause icon
			float pauseSize = 100.0f;
			float centerX = m_width / 2;
			float centerY = m_height / 2;
			D2D1_RECT_F pauseRect1 = D2D1::RectF(
				centerX - 20 - (20 / 2),
				centerY - 50 / 2,
				centerX - 20 + 20 / 2,
				centerY + 50 / 2
			);

			D2D1_RECT_F pauseRect2 = D2D1::RectF(
				centerX + 20 - (20 / 2),
				centerY - 50 / 2,
				centerX + 20 + 20 / 2,
				centerY + 50 / 2
			);

			ComPtr<ID2D1SolidColorBrush> pBrush;
			m_d2dRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrush);
			m_d2dRenderTarget->FillRectangle(pauseRect1, pBrush.Get());
			m_d2dRenderTarget->FillRectangle(pauseRect2, pBrush.Get());
			m_d2dRenderTarget->EndDraw();
			m_swapChain->Present(0, 0);
			return;
		}

		if (m_state == PlayerState::Playing)
		{
			_DrawFrame(frame.Get());
			m_d2dRenderTarget->EndDraw();
			m_swapChain->Present(0, 0);
		}
	}

	void VideoPanel::_OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
	{
		std::thread t([&](uint32_t w, uint32_t h) {
			critical_section::scoped_lock lock(m_critsec);
			critical_section::scoped_lock mflock(m_mediaCallback->GetCritSec());
			DirectXPanel::_ChangeSize(w, h);
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

	void VideoPanel::_DrawFrame(ID2D1Bitmap* bmp)
	{
		if (bmp)
		{
			D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, m_width, m_height);
			m_d2dRenderTarget->DrawBitmap(bmp, &destRect);
		}
	}

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
			d3d::Instance().audio->CommitChanges(XAUDIO2_COMMIT_ALL);
		}	
		_OnPropertyChanged("Volume");
	}
}
