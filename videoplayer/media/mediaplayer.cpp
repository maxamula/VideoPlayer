#include "mediaplayer.h"
#include "ui/mainwindow.h"
#include "srcresolver.h"
#include <evr.h>
#include <Shlwapi.h>

extern MainWindow* g_pMainWindow;

namespace player
{
    MediaPlayer* MediaPlayer::CreateInstance(HWND hRenderTarget)
    {
        assert(hRenderTarget);
        MediaPlayer* pPlayer = new MediaPlayer();
        pPlayer->m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (pPlayer->m_hCloseEvent == NULL)
            throw std::runtime_error("Failed to create event: err(" + std::to_string(GetLastError()) + ")");
        pPlayer->m_hRenderTarget = hRenderTarget;
        return pPlayer;
    }

    MediaPlayer::~MediaPlayer()
    {
        assert(m_pMediaSession == nullptr);
    }

    HRESULT MediaPlayer::QueryInterface(REFIID riid, void** ppv)
    {
        return E_NOINTERFACE;
    }

    ULONG MediaPlayer::AddRef()
    {
        return InterlockedIncrement(&m_refs);
    }

    ULONG MediaPlayer::Release()
    {
        ULONG uCount = InterlockedDecrement(&m_refs);
        if (uCount == 0)
        {
            _Shutdown();
            delete this;
        }
        return uCount;
    }

    HRESULT MediaPlayer::GetParameters(DWORD*, DWORD*)
    {
        return E_NOTIMPL;
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

    void MediaPlayer::OnPaint() const
    {
        if (m_pVideoDisplay)
            m_pVideoDisplay->RepaintVideo();
    }

    void MediaPlayer::Open(const wchar_t* szwFilePath)
    {
        IMFTopology* pTopology = NULL;
        IMFPresentationDescriptor* pSourcePD = NULL;

        _CreateSession();

        SourceResolver r;
        r.CreateMediaSource(szwFilePath, &m_pSource);

        assert(m_pSource->CreatePresentationDescriptor(&pSourcePD) == S_OK);

        _CreateTopology(m_pSource, pSourcePD, m_hRenderTarget, &pTopology);

        assert(m_pMediaSession->SetTopology(0, pTopology) == S_OK);
        m_state = MEDIA_PLAYER_STATE_IDLE;

        SAFE_RELEASE(pSourcePD);
        SAFE_RELEASE(pTopology);
    }

    bool MediaPlayer::Play()
    {
        if (m_state != MEDIA_PLAYER_STATE_PAUSED || m_pMediaSession == NULL || m_pSource == NULL)
            return false;
         _Start();
         return true;
    }

    bool MediaPlayer::Pause()
    {
        if (m_state != MEDIA_PLAYER_STATE_PLAYING || m_pMediaSession == NULL || m_pSource == NULL)
            return false;
        assert(m_pMediaSession->Pause() == S_OK);
        m_state = MEDIA_PLAYER_STATE_PAUSED;
        return true;
    }

    void MediaPlayer::Stop()
    {
        if ((m_state != MEDIA_PLAYER_STATE_PLAYING && m_state != MEDIA_PLAYER_STATE_PAUSED) || m_pMediaSession == NULL)
            return;

        assert(m_pMediaSession->Stop() == S_OK);
        m_state = MEDIA_PLAYER_STATE_STOPPED;
    }

    void MediaPlayer::Resize(USHORT width, USHORT height)
    {
        if (m_pVideoDisplay)
        {
            RECT rect = { 0, 0, width, height };
            m_pVideoDisplay->SetVideoPosition(NULL, &rect);
        }
    }

    void MediaPlayer::_CreateSession()
    {
        _Close();
        assert(MFCreateMediaSession(NULL, &m_pMediaSession) == S_OK);
        assert(m_pMediaSession->BeginGetEvent((IMFAsyncCallback*)this, NULL) == S_OK);
    }

    void MediaPlayer::_CreateTopology(IMFMediaSource* pSource, IMFPresentationDescriptor* pPresentDesc, HWND hRenderTarget, IMFTopology** ppTopology)
    {
        IMFTopology* pTopology = NULL;
        DWORD dwNumStreams = 0;

        assert(MFCreateTopology(&pTopology) == S_OK);

        assert(pPresentDesc->GetStreamDescriptorCount(&dwNumStreams) == S_OK);

        for (DWORD i = 0; i < dwNumStreams; i++)
        {
            _AddBranch(pTopology, pSource, pPresentDesc, i, hRenderTarget);
        }

        *ppTopology = pTopology;
    }

    void MediaPlayer::_AddBranch(IMFTopology* pTopology, IMFMediaSource* pSource, IMFPresentationDescriptor* pPresentDesc, DWORD dwIndex, HWND hRenderTarget)
    {
        IMFStreamDescriptor* pStreamDesc = NULL;
        IMFActivate* pSinkActivate = NULL;
        IMFTopologyNode* pSourceNode = NULL;
        IMFTopologyNode* pOutputNode = NULL;

        BOOL fSelected = FALSE;

        assert(pPresentDesc->GetStreamDescriptorByIndex(dwIndex, &fSelected, &pStreamDesc) == S_OK);
        if (fSelected)
        {
            _CreateSink(pStreamDesc, hRenderTarget, &pSinkActivate);
            pSourceNode = _CreateSourceNode(pTopology, pPresentDesc, pStreamDesc);

            pOutputNode = _CreateOutNode(pTopology, pSinkActivate, 0);

            pSourceNode->ConnectOutput(0, pOutputNode, 0);
        } 

        SAFE_RELEASE(pStreamDesc);
        SAFE_RELEASE(pSinkActivate);
        SAFE_RELEASE(pSourceNode);
        SAFE_RELEASE(pOutputNode);
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

    void MediaPlayer::_Close()
    {
        SAFE_RELEASE(m_pVideoDisplay);
        if (m_pMediaSession)
        {
            DWORD dwRes = 0;
            m_state = MEDIA_PLAYER_STATE_CLOSING;
            assert(m_pMediaSession->Close() == S_OK);
            dwRes = WaitForSingleObject(m_hCloseEvent, 5000);
            if (m_pSource)
                m_pSource->Shutdown();
            if (m_pMediaSession)
                m_pMediaSession->Shutdown();
            SAFE_RELEASE(m_pSource);
            SAFE_RELEASE(m_pMediaSession);
            m_state = MEDIA_PLAYER_STATE_CLOSED;
        }
    }

    void MediaPlayer::_Shutdown()
    {
        _Close();
        if (m_hCloseEvent)
        {
            CloseHandle(m_hCloseEvent);
            m_hCloseEvent = NULL;
        }
    }

    IMFTopologyNode* MediaPlayer::_CreateSourceNode(IMFTopology* pTopology, IMFPresentationDescriptor* pPresentDesc, IMFStreamDescriptor* pStreamDesc)
    {
        IMFTopologyNode* pNode = NULL;

        // Create the node.
        assert(MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode) == S_OK);
        assert(pNode->SetUnknown(MF_TOPONODE_SOURCE, m_pSource) == S_OK);

        assert(pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPresentDesc) == S_OK);

        assert(pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pStreamDesc) == S_OK);

        assert(pTopology->AddNode(pNode) == S_OK);
        return pNode;
    }

    IMFTopologyNode* MediaPlayer::_CreateOutNode(IMFTopology* pTopology, IMFActivate* pActivate, DWORD dwId)
    {
        IMFTopologyNode* pNode = NULL;
        assert(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode) == S_OK);
        assert(pNode->SetObject(pActivate) == S_OK);
        assert(pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId) == S_OK);
        assert(pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE) == S_OK);
        assert(pTopology->AddNode(pNode) == S_OK);
        return pNode;
    }
}