#pragma once
#include "directxpanel.h"
#include "mediacallback.h"
#include <windows.ui.xaml.data.h>

using namespace Windows::UI::Xaml::Data;

namespace VideoPanel
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class VideoPanel sealed : public DirectXPanel, public INotifyPropertyChanged
	{
		friend class MediaCallback;
	public:
		static void Initialize();
		static void Shutdown();

		VideoPanel();
		void Open(Windows::Storage::Streams::IRandomAccessStream^ filestream);

		property uint64 Position
		{
			uint64 get();
			void set(uint64 value);
		}

		property uint64 Duration
		{
			uint64 get();
			void set(uint64 value);
		}
		
		property float Volume
		{
			float get();
			void set(float value);
		}

		property PlayerState State
		{
			PlayerState get();
			void set(PlayerState value);
		}

		virtual event PropertyChangedEventHandler^ PropertyChanged;
	private protected:

		static ComPtr<IXAudio2> s_audio;
		static IXAudio2MasteringVoice* s_masteringVoice;

		void _Update() override;
		virtual void _OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e) override;
		void _OnPropertyChanged(Platform::String^ propertyName);

		void _DrawFrame(ID2D1Bitmap* bmp);

		PlayerState m_state = PlayerState::Idle;
		ComPtr<MediaCallback> m_mediaCallback = nullptr;		
	};
}

