#include "pch.h"
#include "videopanel.h"
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
	ComPtr<IXAudio2> VideoPanel::s_audio = nullptr;
	IXAudio2MasteringVoice* VideoPanel::s_masteringVoice = nullptr;

	void VideoPanel::Initialize()
	{
		ThrowIfFailed(MFStartup(MF_VERSION));
		DirectXPanel::Initialize();
		ThrowIfFailed(XAudio2Create(&s_audio, 0, XAUDIO2_DEFAULT_PROCESSOR));
#ifdef _DEBUG
		XAUDIO2_DEBUG_CONFIGURATION debugConfig = { 0 };
		debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
		debugConfig.BreakMask = 0;
		debugConfig.LogThreadID = TRUE;
		debugConfig.LogFileline = TRUE;
		debugConfig.LogFunctionName = TRUE;
		debugConfig.LogTiming = TRUE;
		s_audio->SetDebugConfiguration(&debugConfig);
#endif
		ThrowIfFailed(s_audio->CreateMasteringVoice(&s_masteringVoice));
	}

	void VideoPanel::Shutdown()
	{
		ThrowIfFailed(MFShutdown());
		s_masteringVoice->DestroyVoice();
	}

	VideoPanel::VideoPanel()
		: m_mediaCallback(MediaCallback::GetInstance(this))
	{

	}

	

	void VideoPanel::Open(IRandomAccessStream^ filestream)
	{
		m_mediaCallback->Open(filestream);
		ComPtr<IMFMediaType> pType = nullptr;
		m_mediaCallback->GetReader()->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pType);
		WAVEFORMATEX* pWave = nullptr;
		UINT32 waveSize = 0;
		ThrowIfFailed(MFCreateWaveFormatExFromMFMediaType(pType.Get(), &pWave, &waveSize));
		ThrowIfFailed(s_audio->CreateSourceVoice(&m_sourceVoice, pWave, 0u, 2.0f, &m_audioHandler));	// !! TODO
		m_audioHandler.SetSource(m_sourceVoice);

		_OnPropertyChanged("Duration");
		State = PlayerState::Playing;
	}

	void VideoPanel::_Update()
	{
		if (m_state == PlayerState::Invalid)
			return;
		m_d2dRenderTarget->BeginDraw();
		ComPtr<ID2D1Bitmap> frame = m_mediaCallback->GetCurrentFrame();
		_OnPropertyChanged("Position");
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
			m_mediaCallback->TryPresent();
			_DrawFrame(frame.Get());
			m_d2dRenderTarget->EndDraw();
			m_swapChain->Present(0, 0);
			/*if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				m_state = PlayerState::Idle;
				return;
			}*/
			m_mediaCallback->TryQueueSample();
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

	void VideoPanel::_GotoPos(uint64 time)
	{
		/*PROPVARIANT var;
		PropVariantInit(&var);
		var.vt = VT_I8;
		var.hVal.QuadPart = time;
		m_reader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
		m_audioHandler.ClearSamplesQueue();
		if (m_state == PlayerState::Paused)
		{
			m_mediaCallback->TryQueueSample();
		}*/
	}


	uint64 VideoPanel::Position::get()
	{
		return m_mediaCallback->GetPosition();
	}

	void VideoPanel::Position::set(uint64 value)
	{
		
		//if (value != m_currentPos && value <= m_duration && m_reader && value)
		//{
		//	std::thread t([&]() {
		//		if (m_bProcessingSample)
		//		{
		//			m_reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
		//			//WaitForSingleObject(m_hProcessingSampleFinished, INFINITE);
		//		}
		//		critical_section::scoped_lock lock(m_critsec);
		//		critical_section::scoped_lock mflock(m_mfcritsec);
		//		//m_reader->Flush(MF_SOURCE_READER_ALL_STREAMS);
		//		_GotoPos(value);
		//		m_currentPos = value;
		//		_OnPropertyChanged("Position");
		//	});
		//	t.detach();
		//}
	}

	uint64 VideoPanel::Duration::get()
	{
		return m_mediaCallback->GetDuration();
	}

	void VideoPanel::Duration::set(uint64 value)
	{
		//m_duration = value;
		_OnPropertyChanged("Duration");
	}

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
			m_audioHandler.Play();
			break;
		case PlayerState::Paused:
			m_state = PlayerState::Paused;
			break;
		}
		_OnPropertyChanged("State");
	}
}
