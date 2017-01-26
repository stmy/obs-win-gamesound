#include "stdafx.h"

#include <string>
#include <shlobj.h>
#include "winmm-proxy.h"

HMODULE winmm_handle;
extern "C" void* winmm_procs[181] = { 0 };
char* winmm_proc_names[] =
{
    "CloseDriver", "DefDriverProc", "DriverCallback", "DrvGetModuleHandle", "GetDriverModuleHandle",
    "OpenDriver", "PlaySound", "PlaySoundA", "PlaySoundW", "SendDriverMessage", "WOWAppExit",
    "auxGetDevCapsA", "auxGetDevCapsW", "auxGetNumDevs", "auxGetVolume", "auxOutMessage", "auxSetVolume",
    "joyConfigChanged", "joyGetDevCapsA", "joyGetDevCapsW", "joyGetNumDevs", "joyGetPos", "joyGetPosEx",
    "joyGetThreshold", "joyReleaseCapture", "joySetCapture", "joySetThreshold", "mciDriverNotify",
    "mciDriverYield", "mciExecute", "mciFreeCommandResource", "mciGetCreatorTask", "mciGetDeviceIDA",
    "mciGetDeviceIDFromElementIDA", "mciGetDeviceIDFromElementIDW", "mciGetDeviceIDW", "mciGetDriverData",
    "mciGetErrorStringA", "mciGetErrorStringW", "mciGetYieldProc", "mciLoadCommandResource",
    "mciSendCommandA", "mciSendCommandW", "mciSendStringA", "mciSendStringW", "mciSetDriverData",
    "mciSetYieldProc", "midiConnect", "midiDisconnect", "midiInAddBuffer", "midiInClose",
    "midiInGetDevCapsA", "midiInGetDevCapsW", "midiInGetErrorTextA", "midiInGetErrorTextW", "midiInGetID",
    "midiInGetNumDevs", "midiInMessage", "midiInOpen", "midiInPrepareHeader", "midiInReset", "midiInStart",
    "midiInStop", "midiInUnprepareHeader", "midiOutCacheDrumPatches", "midiOutCachePatches", "midiOutClose",
    "midiOutGetDevCapsA", "midiOutGetDevCapsW", "midiOutGetErrorTextA", "midiOutGetErrorTextW",
    "midiOutGetID", "midiOutGetNumDevs", "midiOutGetVolume", "midiOutLongMsg", "midiOutMessage",
    "midiOutOpen", "midiOutPrepareHeader", "midiOutReset", "midiOutSetVolume", "midiOutShortMsg",
    "midiOutUnprepareHeader", "midiStreamClose", "midiStreamOpen", "midiStreamOut", "midiStreamPause",
    "midiStreamPosition", "midiStreamProperty", "midiStreamRestart", "midiStreamStop", "mixerClose",
    "mixerGetControlDetailsA", "mixerGetControlDetailsW", "mixerGetDevCapsA", "mixerGetDevCapsW",
    "mixerGetID", "mixerGetLineControlsA", "mixerGetLineControlsW", "mixerGetLineInfoA", "mixerGetLineInfoW",
    "mixerGetNumDevs", "mixerMessage", "mixerOpen", "mixerSetControlDetails", "mmDrvInstall",
    "mmGetCurrentTask", "mmTaskBlock", "mmTaskCreate", "mmTaskSignal", "mmTaskYield", "mmioAdvance",
    "mmioAscend", "mmioClose", "mmioCreateChunk", "mmioDescend", "mmioFlush", "mmioGetInfo",
    "mmioInstallIOProcA", "mmioInstallIOProcW", "mmioOpenA", "mmioOpenW", "mmioRead", "mmioRenameA",
    "mmioRenameW", "mmioSeek", "mmioSendMessage", "mmioSetBuffer", "mmioSetInfo", "mmioStringToFOURCCA",
    "mmioStringToFOURCCW", "mmioWrite", "mmsystemGetVersion", "sndPlaySoundA", "sndPlaySoundW",
    "timeBeginPeriod", "timeEndPeriod", "timeGetDevCaps", "timeGetSystemTime", "timeGetTime", "timeKillEvent",
    "timeSetEvent", "waveInAddBuffer", "waveInClose", "waveInGetDevCapsA", "waveInGetDevCapsW",
    "waveInGetErrorTextA", "waveInGetErrorTextW", "waveInGetID", "waveInGetNumDevs", "waveInGetPosition",
    "waveInMessage", "waveInOpen", "waveInPrepareHeader", "waveInReset", "waveInStart", "waveInStop",
    "waveInUnprepareHeader", "waveOutBreakLoop", "waveOutClose", "waveOutGetDevCapsA", "waveOutGetDevCapsW",
    "waveOutGetErrorTextA", "waveOutGetErrorTextW", "waveOutGetID", "waveOutGetNumDevs", "waveOutGetPitch",
    "waveOutGetPlaybackRate", "waveOutGetPosition", "waveOutGetVolume", "waveOutMessage", "waveOutOpen",
    "waveOutPause", "waveOutPrepareHeader", "waveOutReset", "waveOutRestart", "waveOutSetPitch",
    "waveOutSetPlaybackRate", "waveOutSetVolume", "waveOutUnprepareHeader", "waveOutWrite", (char*)2
};

bool winmm_proxy_init()
{
    winmm_handle = LoadLibraryW(get_original_winmm().c_str());
    if (winmm_handle == nullptr)
    {
        return false;
    }
    for (int i = 0; i < 181; i++)
    {
        winmm_procs[i] = (void*)GetProcAddress(winmm_handle, winmm_proc_names[i]);
    }

    return true;
}

void winmm_proxy_free()
{
    FreeLibrary(winmm_handle);
}

std::wstring get_original_winmm()
{
    PWSTR system32 = nullptr;
    SHGetKnownFolderPath(FOLDERID_System, 0, NULL, &system32);

    std::wstring path(system32);
    path.resize(wcslen(system32));

    CoTaskMemFree(system32);

    return path + L"\\winmm.dll";
}

