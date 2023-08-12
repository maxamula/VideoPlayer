#include "pch.h"
#include "fx.h"
#include <mfidl.h>
#include <ppltasks.h>

namespace VideoPanel::FX
{
	VideoEffectPipeline* VideoEffectPipeline::GetInstance(Windows::Storage::Streams::IRandomAccessStream^ inputStream, Windows::Storage::Streams::IRandomAccessStream^ outputStream)
	{
        VideoEffectPipeline* instance = new VideoEffectPipeline();
        try 
        {
            instance->_Init(inputStream, outputStream);
            return instance;
        }
        catch (...)
        {
            return nullptr;
        }
	}

	HRESULT VideoEffectPipeline::QueryInterface(REFIID riid, void** ppv)
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

	ULONG VideoEffectPipeline::AddRef()
	{
		return InterlockedIncrement(&m_refs);
	}

	ULONG VideoEffectPipeline::Release()
	{
		ULONG uCount = InterlockedDecrement(&m_refs);
		if (uCount == 0)
		{
			delete this;
		}
		return uCount;
	}

	HRESULT VideoEffectPipeline::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		if (SUCCEEDED(hrStatus))
		{
			if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM || llTimestamp * 100 > m_scissorsEnd)
			{
				_Finalize();
				return S_OK;
			}
			else if (llTimestamp * 100 >= m_scissorsStart)
			{
				LONGLONG sampleTime;
				ThrowIfFailed(pSample->GetSampleTime(&sampleTime));
				ComPtr<IMFMediaType> pMediaType = nullptr;
				m_reader->GetCurrentMediaType(dwStreamIndex, &pMediaType);
				GUID majorType{};
				pMediaType->GetMajorType(&majorType);
				DWORD dstStreamIdx = 0;
				if (majorType == MFMediaType_Video)
				{
					if (!m_videoZeroTimestamp.has_value()) m_videoZeroTimestamp = sampleTime;
					dstStreamIdx = m_dwDstVideoStream;
				}
				else if (majorType == MFMediaType_Audio)
				{
					if (!m_audioZeroTimestamp.has_value()) m_audioZeroTimestamp = sampleTime;
					dstStreamIdx = m_dwDstAudioStream;
				}

				for (std::shared_ptr<IVideoEffect> effect : m_effects)
				{
					FX_TARGET fxTarget = effect->GetFxTarget();
					if (fxTarget == FX_TARGET_BOTH || (fxTarget == FX_TARGET_VIDEO && majorType == MFMediaType_Video) || (fxTarget == FX_TARGET_AUDIO && majorType == MFMediaType_Audio))
					{
						effect->ProcessFrame(this, pMediaType.Get(), dwStreamIndex, dwStreamFlags, llTimestamp, pSample);
					}
				}
				LONGLONG newSampleTime = majorType == MFMediaType_Video ? sampleTime - m_videoZeroTimestamp.value() : sampleTime - m_audioZeroTimestamp.value();
				ThrowIfFailed(pSample->SetSampleTime(newSampleTime));
				m_writer->WriteSample(dstStreamIdx, pSample);
			}
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		}
		return S_OK;
	}

	HRESULT VideoEffectPipeline::OnFlush(DWORD dwStreamIndex)
	{
		return S_OK;
	}

	HRESULT VideoEffectPipeline::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return S_OK;
	}

	void VideoEffectPipeline::RunAsync(DoneCallbackDelegate^ callback)
	{
		PROPVARIANT var;
		PropVariantInit(&var);
		var.vt = VT_I8;
		var.uhVal.QuadPart = m_scissorsStart / 100.0;
		m_reader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
		m_callback = callback;
		m_writer->BeginWriting();
		for (std::shared_ptr<IVideoEffect> effect : m_effects)
			effect->OnProccessingBegin(this);
		m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
	}

    void VideoEffectPipeline::_Init(Windows::Storage::Streams::IRandomAccessStream^ inputStream, Windows::Storage::Streams::IRandomAccessStream^ outputStream)
    {
        ComPtr<IMFByteStream> pByteStream = nullptr;
        ThrowIfFailed(MFCreateMFByteStreamOnStreamEx((IUnknown*)inputStream, &pByteStream));

        ComPtr<IMFAttributes> pAttributes = nullptr;
        ThrowIfFailed(MFCreateAttributes(&pAttributes, 1));
        ThrowIfFailed(pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this));
        ThrowIfFailed(pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE));
        ThrowIfFailed(MFCreateSourceReaderFromByteStream(pByteStream.Get(), pAttributes.Get(), &m_reader));


        ComPtr<IMFMediaType> pReaderVideoMediaType = nullptr;
        ThrowIfFailed(MFCreateMediaType(&pReaderVideoMediaType));
        ThrowIfFailed(pReaderVideoMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
        ThrowIfFailed(pReaderVideoMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32));
        ThrowIfFailed(m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pReaderVideoMediaType.Get()));
        ThrowIfFailed(m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE));


        ComPtr<IMFMediaType> pReaderAudioMediaType = nullptr;
        ThrowIfFailed(MFCreateMediaType(&pReaderAudioMediaType));
        ThrowIfFailed(pReaderAudioMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio));
        ThrowIfFailed(pReaderAudioMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_AAC));
        ThrowIfFailed(m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pReaderAudioMediaType.Get()));
        ThrowIfFailed(m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE));

		ThrowIfFailed(m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pReaderVideoMediaType));
		ThrowIfFailed(MFGetAttributeSize(pReaderVideoMediaType.Get(), MF_MT_FRAME_SIZE, &m_width, &m_height));
		ThrowIfFailed(m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pReaderAudioMediaType));
        ComPtr<IMFMediaType> pVideoMediaType = nullptr;
		ThrowIfFailed(m_reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &pVideoMediaType));
        ComPtr<IMFMediaType> pAudioMediaType = nullptr;
		ThrowIfFailed(m_reader->GetNativeMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &pAudioMediaType));

        // Create writter
        ThrowIfFailed(MFCreateMFByteStreamOnStreamEx((IUnknown*)outputStream, &pByteStream));
		ComPtr<IMFAttributes> pWriterAttributes = nullptr;
		ThrowIfFailed(MFCreateAttributes(&pWriterAttributes, 1));
		ThrowIfFailed(pWriterAttributes->SetGUID(MF_TRANSCODE_CONTAINERTYPE, MFTranscodeContainerType_MPEG4));
		ThrowIfFailed(pWriterAttributes->SetUINT32(MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
        ThrowIfFailed(MFCreateSinkWriterFromURL(nullptr, pByteStream.Get(), pWriterAttributes.Get(), &m_writer));

        ThrowIfFailed(m_writer->AddStream(pVideoMediaType.Get(), &m_dwDstVideoStream));
        ThrowIfFailed(m_writer->SetInputMediaType(m_dwDstVideoStream, pReaderVideoMediaType.Get(), nullptr));

		ThrowIfFailed(m_writer->AddStream(pAudioMediaType.Get(), &m_dwDstAudioStream));
		ThrowIfFailed(m_writer->SetInputMediaType(m_dwDstAudioStream, pReaderAudioMediaType.Get(), nullptr));
    }

	void VideoEffectPipeline::_Finalize()
	{
		for (std::shared_ptr<IVideoEffect> effect : m_effects)
			effect->OnProcessingEnd(this);
		ThrowIfFailed(m_writer->Finalize());
		//m_effectMutex.unlock();
		//if (m_callback) Concurrency::create_task([this]() { m_callback(); });
		m_callback();
	}
}