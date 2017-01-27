#pragma once

#include <windows.h>
#include <xapobase.h>
#include "../ipc-audio-streaming/ipc-audio-streaming.h"

struct CaptureEffectParameters
{

};


class __declspec(uuid("{83D20BF3-AD5C-4EB0-B680-C345107E39DA}"))
    CaptureEffect : public CXAPOParametersBase
{
public:
    static HRESULT CreateInstance(void* pInitData, UINT32 cbInitData, CaptureEffect** ppAPO)
    {
        *ppAPO = new CaptureEffect;
        (*ppAPO)->Initialize(pInitData, cbInitData);

        return S_OK;
    }

    STDMETHOD(LockForProcess) (
        UINT32 InputLockedParameterCount,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pInputLockedParameters,
        UINT32 OutputLockedParameterCount,
        const XAPO_LOCKFORPROCESS_BUFFER_PARAMETERS* pOutputLockedParameters
        ) override
    {
        HRESULT hr = CXAPOParametersBase::LockForProcess(
            InputLockedParameterCount,
            pInputLockedParameters,
            OutputLockedParameterCount,
            pOutputLockedParameters);

        if (SUCCEEDED(hr))
        {
            if (!pInputLockedParameters) { return E_POINTER; }
            memcpy(&wave_format, pInputLockedParameters[0].pFormat, sizeof(WAVEFORMATEX));
        }

        return hr;
    }

    STDMETHOD_(void, Process) (
        UINT32 InputProcessParameterCount,
        const XAPO_PROCESS_BUFFER_PARAMETERS* pInputProcessParameters,
        UINT32 OutputProcessParameterCount,
        XAPO_PROCESS_BUFFER_PARAMETERS* pOutputProcessParameters,
        BOOL IsEnabled
        ) override
    {
        if (pInputProcessParameters[0].BufferFlags == XAPO_BUFFER_SILENT)
        {
            memset(pInputProcessParameters[0].pBuffer, 0,
                pInputProcessParameters[0].ValidFrameCount * wave_format.nChannels * sizeof(FLOAT32));

            DoProcess(
                (FLOAT32* __restrict)pInputProcessParameters[0].pBuffer,
                pInputProcessParameters[0].ValidFrameCount,
                wave_format.nChannels);

        }
        else if (pInputProcessParameters[0].BufferFlags == XAPO_BUFFER_VALID)
        {
            DoProcess(
                (FLOAT32* __restrict)pInputProcessParameters[0].pBuffer,
                pInputProcessParameters[0].ValidFrameCount,
                wave_format.nChannels);
        }

        EndProcess();
    }

protected:
    CaptureEffect()
        : CXAPOParametersBase(&reg_props, (BYTE*)parameters, sizeof(CaptureEffectParameters), FALSE),
          producer("XAudio 2.7 capturer")
    {
        ZeroMemory(parameters, sizeof(parameters));
    }

    ~CaptureEffect()
    {

    }

    const WAVEFORMATEX& WaveFormat() const { return wave_format; }

    void OnSetParameters(const void* pParams, UINT32 cbParams) override
    {

    }

private:
    CaptureEffectParameters parameters[3];
    WAVEFORMATEX wave_format;
    static XAPO_REGISTRATION_PROPERTIES reg_props;
    ias::producer producer;

    void DoProcess(FLOAT32* __restrict pData, UINT32 cFrames, UINT32 cChannels)
    {
        ias::audio_sample_info info;
        info.bits_per_sample = wave_format.wBitsPerSample;
        info.channels = cChannels;
        info.samples = cFrames;
        info.rate = wave_format.nSamplesPerSec;
        info.format = ias::audio_format::format_float32;

        producer.write(info, pData, cFrames * cChannels * sizeof(FLOAT32));
    }
};

__declspec(selectany) XAPO_REGISTRATION_PROPERTIES CaptureEffect::reg_props =
{
    __uuidof(CaptureEffect),
    L"CapturingAPO",
    L"Copyright (C) 2016- stmy",
    1,
    0,
    XAPO_FLAG_INPLACE_REQUIRED
    | XAPO_FLAG_CHANNELS_MUST_MATCH
    | XAPO_FLAG_FRAMERATE_MUST_MATCH
    | XAPO_FLAG_BITSPERSAMPLE_MUST_MATCH
    | XAPO_FLAG_BUFFERCOUNT_MUST_MATCH
    | XAPO_FLAG_INPLACE_SUPPORTED,
    1, 1, 1, 1
};
