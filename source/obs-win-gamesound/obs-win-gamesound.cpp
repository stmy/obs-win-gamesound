#include "stdafx.h"
#include <psapi.h>
#include <obs-module.h>
#include <util/threading.h>
#include <util/platform.h>
#include <thread>
#include <mutex>
#include "../ipc-audio-streaming/ipc-audio-streaming.h"
#include "obs-win-gamesound.h"


const char* get_name(void*)
{
    return obs_module_text("Game Sound Capture");
}

speaker_layout get_speaker_layout(const ias::audio_sample_info& info)
{
    switch (info.channels)
    {
    case 1: return speaker_layout::SPEAKERS_MONO;
    case 2: return speaker_layout::SPEAKERS_STEREO;
    case 6: return speaker_layout::SPEAKERS_5POINT1_SURROUND;
    case 8: return speaker_layout::SPEAKERS_7POINT1_SURROUND;
    default: return speaker_layout::SPEAKERS_UNKNOWN;
    }
}

void consumer_proc(void* obj)
{
    auto context = (plugin_context*)obj;

    while (context->running)
    {
        context->mutex->lock();

        if (context->consumer != nullptr)
        {
            while (!context->consumer->is_empty())
            {
                // Read single packet
                ias::audio_sample_info info;
                size_t bytes_read = 0;
                size_t bytes_unread = 0;
                do
                {
                    bytes_read += context->consumer->read(
                        &info, 
                        (void*)((uintptr_t)context->buffer + bytes_read), 
                        context->buflen, &bytes_unread);

                    // Extend the buffer if is not enough length for this stream
                    if (bytes_unread > 0)
                    {
                        void* new_buffer = realloc(context->buffer, context->buflen + bytes_unread);
                        if (new_buffer == nullptr)
                        {
                            throw "Out of memory!!!";
                        }
                        context->buffer = new_buffer;
                    }
                    
                } while (bytes_unread > 0);

                // Output sound
                speaker_layout layout = get_speaker_layout(info);
                if (layout != SPEAKERS_UNKNOWN)
                {
                    obs_source_audio data = {};
                    data.data[0] = (const uint8_t*)context->buffer;
                    data.frames = (uint32_t)info.samples;
                    data.speakers = layout;
                    data.samples_per_sec = info.rate;
                    data.format = AUDIO_FORMAT_FLOAT;
                    data.timestamp = os_gettime_ns();

                    obs_source_output_audio(context->source, &data);
                }
            }
        }

        context->mutex->unlock();

        os_sleep_ms(10);
    }
}

void* create(obs_data_t *settings, obs_source_t *source)
{
    auto context = new plugin_context;
    context->mutex = new std::mutex;
    context->source = source;
    context->buflen = 441 * 8 * 4;
    context->buffer = malloc(context->buflen);
    context->running = true;
    context->thread = new std::thread(consumer_proc, context);

    update(context, settings);

    return context;
}


void destroy(void* obj)
{
    auto context = (plugin_context*)obj;

    context->running = false;
    context->thread->join();
    
    free(context->buffer);

    delete context->thread;
    delete context->consumer;
    delete context->mutex;
}

void update(void* obj, obs_data_t* settings)
{
    int target = (int)obs_data_get_int(settings, "target");
    auto context = (plugin_context*)obj;

    context->mutex->lock();

    if (context->consumer == nullptr ||
        context->consumer->get_producer_id() != target)
    {
        if (context->consumer) delete context->consumer;
        context->consumer = nullptr;

        if (target != -1)
        {
            try
            {
                context->consumer = new ias::consumer(target);
            }
            catch (...)
            {
                context->consumer = nullptr;
            }
        }
    }

    context->mutex->unlock();
}

void get_defaults(obs_data_t* settings)
{
    obs_data_set_default_int(settings, "target", -1);
}

std::string get_executable_name(int pid)
{
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (proc == nullptr) { return "???"; }

    char buffer[MAX_PATH];
    if (!GetProcessImageFileNameA(proc, buffer, MAX_PATH))
    {
        return "???";
    }

    return std::string(buffer);
}

obs_properties_t* get_properties(void*)
{
    obs_properties_t *props = obs_properties_create();

    obs_property_t *target_prop = obs_properties_add_list(props,
        "target", obs_module_text("Target"),
        OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

    for (auto info : ias::get_registered_producers())
    {
        std::string exe = get_executable_name(info.proc_id);
        std::string label = 
            "[" + std::to_string(info.id) + "] " + std::to_string(info.proc_id) + ": " + exe;
        obs_property_list_add_int(target_prop,
            label.c_str(),
            info.id);
    }

    return props;
}

void register_source()
{
    obs_source_info info = {};
    info.id = "win-game-sound";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_AUDIO | OBS_SOURCE_DO_NOT_DUPLICATE;
    info.get_name = get_name;
    info.create = create;
    info.destroy = destroy;
    info.update = update;
    info.get_defaults = get_defaults;
    info.get_properties = get_properties;

    obs_register_source(&info);
}