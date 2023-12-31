#include "videosurface.h"
#include "media.h"
#include <shlwapi.h>
#include <chrono>
#include <vector>
#include <shared_mutex>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

using namespace std::chrono;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace media
{
	std::vector<std::shared_ptr<VideoSurface>> VideoSurface::s_activeSurfaces;
	std::mutex VideoSurface::s_activeSurfacesMutex;
	namespace
	{
		HRESULT CreateMediaSource(const wchar_t* szwFilePath, IMFMediaSource** ppMediaSource)
		{
			HRESULT hr = S_OK;
			ComPtr<IMFSourceResolver> pResolver = nullptr;
			hr = MFCreateSourceResolver(&pResolver);
			if (FAILED(hr))
				return hr;
			if (!ppMediaSource)
				return E_POINTER;
			MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;
			IUnknown* pSource = nullptr;
			if (pResolver->CreateObjectFromURL(szwFilePath, MF_RESOLUTION_MEDIASOURCE, NULL, &ObjectType, &pSource) != S_OK)
				return E_INVALIDARG;
			assert(ObjectType != MF_OBJECT_INVALID);
			hr = pSource->QueryInterface(IID_PPV_ARGS(ppMediaSource));
			return hr;
		}

		std::string convertToMMSS(long long timeUnits)
		{
			int minutes = timeUnits / 60000000;
			int seconds = (timeUnits % 60000000) / 1000000;
			std::string timeString = std::to_string(minutes) + ":" + std::to_string(seconds);
			return timeString;
		}
	}

	VideoSurface* VideoSurface::Create(HWND hWnd, uint16_t width, uint16_t height)
	{
		std::shared_ptr<VideoSurface> surf = std::make_shared<VideoSurface>();
		if (SUCCEEDED(D3D().CreateRenderTarget(hWnd, width, height, surf->m_swap.GetAddressOf(), surf->m_d2dRenderTarget.GetAddressOf(), surf->m_target.GetAddressOf())))
		{
			surf->m_width = width;
			surf->m_height = height;
			surf->m_overlayContext = (void*)ImGui::CreateContext();
			ImGui::SetCurrentContext((ImGuiContext*)surf->m_overlayContext);
			ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(hWnd);
			ImGui_ImplDX11_Init(D3D().Device(), D3D().Context());
			surf->m_hHaltRenderer = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
			std::lock_guard lock(s_activeSurfacesMutex);
			s_activeSurfaces.push_back(surf);
			return surf.get();
		}
		return nullptr;
	}

	HRESULT VideoSurface::QueryInterface(REFIID riid, void** ppv)
	{
		static const QITAB qit[] =
		{
			QITABENT(VideoSurface, IMFSourceReaderCallback),
			{ 0 },
		};
		return QISearch(this, qit, riid, ppv);
	}

	ULONG VideoSurface::AddRef()
	{
		return InterlockedIncrement(&m_refs);
	}

	ULONG VideoSurface::Release()
	{
		ULONG uCount = InterlockedDecrement(&m_refs);
		if (uCount == 0)
		{
			_Halt(INFINITE);
			m_sourceVoice->DestroyVoice();
			std::lock_guard lock(s_activeSurfacesMutex);
			s_activeSurfaces.erase(std::remove_if(s_activeSurfaces.begin(), s_activeSurfaces.end(), [&](std::shared_ptr<VideoSurface>& ptr) { return ptr.get() == this; }));
		}
		return uCount;
	}

	HRESULT VideoSurface::OnReadSample(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample* pSample)
	{
		if (!m_bProcessingFrame)
			return S_OK;
		if (dwStreamFlags & MF_SOURCE_READERF_ENDOFSTREAM)
		{
			m_frameMutex.lock();
			m_frameFlags = dwStreamFlags;
			m_currentPos = llTimestamp;
			m_frameMutex.unlock();
			m_bProcessingFrame = false;
			return S_OK;
		}

		ComPtr<IMFMediaType> pMediaType = nullptr;
		m_reader->GetCurrentMediaType(dwStreamIndex, &pMediaType);
		GUID majorType{};
		pMediaType->GetMajorType(&majorType);

		if (majorType == MFMediaType_Video)
		{
			UINT32 width = 0, height = 0;
			MFGetAttributeSize(pMediaType.Get(), MF_MT_FRAME_SIZE, &width, &height);

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
			m_2dTarget->CreateBitmap(D2D1::SizeU(width, height), pBuffer, width * 4, bitmapProperties, &bitmap);
			buffer->Unlock();
			m_frameMutex.lock();
			m_lastFrame = bitmap;
			m_frameFlags = dwStreamFlags;
			m_currentPos = llTimestamp;
			m_frameMutex.unlock();
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
			//hr = m_sourceVoice->SubmitSourceBuffer(&xaudioBuf);

			m_audioHandler.AddSample(xaudioBuf);

			//m_sourceVoice->Start();
			//WaitForSingleObjectEx(m_audioHandler.hBufferEndEvent, INFINITE, TRUE);
		}
		m_bProcessingFrame = false;
		return S_OK;
	}

	HRESULT VideoSurface::OnFlush(DWORD dwStreamIndex)
	{
		return S_OK;
	}

	HRESULT VideoSurface::OnEvent(DWORD dwStreamIndex, IMFMediaEvent* pEvent)
	{
		return S_OK;
	}

	LRESULT VideoSurface::HandleWin32Msg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_MOUSEMOVE)
			m_overlayDuration = 2000;
		return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}

	HRESULT VideoSurface::OpenSource(const wchar_t* szPath)
	{
		HRESULT hr = S_OK;
		m_reader.Reset();
		m_lastFrame.Reset();
		ComPtr<IMFMediaSource> pSource;
		hr = CreateMediaSource(szPath, &pSource);
		if (FAILED(hr)) return hr;
		ComPtr<IMFAttributes> pAttributes = nullptr;
		hr = MFCreateAttributes(&pAttributes, 1);
		if (FAILED(hr)) return hr;
		hr = pAttributes->SetUnknown(MF_SOURCE_READER_ASYNC_CALLBACK, this);
		if (FAILED(hr)) return hr;
		hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
		if (FAILED(hr)) return hr;
		hr = MFCreateSourceReaderFromMediaSource(pSource.Get(), pAttributes.Get(), &m_reader);
		if (FAILED(hr)) return hr;
		ComPtr<IMFMediaType> pNewMediaType = nullptr;
		hr = MFCreateMediaType(&pNewMediaType);
		if (FAILED(hr)) return hr;
		hr = pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		if (FAILED(hr)) return hr;
		hr = pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
		if (FAILED(hr)) return hr;
		hr = m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pNewMediaType.Get());
		if (FAILED(hr)) return hr;
		hr = m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);
		if (FAILED(hr)) return hr;

		pNewMediaType.Reset();
		hr = MFCreateMediaType(&pNewMediaType);
		if (FAILED(hr)) return hr;
		hr = pNewMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
		if (FAILED(hr)) return hr;
		hr = pNewMediaType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
		if (FAILED(hr)) return hr;
		hr = m_reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, NULL, pNewMediaType.Get());
		if (FAILED(hr)) return hr;

		hr = m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
		if (FAILED(hr)) return hr;
		m_reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pNewMediaType);
		WAVEFORMATEX* pWave = nullptr;
		UINT32 waveSize = 0;
		hr = MFCreateWaveFormatExFromMFMediaType(pNewMediaType.Get(), &pWave, &waveSize);
		if (FAILED(hr)) return hr;
		hr = D3D().Audio()->CreateSourceVoice(&m_sourceVoice, pWave, 0u, 2.0f, &m_audioHandler);
		if (FAILED(hr)) return hr;
		m_audioHandler.SetSource(m_sourceVoice);

		if (SUCCEEDED(hr))
		{
			PROPVARIANT var;
			PropVariantInit(&var);
			hr = m_reader->GetPresentationAttribute(MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
			if (SUCCEEDED(hr) && (var.vt == VT_UI8))
			{
				m_duration = var.uhVal.QuadPart;
			}
			PropVariantClear(&var);
			m_audioHandler.Play();
			m_state = PLAYER_STATE_PLAYING;
			m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
		}
		return hr;
	}

	HRESULT VideoSurface::Resize(uint16_t width, uint16_t height)
	{
		PLAYER_STATE prevState = m_state;
		HRESULT hr = _Halt(INFINITE);
		assert(SUCCEEDED(hr));
		m_width = width;
		m_height = height;
		m_d2dRenderTarget.Reset();
		m_target.Reset();
		hr = D3D().ResizeSwapchain(m_swap.Get(), width, height, m_d2dRenderTarget.GetAddressOf(), m_target.GetAddressOf());
		if (SUCCEEDED(hr))
			m_state = prevState;
		else
			m_state = PLAYER_STATE_INVALID;
		return hr;
	}

	void VideoSurface::Play()
	{
		if (m_state == PLAYER_STATE_PAUSED)
		{
			m_state = PLAYER_STATE_PLAYING;
			m_audioHandler.Play();
		}
			
	}

	void VideoSurface::Pause()
	{
		if (m_state == PLAYER_STATE_PLAYING)
			m_state = PLAYER_STATE_PAUSED;
	}

	void VideoSurface::Update()
	{
		if (m_state == PLAYER_STATE_INVALID)
			return;
		if (m_state == PLAYER_STATE_HALT)
		{
			if (m_bProcessingFrame)
				return;
			m_state = PLAYER_STATE_INVALID;
			SetEvent(m_hHaltRenderer);
			return;
		}
		m_2dTarget->BeginDraw();
		m_frameMutex.lock();
		ComPtr<ID2D1Bitmap> frame = m_lastFrame;
		DWORD flags = m_frameFlags;
		LONGLONG timestamp = m_currentPos;
		m_frameMutex.unlock();
		if (m_state == PLAYER_STATE_IDLE)
		{
			m_2dTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
			m_2dTarget->EndDraw();
			m_swap->Present(0, 0);
			return;
		}
		if (m_state == PLAYER_STATE_PAUSED)
		{

			_DrawVideoFrame(frame.Get());
			// Draw pause icon
			float pauseSize = 100.0f;
			float centerX = m_width / 2;
			float centerY = m_height / 2;
			D2D1_RECT_F pauseRect1 = D2D1::RectF(
				centerX - 20 - (20 / 2),
				centerY - 100 / 2,
				centerX - 20 + 20 / 2,
				centerY + 100 / 2
			);
			
			D2D1_RECT_F pauseRect2 = D2D1::RectF(
				centerX + 20 - (20 / 2),
				centerY - 100 / 2,
				centerX + 20 + 20 / 2,
				centerY + 100 / 2
			);

			ComPtr<ID2D1SolidColorBrush> pBrush;
			m_2dTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrush);
			m_2dTarget->FillRectangle(pauseRect1, pBrush.Get());
			m_2dTarget->FillRectangle(pauseRect2, pBrush.Get());
			m_2dTarget->EndDraw();
			_DrawOverlay(timestamp);
			m_swap->Present(0, 0);
			return;
		}

		if (m_state == PLAYER_STATE_PLAYING)
		{
			_DrawVideoFrame(frame.Get());
			m_2dTarget->EndDraw();
			_DrawOverlay(timestamp);
			m_swap->Present(0, 0);
			if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			{
				m_state = PLAYER_STATE_IDLE;
				return;
			}
			else if (!m_bProcessingFrame)
			{
				m_bProcessingFrame = true;
				m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
			}
		}
	}

	void VideoSurface::_Reset()
	{
		if (m_sourceVoice)
		{
			m_sourceVoice->DestroyVoice();
			m_sourceVoice = nullptr;
		}
		m_reader.Reset();
		m_bProcessingFrame = false;

		std::mutex m_frameMutex{};
		ComPtr<ID2D1Bitmap> m_lastFrame = nullptr;
		DWORD m_frameFlags = 0;
		LONGLONG m_currentPos = 0;

		LONGLONG m_duration = 0;
	}

	HRESULT VideoSurface::_Halt(DWORD timeout)
	{
		if (m_state != PLAYER_STATE_INVALID)
		{
			m_state = PLAYER_STATE_HALT;
			if (WaitForSingleObject(m_hHaltRenderer, timeout) == WAIT_TIMEOUT)
			{
				return E_FAIL;
			}
			return S_OK;
		}
		return E_UNEXPECTED;
	}

	void VideoSurface::_DrawOverlay(LONGLONG& timestamp)
	{
		if (m_overlayDuration)
		{
			D3D().SetDX11RenderTargetView(m_target.GetAddressOf());
			ImGui::SetCurrentContext((ImGuiContext*)m_overlayContext);
			ImGui_ImplDX11_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
			ImGui::SetNextWindowSize(ImVec2(m_width, 50));
			ImGui::SetNextWindowPos(ImVec2(0, m_height - 50));
			ImGui::Begin("Playback control", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);
			ImGui::Text("%s", convertToMMSS(timestamp).c_str());
			ImGui::SetNextItemWidth(ImGui::GetWindowSize().x - ImGui::GetStyle().WindowPadding.x * 2);
			if (ImGui::SliderScalar("##time", ImGuiDataType_S64, &timestamp, &PLAYBACK_START, &m_duration, ""))
			{
				if (!m_bProcessingFrame)
				{
					_GotoPos(timestamp);
					m_audioHandler.ClearSamplesQueue();
					if (m_state == PLAYER_STATE_PAUSED)
					{
						m_bProcessingFrame = true;
						m_reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, nullptr, nullptr, nullptr, nullptr);
					}
				}
			}
			ImGui::End();
			ImGui::Render();
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		}
	}

	void VideoSurface::_GotoPos(LONGLONG time)
	{
		PROPVARIANT var;
		PropVariantInit(&var);
		var.vt = VT_I8;
		var.hVal.QuadPart = time;
		m_reader->SetCurrentPosition(GUID_NULL, var);
		PropVariantClear(&var);
	}

	void VideoSurface::_DrawVideoFrame(ID2D1Bitmap* frame)
	{
		if (frame)
		{
			D2D1_RECT_F destRect = D2D1::RectF(0.0f, 0.0f, m_width, m_height);
			m_2dTarget->DrawBitmap(frame, &destRect);
		}
	}
}