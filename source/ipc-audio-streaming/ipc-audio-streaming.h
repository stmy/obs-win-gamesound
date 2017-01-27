#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

namespace ias
{
    const int invalid_id = -1;
    const int max_desc_length = 64;

    struct audio_sample
    {
        float data[8];
    };

    enum audio_format : int
    {
        format_float16,
        format_float32,
        format_int8,
        format_int16,
        format_int32,
        format_uint8,
        format_uint16,
        format_uint32
    };

    struct audio_sample_info
    {
        int samples;
        int channels;
        int rate;
        int bits_per_sample;
        audio_format format;
    };

    struct producer_info
    {
        int id;
        int proc_id;
        std::string description;
    };

    class producer
    {
    public:
        producer(std::string description);
        ~producer();

        void write(const audio_sample_info& info, void* samples, size_t num_bytes);
        int get_id();

    private:
        struct consumer_info
        {
            int id;
            void* shm;
            void* queue;
            int64_t last_consumed;
            bool timed_out;
        };

        std::string description;
        int id;
        bool running;
        std::mutex consumers_mutex;
        std::vector<consumer_info> consumers;
        std::thread request_proc_thread;

        void regist();
        void request_proc();
        void accept_consumer(int id);
    };


    class consumer
    {
    public:
        consumer(int producer_id);
        ~consumer();

        size_t read(audio_sample_info* info, void* buffer, size_t buflen, size_t* bytes_unread);
        bool is_empty();
        void empty();
        int get_consumer_id();
        int get_producer_id();

    private:
        int consumer_id;
        int producer_id;
        audio_sample_info current_info;
        size_t bytes_unread;
        void* shm;
        void* queue;
    };

    class registry
    {
    public:
        registry();
        ~registry();

        int get_unique_id();
        int register_producer(std::string description);
        bool unregister_producer(int id);
        std::vector<ias::producer_info> get_registered_producers();

    private:
        void* shm;
        void* mutex;

        void init();
        void collect_garbages();
    };


    std::vector<producer_info> get_registered_producers();
}