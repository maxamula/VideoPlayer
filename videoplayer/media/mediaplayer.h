#pragma once
#include "common.h"

namespace player
{
    enum MEDIA_PLAYER_STATE
    {
        MEDIA_PLAYER_STATE_CLOSED,
        MEDIA_PLAYER_STATE_IDLE,
        MEDIA_PLAYER_STATE_PLAYING,
        MEDIA_PLAYER_STATE_PAUSED,
        MEDIA_PLAYER_STATE_STOPPED,
        MEDIA_PLAYER_STATE_CLOSING
    };

    class MediaPlayer : IMFAsyncCallback
    {
    public:
        DISABLE_MOVE_COPY(MediaPlayer);
        static MediaPlayer* CreateInstance(HWND hRenderTarget);

        // IUnknown
        HRESULT QueryInterface(REFIID iid, void** ppv) override;
        ULONG AddRef() override;
        ULONG Release() override;

        // IMFAsyncCallback
        HRESULT GetParameters(DWORD*, DWORD*) override;
        HRESULT Invoke(IMFAsyncResult* pAsyncResult) override;

        inline MEDIA_PLAYER_STATE GetState() const { return m_state; };
        void HandleEvent(IMFMediaEvent* pEvent);
        void OnPaint() const;

        void Open(const wchar_t* szwFilePath);
        bool Play();
        bool Pause();
        void Stop();

        inline void Draw() { if (m_pVideoDisplay) m_pVideoDisplay->RepaintVideo(); }

        inline bool HasVideo() const { return m_pVideoDisplay != NULL; }

        void Resize(USHORT width, USHORT height);

    protected:
        ~MediaPlayer();
        MediaPlayer() = default;

        void _CreateSession();
        void _CreateTopology(IMFMediaSource* pSource, IMFPresentationDescriptor* pPD, HWND hVideoWnd, IMFTopology** ppTopology);
        void _AddBranch(IMFTopology* pTopology, IMFMediaSource* pSource, IMFPresentationDescriptor* pPresentDesc, DWORD dwIndex, HWND hRenderTarget);
        void _CreateSink(IMFStreamDescriptor* pSourceSD, HWND hRenderTarget, IMFActivate** ppActivate);
        void _Start();
        void _Close();

        IMFMediaSession* m_pMediaSession = nullptr;
        IMFMediaSource* m_pSource = nullptr;
        IMFVideoDisplayControl* m_pVideoDisplay = nullptr;

        HWND m_hRenderTarget = NULL;
        HANDLE m_hCloseEvent = NULL;

        MEDIA_PLAYER_STATE m_state = MEDIA_PLAYER_STATE_CLOSED;
    private:
        IMFTopologyNode* _CreateSourceNode(IMFTopology* pTopology, IMFPresentationDescriptor* pPresentDesc, IMFStreamDescriptor* pStreamDesc);
        IMFTopologyNode* _CreateOutNode(IMFTopology* pTopology, IMFActivate* pActivate, DWORD dwId);

        unsigned int m_refs;
    };
}

