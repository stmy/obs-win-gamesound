// xaudio2_7-hook.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <MinHook.h>
#include <regex>
#include <xaudio2.h>
#include "xaudio2_7-hook.h"
#include "xapo-captfx.h"


COCREATEINSTANCE fnCoCreateInstance;
CREATEMASTERINGVOICE fnCreateMasteringVoice;

void hook_init()
{
    MH_Initialize();
}

//
// Hook CoCreateInstance first to get instance of IXAudio2 COM object
//

void hook_CoCreateInstance()
{
    MH_CreateHookApiEx(L"ole32.dll", "CoCreateInstance", &CoCreateInstance_Hook, (LPVOID*)&fnCoCreateInstance, NULL);
    MH_EnableHook(MH_ALL_HOOKS);
}

HRESULT STDAPICALLTYPE CoCreateInstance_Hook(REFCLSID rclsid, IUnknown* pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID* ppv)
{
    HRESULT result = fnCoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);

    if (riid == CLSID_XAudio2_7)
    {
        hook_IXAudio2_CreateMasteringVoice((IXAudio2*)*ppv);
    }

    return result;
}

//
// Hook IXAudio2::CreateMasteringVoice second
//

void hook_IXAudio2_CreateMasteringVoice(IXAudio2* xaudio2)
{
    intptr_t* vtable = *reinterpret_cast<intptr_t**>(xaudio2);
    if (MH_CreateHook((void*)vtable[IXA2_7_CreateMasteringVoice], &CreateMasteringVoice_Hook, (void**)&fnCreateMasteringVoice) == MH_OK)
    {
        if (MH_EnableHook(MH_ALL_HOOKS) == MH_OK)
        {
            return;
        }
    }

    MessageBox(NULL, L"Failed installing hook on IXAudio2::CreateMasteringVoice", L"HOOK", MB_OK);

}

HRESULT __fastcall CreateMasteringVoice_Hook(IXAudio2* _this, IXAudio2MasteringVoice** ppMasteringVoice, UINT32 InputChannels, UINT32 InputSampleRate, UINT32 Flags, UINT32 DeviceIndex, const XAUDIO2_EFFECT_CHAIN* pEffectChain)
{
    CaptureEffect* capfx;
    CaptureEffect::CreateInstance(nullptr, 0, &capfx);

    size_t memsize = sizeof(XAUDIO2_EFFECT_DESCRIPTOR) * (pEffectChain->EffectCount + 1);
    XAUDIO2_EFFECT_DESCRIPTOR* descriptors = (XAUDIO2_EFFECT_DESCRIPTOR*)malloc(memsize);
    memcpy(descriptors, pEffectChain->pEffectDescriptors, memsize - sizeof(XAUDIO2_EFFECT_DESCRIPTOR));

    // Append capturing effect into effect chain
    XAUDIO2_EFFECT_CHAIN* effect_chain = const_cast<XAUDIO2_EFFECT_CHAIN*>(pEffectChain);
    effect_chain->EffectCount += 1;
    effect_chain->pEffectDescriptors = descriptors;
    effect_chain->pEffectDescriptors[effect_chain->EffectCount - 1].InitialState = TRUE;
    effect_chain->pEffectDescriptors[effect_chain->EffectCount - 1].OutputChannels = 6;
    effect_chain->pEffectDescriptors[effect_chain->EffectCount - 1].pEffect = static_cast<IXAPO*>(capfx);

    HRESULT result = fnCreateMasteringVoice(_this, ppMasteringVoice, InputChannels, InputSampleRate, Flags, DeviceIndex, pEffectChain);

    capfx->Release();
    hook_free();

    return result;
}

void hook_free()
{
    MH_DisableHook(MH_ALL_HOOKS);
}



