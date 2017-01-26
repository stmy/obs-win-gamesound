// producer-test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../ipc-audio-streaming/ipc-audio-streaming.h"
#include <thread>


int main()
{
    ias::producer producer("hello");
    printf("Producer created\n");

    printf("List of producers:\n");
    for (auto info : ias::get_registered_producers())
    {
        printf("  id:%d (pid:%d '%s')\n", info.id, info.proc_id, info.description.c_str());
    }

    ias::consumer consumer1(producer.get_id());
    ias::consumer consumer2(producer.get_id());
    printf("Consumer created\n");

    bool terminate = false;
    std::thread producer_thread([&producer, &terminate]
    {
        int i = 0;
        while (!terminate)
        {
            ias::audio_sample s;
            s.data[0] = (float)i;
            producer.push(&s, 1);
            printf("Producer: %f\n", s.data[0]);

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            i++;
        }
    });

    std::thread consumer1_thread([&consumer1, &terminate]
    {
        while (!terminate)
        {
            ias::audio_sample s;
            size_t count = consumer1.pop(&s, 1);

            if (count > 0)
            {
                printf("Consumer1: %f\n", s.data[0]);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });

    std::thread consumer2_thread([&consumer2, &terminate]
    {
        while (!terminate)
        {
            ias::audio_sample s;
            size_t count = consumer2.pop(&s, 1);

            if (count > 0)
            {
                printf("Consumer2: %f\n", s.data[0]);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });

    getchar();

    terminate = true;

    consumer1_thread.join();
    consumer2_thread.join();
    producer_thread.join();

    return 0;
}

