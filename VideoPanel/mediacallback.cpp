#include "pch.h"
#include "mediacallback.h"
#include "videopanel.h"
#include <Shlwapi.h>

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
		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			return S_OK;
		}

		ComPtr<IMFMediaType> pMediaType = nullptr;
		m_reader->GetCurrentMediaType(dwStreamIndex, &pMediaType);
		GUID majorType{};
		pMediaType->GetMajorType(&majorType);

		if (majorType == MFMediaType_Video)
		{
			ComPtr<ID2D1Bitmap> bitmap = _ProcessVideoFrame(pMediaType.Get(), pSample);
			if (m_videoQueue.unsafe_size() == 0 && m_currentAudio.pos >= m_currentVideo.pos)
				m_currentVideo = { bitmap, dwStreamFlags, (uint64)llTimestamp };
			else
				m_videoQueue.push({bitmap, dwStreamFlags, (uint64)llTimestamp});
		}
		if (majorType == MFMediaType_Audio)
		{
			ComPtr<IMFMediaBuffer> buffer;
			pSample->ConvertToContiguousBuffer(&buffer);

			BYTE* audioData = nullptr;
			DWORD audioDataSize = 0;
			DWORD audioFlags = 0;
			HRESULT hr = buffer->Lock(&audioData, nullptr, &audioDataSize);
			BYTE* newBuf = (BYTE*)malloc(audioDataSize);
			memcpy(newBuf, audioData, audioDataSize);
			hr = buffer->Unlock();

			XAUDIO2_BUFFER xaudioBuf{};
			xaudioBuf.pAudioData = newBuf;
			xaudioBuf.AudioBytes = audioDataSize;

			if (m_bEmptyAudioQueue)
			{
				if (m_panel->m_state == PlayerState::Playing)
				{
					m_currentAudio = { xaudioBuf, (uint64)llTimestamp };
					m_sourceVoice->SubmitSourceBuffer(&m_currentAudio.buf);
					m_sourceVoice->Start();
				}
				else
					m_audioQueue.push({ xaudioBuf, (uint64)llTimestamp });
				m_bEmptyAudioQueue = false;
			}
			else
				m_audioQueue.push({ xaudioBuf, (uint64)llTimestamp });
		}
		if(m_videoQueue.unsafe_size() < MEDIAQUEUE_LIMIT && m_audioQueue.unsafe_size() < MEDIAQUEUE_LIMIT)
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
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
		free((void*)m_currentAudio.buf.pAudioData);
		if (m_panel->m_state == PlayerState::Playing)
		{
			if (m_audioQueue.try_pop(m_currentAudio))
			{
				m_sourceVoice->SubmitSourceBuffer(&m_currentAudio.buf);
				m_sourceVoice->Start();
			}
			else
				m_bEmptyAudioQueue = true;
		}

		// Display next video frame
		VIDEOFRAME_DATA frame{};
		while (m_currentAudio.pos >= m_currentVideo.pos)
		{
			if (m_videoQueue.try_pop(frame))
			{
				m_currentVideo = frame;
				m_position = m_currentVideo.pos;
			}	
		}

		TryQueueSample();
	}

	void MediaCallback::Open(Windows::Storage::Streams::IRandomAccessStream^ filestream)
	{
		m_reader.Reset();
		m_currentAudio = {};
		m_currentVideo = {};

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
		ThrowIfFailed(VideoPanel::s_audio->CreateSourceVoice(&m_sourceVoice, pWave, 0u, 2.0f, this));
	}

	void MediaCallback::Start()
	{
		if (m_panel->m_state == PlayerState::Playing)
		{
			AUDIOFRAME_DATA audio{};
			if (m_audioQueue.try_pop(audio))
			{
				m_currentAudio = audio;
				m_sourceVoice->SubmitSourceBuffer(&m_currentAudio.buf);
				m_sourceVoice->Start();
			}
			else
				m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		}
	}

	void MediaCallback::Goto(uint64 time)
	{
		if (time > m_duration)
			return;

		PROPVARIANT var;
		PropVariantInit(&var);
		var.vt = VT_I8;
		var.uhVal.QuadPart = time;

		ThrowIfFailed(m_reader->Flush(MF_SOURCE_READER_ALL_STREAMS));
		WaitForSingleObject(m_hFlushEvent, INFINITE);
		concurrency::critical_section::scoped_lock lock(m_mfcritsec);
		m_videoQueue.clear();
		m_audioQueue.clear();
		m_currentVideo = {};
		m_reader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
		m_position = time;
		if(m_panel->m_state == PlayerState::Playing)
			Start();
	}

	void MediaCallback::TryQueueSample()
	{
		concurrency::critical_section::scoped_lock lock(m_mfcritsec);
		if (m_videoQueue.unsafe_size() < MEDIAQUEUE_LIMIT && m_audioQueue.unsafe_size() < MEDIAQUEUE_LIMIT)
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
	}

	MediaCallback::~MediaCallback()
	{
		CloseHandle(m_hFlushEvent);
	}

	ComPtr<ID2D1Bitmap> MediaCallback::_ProcessVideoFrame(IMFMediaType* pMediaType, IMFSample* pSample)
	{
		UINT32 width = 0, height = 0;
		MFGetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, &width, &height);

		ComPtr<IMFMediaBuffer> buffer;
		pSample->ConvertToContiguousBuffer(&buffer);
		ComPtr<IMFDXGIBuffer> dxgiBuffer;
		buffer.As(&dxgiBuffer);
		BYTE* pBuffer = nullptr;
		DWORD bufferSize = 0;
		buffer->Lock(&pBuffer, nullptr, &bufferSize);

		D2D1_BITMAP_PROPERTIES bitmapProperties;
		bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
		bitmapProperties.dpiX = 96.0f;
		bitmapProperties.dpiY = 96.0f;

		ComPtr<ID2D1Bitmap> bitmap;
		m_panel->m_d2dRenderTarget->CreateBitmap(D2D1::SizeU(width, height), pBuffer, width * 4, bitmapProperties, &bitmap);
		buffer->Unlock();
		return bitmap;
	}
}