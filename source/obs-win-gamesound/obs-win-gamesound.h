#pragma once

#include <thread>
#include <mutex>
#include <obs-module.h>
#include "../ipc-audio-streaming/ipc-audio-streaming.h"

struct plugin_context
{
    bool running = true;
    obs_source_t* source = nullptr;
    ias::consumer* consumer = nullptr;
    std::mutex* mutex = nullptr;
    std::thread* thread = nullptr;
    void* buffer = nullptr;
    size_t buflen = 0;
};

void register_source();

const char* get_name(void*);
void* create(obs_data_t *settings, obs_source_t *source);
void destroy(void* obj);
void update(void* obj, obs_data_t* settings);
void get_defaults(obs_data_t* settings);
obs_properties_t* get_properties(void*);

void consumer_proc(void* obj);
speaker_layout get_speaker_layout(const ias::audio_sample_info& info);
std::string get_executable_name(int pid);
