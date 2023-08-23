#include "pch.h"
#include "mediacallback.h"
#include "videopanel.h"
#include "audio.h"
#include "gpumem.h"
#include <future>
#include <chrono>
#include <thread>
#include <ppltasks.h>

using namespace concurrency;
using namespace Windows::System::Threading;

namespace VideoPanel
{
	MediaCallback* MediaCallback::GetInstance(VideoPanel^ videopanel)
	{
		MediaCallback* instance = new MediaCallback();
		instance->m_panel = videopanel;
		return instance;
	}

	HRESULT MediaCallback::QueryInterface(REFIID riid, void** ppv)
	{
		if (ppv == nullptr)
			return E_POINTER;
		*ppv = nullptr;
		if (riid == IID_IMFSourceReaderCallback)
		{
			*ppv = static_cast<IMFSourceReaderCallback*>(this);
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG MediaCallback::AddRef()
	{
		return InterlockedIncrement(&m_refs);
	}

	ULONG MediaCallback::Release()
	{
		ULONG uCount = InterlockedDecrement(&m_refs);
		if (uCount == 0)
		{
			delete this;
		}
		return uCount;
	}

	HRESULT MediaCallback::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		concurrency::critical_section::scoped_lock lock(m_mfcritsec);
		SAMPLE_DATA data{};
		data.pos = llTimestamp;
		data.flags = dwStreamFlags;
		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			m_mediaQueue.push(data);
			return S_OK;
		}

		if (m_seeking != uint64_invalid)
		{
			/*if (llTimestamp < m_seeking)
			{
				m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
				return S_OK;
			}
			else
			{
				m_clock->Start(m_seeking);
				m_seeking = uint64_invalid;
			}*/
			m_clock->Start(llTimestamp);
			m_seeking = uint64_invalid;
		}

		ComPtr<IMFMediaType> pMediaType = nullptr;
		m_reader->GetCurrentMediaType(dwStreamIndex, &pMediaType);
		GUID majorType{};
		pMediaType->GetMajorType(&majorType);

		

		if (majorType == MFMediaType_Video)
		{
			data.type = SAMPLE_TYPE_VIDEO;

			ComPtr<IMFMediaBuffer> buffer;
			pSample->ConvertToContiguousBuffer(&buffer);
			BYTE* pBuffer = nullptr;
			DWORD bufferSize = 0;
			buffer->Lock(&pBuffer, nullptr, &bufferSize);

			//ReverseArray(pBuffer, bufferSize);
			data.data = std::make_shared<GFX::Texture>(pBuffer, bufferSize, m_videoWidth, m_videoHeight);

			buffer->Unlock();
		}
		if (majorType == MFMediaType_Audio)
		{
			IMFMediaBuffer* buffer = nullptr;
			pSample->ConvertToContiguousBuffer(&buffer);
			if (!buffer)
				return S_OK;

			buffer->AddRef();

			BYTE* audioData = nullptr;
			DWORD audioDataSize = 0;
			DWORD audioFlags = 0;
			HRESULT hr = buffer->Lock(&audioData, nullptr, &audioDataSize);

			XAUDIO2_BUFFER xaudioBuf{};
			xaudioBuf.pAudioData = audioData;
			xaudioBuf.AudioBytes = audioDataSize;
			xaudioBuf.pContext = buffer;

			data.type = SAMPLE_TYPE_AUDIO;
			data.data = xaudioBuf;
			buffer->Release();
		}
		m_mediaQueue.push(data);

		if (m_mediaQueue.unsafe_size() < MEDIAQUEUE_LIMIT)
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		else
			m_bDeferredFrameRequest = true;
		return S_OK;
	}

	HRESULT MediaCallback::OnFlush(DWORD dwStreamIndex)
	{
		concurrency::critical_section::scoped_lock lock(m_mfcritsec);
		m_sourceVoice->FlushSourceBuffers();
		if (m_seeking != uint64_invalid)
		{
			m_clock->Stop();
			PROPVARIANT var;
			PropVariantInit(&var);
			var.vt = VT_I8;
			var.uhVal.QuadPart = m_seeking;
			m_mediaQueue.clear();
			m_reader->SetCurrentPosition(GUID_NULL, var);
			PropVariantClear(&var);
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		}
		SetEvent(m_hFlushEvent);
		return S_OK;
	}

	HRESULT MediaCallback::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return S_OK;
	}

	void MediaCallback::OnBufferEnd(void* pBufferContext)
	{
		IMFMediaBuffer* buf = reinterpret_cast<IMFMediaBuffer*>(pBufferContext);
		buf->Unlock();
		buf->Release();
	}

	void MediaCallback::Open(Windows::Storage::Streams::IRandomAccessStream^ filestream)
	{
		m_reader.Reset();
		m_current = {};
		m_mediaQueue.clear();

		ComPtr<IMFByteStream> pByteStream = nullptr;
		ThrowIfFailed(MFCreateMFByteStreamOnStreamEx((IUnknown*)filestream, &pByteStream));


		ComPtr<IMFAttributes> pAttributes = nullptr;
		ThrowIfFailed(MFCreateAttributes(&pAttributes, 1));
		ThrowIfFailed(pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this));	// !! TODO !!
		ThrowIfFailed(pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE));
		ThrowIfFailed(MFCreateSourceReaderFromByteStream(pByteStream.Get(), pAttributes.Get(), &m_reader));


		ComPtr<IMFMediaType> pNewMediaType = nullptr;
		ThrowIfFailed(MFCreateMediaType(&pNewMediaType));
		ThrowIfFailed(pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
		ThrowIfFailed(pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
		ThrowIfFailed(m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pNewMediaType.Get()));
		ThrowIfFailed(m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE));


		pNewMediaType.Reset();
		ThrowIfFailed(MFCreateMediaType(&pNewMediaType));
		ThrowIfFailed(pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
		ThrowIfFailed(pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM));
		ThrowIfFailed(m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pNewMediaType.Get()));

		ThrowIfFailed(m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));

		PROPVARIANT var;
		PropVariantInit(&var);
		ThrowIfFailed(m_reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var));
		if (var.vt == VT_UI8)
			m_duration = var.uhVal.QuadPart;
		PropVariantClear(&var);


		ComPtr<IMFMediaType> pType = nullptr;
		m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pType);
		WAVEFORMATEX* pWave = nullptr;
		UINT32 waveSize = 0;
		ThrowIfFailed(MFCreateWaveFormatExFromMFMediaType(pType.Get(), &pWave, &waveSize));
		ThrowIfFailed(Audio::Instance().GetAudio()->CreateSourceVoice(&m_sourceVoice, pWave, 0u, 2.0f, this));

		m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pType);
		MFGetAttributeSize(pType.Get(), MF_MT_FRAME_SIZE, &m_videoWidth, &m_videoHeight);

		ThrowIfFailed(MFCreatePresentationClock(&m_clock));
		ComPtr<IMFPresentationTimeSource> pTimeSource = nullptr;
		ThrowIfFailed(MFCreateSystemTimeSource(&pTimeSource));
		m_clock->SetTimeSource(pTimeSource.Get());
	}

	void MediaCallback::Start()
	{
		ThrowIfFailed(m_sourceVoice->Start());
		auto workItemHandler = ref new WorkItemHandler([this](Windows::Foundation::IAsyncAction^ action)
			{
				LONGLONG resumeTime = 0;
				ThrowIfFailed(m_clock->GetTime(&resumeTime));
				ThrowIfFailed(m_clock->Start(resumeTime));
				while (action->Status == Windows::Foundation::AsyncStatus::Started)
				{
					_ProcessMediaQueue();
				}
			});
		m_queueWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
		m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
	}

	void MediaCallback::Pause()
	{
		ThrowIfFailed(m_sourceVoice->Stop());
		if (m_queueWorker)
			m_queueWorker->Cancel();
		ThrowIfFailed(m_clock->Pause());
	}

	void MediaCallback::Stop()
	{
		ThrowIfFailed(m_sourceVoice->Stop());
		if (m_queueWorker)
			m_queueWorker->Cancel();
		ThrowIfFailed(m_clock->Stop());
	}

	void MediaCallback::Goto(uint64 time)
	{
		if (time > m_duration)
			return;
		m_seeking = time;
		_Flush();
	}

	MediaCallback::~MediaCallback()
	{
		CloseHandle(m_hFlushEvent);
		if (m_sourceVoice)
		{
			m_sourceVoice->DestroyVoice();
			m_sourceVoice = nullptr;
		}
	}

	void MediaCallback::_Flush()
	{
		m_sourceVoice->FlushSourceBuffers();
		ThrowIfFailed(m_reader->Flush(MF_SOURCE_READER_ALL_STREAMS));
		WaitForSingleObject(m_hFlushEvent, INFINITE);
		
	}

	void MediaCallback::_DeferredFrameRequest()
	{
		if (m_bDeferredFrameRequest)
		{
			m_bDeferredFrameRequest = false;
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		}
	}

	void MediaCallback::_ProcessMediaQueue() noexcept
	{
		SAMPLE_DATA sampleData;
		LONGLONG currentTime = 0;
		if (m_mediaQueue.try_pop(sampleData))	// Pop new sample
		{
			ThrowIfFailed(m_clock->GetTime(&currentTime));
			/*if (m_seeking != uint64_invalid)
			{
				if (sampleData.pos < m_seeking)
				{
					_DeferredFrameRequest();
					return;
				}
				else
					m_seeking = uint64_invalid;
			}*/

			if (currentTime < sampleData.pos)
			{
				std::chrono::nanoseconds sleepTime((sampleData.pos - currentTime) * 100);
				std::this_thread::sleep_for(sleepTime);
			}

			// Then present frame/play sound
			if (sampleData.type == SAMPLE_TYPE_VIDEO)
			{
				m_current = sampleData;
				m_displayedFrame = std::get<std::shared_ptr<GFX::Texture>>(sampleData.data);
			}
			else if (sampleData.type == SAMPLE_TYPE_AUDIO)
			{
				m_current = sampleData;
				XAUDIO2_BUFFER buffer = std::get<XAUDIO2_BUFFER>(sampleData.data);
				m_sourceVoice->SubmitSourceBuffer(&buffer);
			}

			// If last sample was processed, exit the loop
			if (sampleData.flags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				m_panel->State = PlayerState::Idle;
				return;
			}

			// If media queue was full while previous OnReadSample call, try to get sample
			_DeferredFrameRequest();
		}
		// Update video proggress ui
		m_panel->_OnPropertyChanged("Position");
	}
}