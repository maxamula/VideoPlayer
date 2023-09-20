#pragma once
#include "pch.h"
#include <optional>

#define EFFECT_PIPELINE_NOCUT 0xffffffffffffffff

namespace VideoPanel::FX
{
	class VideoEffectPipeline;

	enum FX_TARGET : uint8
	{
		FX_TARGET_BOTH,
		FX_TARGET_VIDEO,
		FX_TARGET_AUDIO
	};

	public delegate void DoneCallbackDelegate(void);

	class IVideoEffect
	{
	public:
		virtual void OnProccessingBegin(VideoEffectPipeline* pipeline) noexcept = 0;
		virtual void ProcessFrame(VideoEffectPipeline* pipeline, IMFMediaType* pMediaType, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) noexcept = 0;
		virtual void OnProcessingEnd(VideoEffectPipeline* pipeline) noexcept = 0;
		virtual FX_TARGET GetFxTarget() noexcept = 0;
	};

	class VideoEffectPipeline : public IMFSourceReaderCallback
	{
	public:
		static VideoEffectPipeline* GetInstance(Windows::Storage::Streams::IRandomAccessStream^ inputStream, Windows::Storage::Streams::IRandomAccessStream^ outputStream);
		// IUnknown
		HRESULT __stdcall QueryInterface(REFIID riid, void** ppv) override;
		ULONG __stdcall AddRef() override;
		ULONG __stdcall Release() override;

		// IMFSourceReaderCallback
		HRESULT __stdcall OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample) override;
		HRESULT __stdcall OnFlush(DWORD dwStreamIndex) override;
		HRESULT __stdcall OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent) override;

		void RunAsync(DoneCallbackDelegate^ callback);
		template <typename T, typename... Args>
		void AddEffect(Args&&... args)
		{
			static_assert(std::is_base_of_v<IVideoEffect, T>, "T must derive from IVideoEffect");
			if (m_effectMutex.try_lock())
			{
				m_effects.push_back(std::make_shared<T>(std::forward<Args>(args)...));
				m_effectMutex.unlock();
			}	
		}

		template <typename T>
		void RemoveEffect()
		{
			if (m_effectMutex.try_lock())
			{
				m_effects.erase(std::remove_if(m_effects.begin(), m_effects.end(),
					[](const std::shared_ptr<IVideoEffect>& effect) {
						return dynamic_cast<T*>(effect.get()) != nullptr;
					}),
					m_effects.end());
				m_effectMutex.unlock();
			}
		}
		inline void SetScissors(uint64 start, uint64 end) { m_scissorsStart = start; m_scissorsEnd = end; }
		inline double GetFrameRate() const { return m_frameRate; }
		inline uint32 GetWidth() const { return m_width; }
		inline uint32 GetHeight() const { return m_height; }
		inline double GetFramerate() const { return m_frameRate; }
	private:
		VideoEffectPipeline() = default;
		void _Init(Windows::Storage::Streams::IRandomAccessStream^ inputStream, Windows::Storage::Streams::IRandomAccessStream^ outputStream);
		void _Finalize();

		std::vector<std::shared_ptr<IVideoEffect>> m_effects{};
		std::mutex m_effectMutex{};

		ComPtr<IMFSourceReader> m_reader = nullptr;
		ComPtr<IMFSinkWriter> m_writer = nullptr;

		DWORD m_dwDstVideoStream = 0, m_dwDstAudioStream = 0;
		std::optional<LONGLONG> m_videoZeroTimestamp{};	// Stored as ticks
		std::optional<LONGLONG> m_audioZeroTimestamp{};
		uint64 m_scissorsStart = 0;	// Stored as nanoseconds
		uint64 m_scissorsEnd = EFFECT_PIPELINE_NOCUT;

		// Video params
		uint32 m_width = 0, m_height = 0;
		double m_frameRate = 0.0;

		DoneCallbackDelegate^ m_callback;

		long m_refs;
	};	

	public ref class VideoCutter sealed
	{
	public:
		VideoCutter(Windows::Storage::Streams::IRandomAccessStream^ inputStream, Windows::Storage::Streams::IRandomAccessStream^ outputStream, uint64 from, uint64 to)
		{
			m_pipeline = VideoEffectPipeline::GetInstance(inputStream, outputStream);
			m_pipeline->SetScissors(from, to);
		}

		void SaveCopy(DoneCallbackDelegate^ callback)
		{
			m_pipeline->RunAsync(callback);
		}

	private protected:
		VideoEffectPipeline* m_pipeline = nullptr;
	};
}

