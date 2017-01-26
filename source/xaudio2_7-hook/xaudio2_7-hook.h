#pragma once

#include <xaudio2.h>

void hook_init();
void hook_CoCreateInstance();
void hook_IXAudio2_CreateMasteringVoice(IXAudio2*);
void hook_free();

typedef HRESULT(STDAPICALLTYPE *COCREATEINSTANCE)(REFCLSID, IUnknown*, DWORD, REFIID, LPVOID*);
HRESULT STDAPICALLTYPE CoCreateInstance_Hook(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv);

typedef HRESULT(__thiscall *CREATEMASTERINGVOICE)(IXAudio2*, IXAudio2MasteringVoice**, UINT32, UINT32, UINT32, UINT32, const XAUDIO2_EFFECT_CHAIN*);
HRESULT __fastcall CreateMasteringVoice_Hook(IXAudio2* _this, IXAudio2MasteringVoice** ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, UINT32 DeviceIndex, const XAUDIO2_EFFECT_CHAIN* pEffectChain);

const IID CLSID_XAudio2_7 = { 0x8bcf1f58, 0x9fe7, 0x4583,{ 0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb } };

enum ixaudio2_7_ordinals
{
    IXA2_7_QueryInterface = 0,
    IXA2_7_AddRef = 1,
    IXA2_7_Release = 2,
    IXA2_7_GetDeviceCount = 3,
    IXA2_7_GetDeviceDetails = 4,
    IXA2_7_Initialize = 5,
    IXA2_7_RegisterForCallbacks = 6,
    IXA2_7_UnregisterForCallbacks = 7,
    IXA2_7_CreateSourceVoice = 8,
    IXA2_7_CreateSubmixVoice = 9,
    IXA2_7_CreateMasteringVoice = 10,
    IXA2_7_StartEngine = 11,
    IXA2_7_StopEngine = 12,
    IXA2_7_CommitChanges = 13,
    IXA2_7_GetPerformanceData = 14,
    IXA2_7_GetDebugConfiguration = 15
};

enum ixaudio2_9_ordinals
{
    IXA2_9_QueryInterface = 0,
    IXA2_9_AddRef = 1,
    IXA2_9_Release = 2,
    IXA2_9_RegisterForCallbacks = 3,
    IXA2_9_UnregisterForCallbacks = 4,
    IXA2_9_CreateSourceVoice = 5,
    IXA2_9_CreateSubmixVoice = 6,
    IXA2_9_CreateMasteringVoice = 7,
    IXA2_9_StartEngine = 8,
    IXA2_9_StopEngine = 9,
    IXA2_9_CommitChanges = 10,
    IXA2_9_GetPerformanceData = 11,
    IXA2_9_GetDebugConfiguration = 12
};

enum ixaudio2_7_voice_ordinals
{
    IXA2_7_VOICE_GetVoiceDetails = 0,
    IXA2_7_VOICE_SetOutputVoices = 1,
    IXA2_7_VOICE_SetEffectChain = 2,
    IXA2_7_VOICE_EnableEffect = 3,
    IXA2_7_VOICE_DisableEffect = 4,
    IXA2_7_VOICE_GetEffectState = 5,
    IXA2_7_VOICE_SetEffectParameters = 6,
    IXA2_7_VOICE_GetEffectParameters = 7,
    IXA2_7_VOICE_SetFilterParameters = 8,
    IXA2_7_VOICE_GetFilterParameters = 9,
    IXA2_7_VOICE_SetOutputFilterParameters = 10,
    IXA2_7_VOICE_GetOutputFilterParameters = 11,
    IXA2_7_VOICE_SetVolume = 12,
    IXA2_7_VOICE_GetVolume = 13,
    IXA2_7_VOICE_SetChannelVolumes = 14,
    IXA2_7_VOICE_GetChannelVolumes = 15,
    IXA2_7_VOICE_SetOutputMatrix = 16,
    IXA2_7_VOICE_GetOutputMatrix = 17,
    IXA2_7_VOICE_DestroyVoice = 18
};

enum ixaudio2_9_voice_ordinals
{
    IXA2_9_VOICE_GetVoiceDetails = 0,
    IXA2_9_VOICE_SetOutputVoices = 1,
    IXA2_9_VOICE_SetEffectChain = 2,
    IXA2_9_VOICE_EnableEffect = 3,
    IXA2_9_VOICE_DisableEffect = 4,
    IXA2_9_VOICE_GetEffectState = 5,
    IXA2_9_VOICE_SetEffectParameters = 6,
    IXA2_9_VOICE_GetEffectParameters = 7,
    IXA2_9_VOICE_SetFilterParameters = 8,
    IXA2_9_VOICE_GetFilterParameters = 9,
    IXA2_9_VOICE_SetOutputFilterParameters = 10,
    IXA2_9_VOICE_GetOutputFilterParameters = 11,
    IXA2_9_VOICE_SetVolume = 12,
    IXA2_9_VOICE_GetVolume = 13,
    IXA2_9_VOICE_SetChannelVolumes = 14,
    IXA2_9_VOICE_GetChannelVolumes = 15,
    IXA2_9_VOICE_SetOutputMatrix = 16,
    IXA2_9_VOICE_GetOutputMatrix = 17,
    IXA2_9_VOICE_DestroyVoice = 18
};

