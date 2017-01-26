// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "winmm-proxy.h"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    static HMODULE module;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
        winmm_proxy_init();
        module = LoadLibraryW(L"xaudio2_7-hook.dll");
        break;
	
	case DLL_PROCESS_DETACH:
        FreeLibrary(module);
        winmm_proxy_free();
		break;
	}
	return TRUE;
}

