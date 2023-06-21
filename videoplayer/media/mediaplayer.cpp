#include "mediaplayer.h"
#include "ui/mainwindow.h"
#include "srcresolver.h"
#include <evr.h>

extern MainWindow* g_pMainWindow;

namespace player
{
    MediaPlayer* MediaPlayer::CreateInstance(HWND hRenderTarget, HWND hApplication)
    {
        assert(hRenderTarget && hApplication);
        MediaPlayer* pPlayer = new MediaPlayer();
        pPlayer->m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPlayer->m_hCloseEvent == NULL)
            throw std::runtime_error("Failed to create event: err(" + std::to_string(GetLastError()) + ")");
        pPlayer->m_hRenderTarget = hRenderTarget;
        pPlayer->m_hApplication = hApplication;

        return pPlayer;
    }

    MediaPlayer::~MediaPlayer()
    {
        assert(m_pMediaSession == nullptr);
    }

    HRESULT MediaPlayer::QueryInterface(REFIID riid, void** ppv)
    {
        return E_NOINTERFACE;   // TODO: implement
    }

    ULONG MediaPlayer::AddRef()
    {
        return InterlockedIncrement(&m_refs);
    }

    ULONG MediaPlayer::Release()
    {
        ULONG uCount = InterlockedDecrement(&m_refs);
        if (uCount == 0)
            delete this;
        return uCount;
    }

    HRESULT MediaPlayer::GetParameters(DWORD*, DWORD*)
    {
        return S_OK;
    }

    HRESULT MediaPlayer::Invoke(IMFAsyncResult* pAsyncResult)
    {
        MediaEventType meType = MEUnknown;
        IMFMediaEvent* pEvent = nullptr;

        assert(m_pMediaSession->EndGetEvent(pAsyncResult, &pEvent) == S_OK);
        assert(pEvent->GetType(&meType) == S_OK);
        if (meType == MESessionClosed)
            SetEvent(m_hCloseEvent);
        else
            assert(m_pMediaSession->BeginGetEvent(this, NULL) == S_OK);

        if (m_state != MEDIA_PLAYER_STATE_CLOSING)
        {
            pEvent->AddRef();
            g_pMainWindow->PostMediaEvent(pEvent, meType);
            //PostMessage(m_hApplication, WM_MEDIAEVENT, (WPARAM)pEvent, (LPARAM)meType);
        }
        SAFE_RELEASE(pEvent);
        return S_OK;
    }

    void MediaPlayer::HandleEvent(IMFMediaEvent* pEvent)
    {
        MediaEventType meType = MEUnknown;
        assert(pEvent->GetType(&meType) == S_OK);
        HRESULT hrStatus = S_OK;
        assert(pEvent->GetStatus(&hrStatus) == S_OK);
        //assert(hrStatus == S_OK);
        switch (meType)
        {
        case MESessionTopologyStatus:
        {
            UINT32 status;

            HRESULT hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status);
            if (SUCCEEDED(hr) && (status == MF_TOPOSTATUS_READY))
            {
                SAFE_RELEASE(m_pVideoDisplay);
                (void)MFGetService(m_pMediaSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_pVideoDisplay));
                _Start();
            }
        }
            break;

        case MEEndOfPresentation:
            m_state = MEDIA_PLAYER_STATE_STOPPED;
            break;

        case MENewPresentation:
            break;
        }
        SAFE_RELEASE(pEvent);
    }

    void MediaPlayer::Open(const wchar_t* szwFilePath)
    {
        IMFTopology* pTopology = nullptr;
        IMFPresentationDescriptor* pSourcePD = nullptr;
        if (m_pMediaSession)
            SAFE_RELEASE(m_pMediaSession);
        assert(MFCreateMediaSession(NULL, &m_pMediaSession) == S_OK);
        m_pMediaSession->BeginGetEvent((IMFAsyncCallback*)this, NULL);
        SourceResolver resolver;
        resolver.CreateMediaSource(szwFilePath, &m_pSource);
        m_pSource->CreatePresentationDescriptor(&pSourcePD);
        
        // Create topology
        assert(MFCreateTopology(&pTopology) == S_OK);

        DWORD dwDescriptorCount = 0;
        assert(pSourcePD->GetStreamDescriptorCount(&dwDescriptorCount) == S_OK);
        for (DWORD i = 0; i < dwDescriptorCount; i++)
        {
            IMFStreamDescriptor* pStreamDesc = nullptr;
            IMFActivate* pSinkActivate = nullptr;
            IMFTopologyNode* pSourceNode = nullptr;
            IMFTopologyNode* pDecoderNode = nullptr;
            IMFTopologyNode* pOutputNode = nullptr;

            BOOL bSelected = FALSE;
            assert(pSourcePD->GetStreamDescriptorByIndex(i, &bSelected, &pStreamDesc) == S_OK);
            _CreateSink(pStreamDesc, m_hRenderTarget, &pSinkActivate);
            
            // Source node
            assert(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode) == S_OK);
            // Attribs
            assert(pSourceNode->SetUnknown(MF_TOPONODE_SOURCE, m_pSource) == S_OK);
            assert(pSourceNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pSourcePD) == S_OK);
            assert(pSourceNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pStreamDesc) == S_OK);
            // Add to pipeline
            pTopology->AddNode(pSourceNode);

            // Decoder
            assert(MFCreateTopologyNode(MF_TOPOLOGY_TRANSFORM_NODE, &pDecoderNode) == S_OK);
            IMFActivate* pH264DecoderActivate = nullptr;
            UINT32 count = 0;
            IMFActivate** activateArray = nullptr;
            HRESULT hr = MFdecoder MFCreateDecoderEnum(MF_VIDEO_FORMAT_H264, &activateArray, &count);
            if (SUCCEEDED(hr) && count > 0)
            {
                pH264DecoderActivate = activateArray[0];
            }*/

            // Out node
            assert(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode) == S_OK);
            assert(pOutputNode->SetObject(pSinkActivate) == S_OK);
            // Attribs
            assert(pOutputNode->SetUINT32(MF_TOPONODE_STREAMID, i) == S_OK);
            assert(pOutputNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE) == S_OK);
            // Add to pipeline
            pTopology->AddNode(pOutputNode);

            pSourceNode->ConnectOutput(0, pOutputNode, 0);

            SAFE_RELEASE(pStreamDesc);
            SAFE_RELEASE(pSinkActivate);
            SAFE_RELEASE(pSourceNode);
            SAFE_RELEASE(pOutputNode);
        }

        m_pMediaSession->SetTopology(0, pTopology);

        m_state = MEDIA_PLAYER_STATE_IDLE;

        SAFE_RELEASE(pSourcePD);
        SAFE_RELEASE(pTopology);
    }

    bool MediaPlayer::Play()
    {
        if (m_state != MEDIA_PLAYER_STATE_PLAYING || m_pMediaSession == NULL || m_pSource == NULL)
            return false;
        assert(m_pMediaSession->Pause() == S_OK);
        m_state = MEDIA_PLAYER_STATE_PAUSED;
        return true;
    }

    void MediaPlayer::_CreateSink(IMFStreamDescriptor* pSourceSD, HWND hRenderTarget, IMFActivate** ppActivate)
    {
        IMFMediaTypeHandler* pHandler = NULL;
        IMFActivate* pActivate = NULL;

        assert(pSourceSD->GetMediaTypeHandler(&pHandler) == S_OK);

        GUID guidMajorType;
        assert(pHandler->GetMajorType(&guidMajorType) == S_OK);

        if (MFMediaType_Audio == guidMajorType)
            assert(MFCreateAudioRendererActivate(&pActivate) == S_OK);
        else if (MFMediaType_Video == guidMajorType)    
            assert(MFCreateVideoRendererActivate(hRenderTarget, &pActivate) == S_OK);

        *ppActivate = pActivate;

        SAFE_RELEASE(pHandler);
    }

    void MediaPlayer::_Start()
    {
        assert(m_pMediaSession != NULL);
        PROPVARIANT varStart;
        PropVariantInit(&varStart);
        if (m_pMediaSession->Start(&GUID_NULL, &varStart) == S_OK)
            m_state = MEDIA_PLAYER_STATE_PLAYING;
        PropVariantClear(&varStart);
    }
}