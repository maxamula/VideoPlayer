#include "mediaplayer.h"
#include "srcresolver.h"

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
        return S_OK;
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
        HRESULT h = m_pMediaSession->Start(NULL, NULL);

        SAFE_RELEASE(pSourcePD);
        SAFE_RELEASE(pTopology);
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
}