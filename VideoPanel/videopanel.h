#pragma once
#include "rendererbase.h"
#include "mediacallback.h"
#include <windows.ui.xaml.data.h>

using namespace Windows::UI::Xaml::Data;

namespace VideoPanel
{
	[Windows::UI::Xaml::Data::Bindable]
	public ref class VideoPanel sealed : public RendererBase, public INotifyPropertyChanged
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
		
		property uint32 Brightness
		{
			uint32 get();
			void set(uint32 value);
		}

		property bool KeepAspect
		{
			bool get();
			void set(bool value);
		}

		virtual event PropertyChangedEventHandler^ PropertyChanged;
	private protected:
		void _Update() override;
		virtual void _OnSizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e) override;
		void _OnPropertyChanged(Platform::String^ propertyName);

		PlayerState m_state = PlayerState::Idle;
		uint32 m_brightness = 100;
		bool m_bAspectRatio = true;
		ComPtr<MediaCallback> m_mediaCallback = nullptr;		
	};
}

