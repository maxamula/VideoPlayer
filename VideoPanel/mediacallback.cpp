#include "pch.h"
#include "mediacallback.h"
#include "videopanel.h"
#include "audio.h"
#include "gpumem.h"
#include <future>
#include <chrono>
#include <thread>
#include <ppltasks.h>
#include <iostream>

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
		if (m_panel->State != PlayerState::Playing)
			return S_OK;
		

		SAMPLE_DATA data{};
		data.pos = llTimestamp * 100;	// store in nanoseconds
		data.flags = dwStreamFlags;
		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			m_mediaQueue.push(data);
			return S_OK;
		}

		ComPtr<IMFMediaType> pMediaType = nullptr;
		m_reader->GetCurrentMediaType(dwStreamIndex, &pMediaType);
		GUID majorType{};
		pMediaType->GetMajorType(&majorType);

		if (majorType == MFMediaType_Video)
		{
			data.type = SAMPLE_TYPE_VIDEO;
			// Upload video frame to GPU
			UINT32 width = 0, height = 0;
			MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);

			ComPtr<IMFMediaBuffer> buffer;
			pSample->ConvertToContiguousBuffer(&buffer);
			BYTE* pBuffer = nullptr;
			DWORD bufferSize = 0;
			buffer->Lock(&pBuffer, nullptr, &bufferSize);

			data.data = std::make_shared<GFX::Texture>(pBuffer, bufferSize, width, height);

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
			m_duration = var.uhVal.QuadPart * 100;
		PropVariantClear(&var);


		ComPtr<IMFMediaType> pType = nullptr;
		m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pType);
		WAVEFORMATEX* pWave = nullptr;
		UINT32 waveSize = 0;
		ThrowIfFailed(MFCreateWaveFormatExFromMFMediaType(pType.Get(), &pWave, &waveSize));
		ThrowIfFailed(Audio::Instance().GetAudio()->CreateSourceVoice(&m_sourceVoice, pWave, 0u, 2.0f, this));
	}

	void MediaCallback::Start()
	{
		m_sourceVoice->Start();
		auto workItemHandler = ref new WorkItemHandler([this](Windows::Foundation::IAsyncAction^ action)
			{
				while (action->Status == Windows::Foundation::AsyncStatus::Started)
				{
					_ProcessMediaQueue();
				}
			});
		m_queueWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
		m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
	}

	void MediaCallback::Stop()
	{
		m_sourceVoice->Stop();
		if (m_queueWorker)
			m_queueWorker->Cancel();
	}

	void MediaCallback::Goto(uint64 time)
	{
		if (time > m_duration)
			return;
		uint64 timeTicks = time / 100.0f;

		PROPVARIANT var;
		PropVariantInit(&var);
		var.vt = VT_I8;
		var.uhVal.QuadPart = timeTicks;

		ThrowIfFailed(m_reader->Flush(MF_SOURCE_READER_ALL_STREAMS));
		WaitForSingleObject(m_hFlushEvent, INFINITE);
		concurrency::critical_section::scoped_lock lock(m_mfcritsec);
		
		m_mediaQueue.clear();
		m_reader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
		m_position = time;
		m_reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		m_bGoto = true;
	}

	MediaCallback::~MediaCallback()
	{
		CloseHandle(m_hFlushEvent);
	}

	void MediaCallback::QueueThreadProxy(MediaCallback* This) noexcept
	{
		This->_ProcessMediaQueue();
	}

	void MediaCallback::_ProcessMediaQueue() noexcept
	{	
		static auto start = std::chrono::high_resolution_clock::now();
		uint64 timeDiff = 0;
		SAMPLE_DATA sampleData;
		if (m_mediaQueue.try_pop(sampleData))	// Pop new sample
		{
			// Check if new sample is ahead of time
			auto end = std::chrono::high_resolution_clock::now();
			timeDiff = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
			if (sampleData.pos > m_current.pos + timeDiff && !m_bGoto)
			{
				std::chrono::nanoseconds sleepTime(17000/*sampleData.pos - (m_current.pos + timeDiff + 170000)*/);
				//std::this_thread::sleep_for(sleepTime);
			}
			else if(m_bGoto) m_bGoto = false;
			m_position = sampleData.pos;
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
				/*m_sourceVoice->Stop();
				m_panel->m_state = PlayerState::Idle;
				m_panel->_OnPropertyChanged("State");
				m_queueThread->detach();*/
				return;
			}

			// If media queue was full while previous OnReadSample call, try to get sample
			if (m_bDeferredFrameRequest)
			{
				m_bDeferredFrameRequest = false;
				m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
			}	
			start = std::chrono::high_resolution_clock::now();
		}		
	}
}