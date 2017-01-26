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

        void push(audio_sample* samples, size_t count);
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

        size_t pop(audio_sample* samples, size_t count);
        void empty();
        int get_consumer_id();
        int get_producer_id();

    private:
        int consumer_id;
        int producer_id;
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