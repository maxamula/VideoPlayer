#include <Windows.h>
#include "pch.h"

namespace VideoPanel
{
	HINSTANCE g_hInstance;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        VideoPanel::g_hInstance = hinstDLL;
#ifdef _DEBUG
        Beep(600, 400);
#endif // _DEBUG
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}