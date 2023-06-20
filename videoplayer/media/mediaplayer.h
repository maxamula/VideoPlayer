#pragma once
#include "common.h"

namespace player
{
    class MediaPlayer : IMFAsyncCallback
    {
    public:
        DISABLE_MOVE_COPY(MediaPlayer);
        static MediaPlayer* CreateInstance(HWND hRenderTarget, HWND hApplication);

        // IUnknown
        HRESULT QueryInterface(REFIID iid, void** ppv) override;
        ULONG AddRef() override;
        ULONG Release() override;

        // IMFAsyncCallback
        HRESULT GetParameters(DWORD*, DWORD*) override;
        HRESULT Invoke(IMFAsyncResult* pAsyncResult) override;

        void Open(const wchar_t* szwFilePath);
        void Play();
        void Pause();
        void Stop();

        bool DrawFrame();
        bool Resize(USHORT width, USHORT height);

    protected:
        ~MediaPlayer();
        MediaPlayer() = default;

        void _CreateSink(IMFStreamDescriptor* pSourceSD, HWND hRenderTarget, IMFActivate** ppActivate);

        IMFMediaSession* m_pMediaSession = nullptr;
        IMFTopology* m_Topology = nullptr;
        IMFMediaSource* m_pSource = nullptr;
        IMFVideoDisplayControl* m_pVideoDisplay = nullptr;

        HWND m_hRenderTarget = NULL;
        HWND m_hApplication = NULL;
        HANDLE m_hCloseEvent = NULL;
    private:
        unsigned int m_refs;
    };
}

