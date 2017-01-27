#include "stdafx.h"

#include <math.h>
#include <thread>
#include <xaudio2.h>
#include <algorithm>

#include "../ipc-audio-streaming/ipc-audio-streaming.h"
#include "../xaudio2_7-hook/xapo-captfx.h"

void generate_sinewave(int freq, const WAVEFORMATEX& wfex, void* data);

int main()
{
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Create effect chain for mastering voice
    CaptureEffect* captfx;
    CaptureEffect::CreateInstance(nullptr, 0, &captfx);

    XAUDIO2_EFFECT_DESCRIPTOR fxdesc;
    fxdesc.InitialState = true;
    fxdesc.OutputChannels = 6;
    fxdesc.pEffect = static_cast<IXAPO*>(captfx);

    XAUDIO2_EFFECT_CHAIN fxchain;
    fxchain.EffectCount = 1;
    fxchain.pEffectDescriptors = &fxdesc;

    // Generate wave data
    WAVEFORMATEX wfex = { 0 };
    wfex.cbSize = sizeof(WAVEFORMATEX);
    wfex.nChannels = 8;
    wfex.nSamplesPerSec = 44100;
    wfex.wBitsPerSample = 16;
    wfex.nBlockAlign = wfex.nChannels * wfex.wBitsPerSample / 8;
    wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
    wfex.wFormatTag = WAVE_FORMAT_PCM;

    XAUDIO2_BUFFER buffer = { 0 };
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = wfex.nAvgBytesPerSec;
    buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
    buffer.pAudioData = (uint8_t*)malloc(wfex.nAvgBytesPerSec);

    generate_sinewave(500, wfex, (void*)buffer.pAudioData);

    // Create XAudio2 instances and play sound with capturing
    IXAudio2* xaudio2;
    IXAudio2MasteringVoice* mvoice;
    IXAudio2SourceVoice* voice;

    XAudio2Create(&xaudio2);
    xaudio2->CreateMasteringVoice(&mvoice, wfex.nChannels, 44100, 0, 0, &fxchain);
    xaudio2->CreateSourceVoice(&voice, &wfex);
    voice->SubmitSourceBuffer(&buffer);
    voice->Start();

    captfx->Release();

    // Create consumer
    auto producers = ias::get_registered_producers();
    auto find_result = std::find_if(producers.begin(), producers.end(), [](ias::producer_info info)
    {
        return info.proc_id == GetCurrentProcessId();
    });
    if (find_result == producers.end())
    {
        printf("Producer with proc_id = %d is not found.", GetCurrentProcessId());
        printf("Press any key to exit.");
        getchar();
        return 0;
    }

    ias::consumer consumer((*find_result).id);

    bool exit = false;
    std::thread consumer_thread([&consumer, &exit]
    {
        ias::audio_sample sample_buf[512];
        while (!exit)
        {
            ias::audio_sample_info info;
            size_t unread;
            while (consumer.read(&info, sample_buf, sizeof(sample_buf), &unread) > 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    printf("Press any key to exit.");
    getchar();

    exit = true;
    consumer_thread.join();
    voice->DestroyVoice();
    mvoice->DestroyVoice();

    return 0;
}

void generate_sinewave(int freq, const WAVEFORMATEX& wfex, void* data)
{
    short* samples = (short*)data;
    float delta = 1.0f / (float)wfex.nSamplesPerSec;
    for (int frame = 0; frame < wfex.nSamplesPerSec; frame++)
    {
        short value = (short)(sinf((2.0f * 3.1415f / (1.0f / (float)freq)) * delta * (float)frame) * (float)0x0FFF);
        for (int channel = 0; channel < wfex.nChannels; channel++)
        {
            *samples = value;
            samples++;
        }
    }
}