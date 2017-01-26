#include "stdafx.h"
#include "ipc-audio-streaming.h"
#include <windows.h>
#include <mutex>
#include <boost/date_time.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/range/algorithm_ext/erase.hpp>

namespace bip = boost::interprocess;
namespace bco = boost::container;
namespace blf = boost::lockfree;
namespace bpt = boost::posix_time;

namespace ias
{
	const std::size_t stream_buffer_samples = 2048;
	const std::size_t stream_buffer_size = sizeof(ias::audio_sample) * stream_buffer_samples + 1024;
	const std::size_t request_buffer_size = 1024;
	const std::size_t registry_size = 65536;
	const char*       stream_shm_prefix = "{82B4CD66-ABF7-45BD-9272-3C188424843B}ipc-audio-consumer-";
	const char*       request_queue_prefix = "{82B4CD66-ABF7-45BD-9272-3C188424843B}ipc-audio-producer-";
	const char*       registry_name = "{82B4CD66-ABF7-45BD-9272-3C188424843B}ipc-audio-registry";
	const char*       registry_mutex_name = "{82B4CD66-ABF7-45BD-9272-3C188424843B}ipc-audio-registry-mutex";
	const bpt::ptime  epoch(boost::gregorian::date(1970, boost::gregorian::Jan, 1));

	struct shm_producer_info
	{
		int id;
		char description[ias::max_desc_length];
		int proc_id;
	};
	typedef bip::allocator<shm_producer_info, bip::managed_shared_memory::segment_manager> producer_info_allocator;
	typedef bip::vector<shm_producer_info, producer_info_allocator> producer_info_vector;

	typedef blf::spsc_queue<audio_sample, blf::capacity<stream_buffer_samples>> ring_buffer;
}




// -----------------------------------------------------------------------------------------------------------------
// registry
//
//

ias::registry::registry()
{
	init();
}

ias::registry::~registry()
{
	((bip::named_mutex*)mutex)->unlock();
	if (mutex) delete (bip::named_mutex*)mutex;
	if (shm) delete (bip::managed_shared_memory*)shm;
}

int ias::registry::get_unique_id()
{
	int* counter = ((bip::managed_shared_memory*)shm)->find_or_construct<int>("counter")();
	int unique_id = (*counter)++;

	// Skip invalid ID
	if (*counter == invalid_id)
		(*counter)++;

	return unique_id;
}

int ias::registry::register_producer(std::string description)
{
	try
	{
		producer_info_vector* registry = 
			((bip::managed_shared_memory*)shm)->find_or_construct<producer_info_vector>("registry")(
				((bip::managed_shared_memory*)shm)->get_segment_manager()
			);

		shm_producer_info info = { 0 };
		info.id = get_unique_id();
		memcpy(info.description, description.c_str(), min(max_desc_length, description.length()));
		info.proc_id = GetCurrentProcessId();

		registry->push_back(info);

		return info.id;
	}
	catch (...)
	{
		return invalid_id;
	}
}

bool ias::registry::unregister_producer(int id)
{
	try
	{
		producer_info_vector* registry = ((bip::managed_shared_memory*)shm)->find<producer_info_vector>("registry").first;
		if (registry)
		{
			// Remove selected producer from registry
			boost::range::remove_erase_if(*registry,
				[id](shm_producer_info entry) -> bool
			{
				return entry.id == id;
			});
		}
		return true;
	}
	catch (...)
	{
		return false;
	}
}

std::vector<ias::producer_info> ias::registry::get_registered_producers()
{
	collect_garbages();

	std::vector<ias::producer_info> result;
	try
	{
		producer_info_vector* registry = ((bip::managed_shared_memory*)shm)->find<producer_info_vector>("registry").first;
		if (registry)
		{
			for (shm_producer_info entry : *registry)
			{
				ias::producer_info info;
				info.id = entry.id;
				info.proc_id = entry.proc_id;
				info.description = std::string(entry.description);
				result.push_back(info);
			}
		}
	}
	catch (...)
	{

	}

	return result;
}

void ias::registry::init()
{
	mutex = new bip::named_mutex(bip::open_or_create, registry_mutex_name);
	const bpt::ptime timeout = bpt::microsec_clock::universal_time() + bpt::seconds(1);
	if (((bip::named_mutex*)mutex)->timed_lock(timeout))
	{
		shm = new bip::managed_shared_memory(bip::open_or_create, registry_name, registry_size);
	}
	else // Seems another thread is died without unlock the mutex...
	{
		bip::named_mutex::remove(registry_mutex_name);
		init();
	}
}

void ias::registry::collect_garbages()
{
	try
	{
		producer_info_vector* reg = ((bip::managed_shared_memory*)shm)->find<producer_info_vector>("registry").first;
		if (reg)
		{
			// Remove selected producer from registry
			boost::range::remove_erase_if(*reg,
				[](shm_producer_info entry) -> bool
			{
				if (entry.proc_id == 0) return true;

				HANDLE proc_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, entry.proc_id);
				if (proc_handle == NULL) return true;

				DWORD exit_code;
				BOOL r = GetExitCodeProcess(proc_handle, &exit_code);
				if (r == FALSE || exit_code != STILL_ACTIVE)
				{
					return true;
				}

				return false;
			});
		}
	}
	catch (...) {}
}












// -----------------------------------------------------------------------------------------------------------------
// producer
//
//

ias::producer::producer(std::string description)
	:
	description(description),
	consumers(),
	running(true)
{
	regist();
	request_proc_thread = std::thread(&producer::request_proc, this);
}

ias::producer::~producer()
{
	registry reg;
	reg.unregister_producer(id);

	{
		std::lock_guard<std::mutex> lock(consumers_mutex);
		for (auto info : consumers)
		{
			delete info.shm;
		}
	}

	running = false;
	request_proc_thread.join();
}

void ias::producer::push(audio_sample* samples, size_t count)
{
	bool remove_required = false;

	for (auto info : consumers)
	{
		try
		{
			size_t num_pushed = ((ring_buffer*)info.queue)->push(samples, count);

			if (num_pushed > 0)
			{
				info.last_consumed = (bpt::microsec_clock::universal_time() - epoch).total_milliseconds();
			}
			else
			{
				// Remove consumers which not consumes data for 10 seconds
				auto from_last_consume =
					(bpt::microsec_clock::universal_time() - epoch).total_milliseconds() - info.last_consumed;
				if (from_last_consume > 10 * 1000)
				{
					info.timed_out = true;
					remove_required = true;
				}
			}
		}
		catch (...)
		{
			info.timed_out = true;
			remove_required = true;
		}
	}

	if (remove_required)
	{
		std::lock_guard<std::mutex> lock(consumers_mutex);
		boost::range::remove_erase_if(consumers,
			[](consumer_info info) -> bool
		{
			return info.timed_out;
		}
		);
	}
}

int ias::producer::get_id()
{
	return id;
}

void ias::producer::regist()
{
	registry reg;
	id = reg.register_producer(description);
}

void ias::producer::request_proc()
{
	int consumer_id;
	std::string rq_name = request_queue_prefix + std::to_string(id);
	bip::message_queue::remove(rq_name.c_str());
	bip::message_queue request_queue(bip::open_or_create, rq_name.c_str(), 100, 4);

	while (running)
	{
		// Wait for the requests
		bip::message_queue::size_type bytes_read = 0;
		unsigned int priority = 0;
		const bpt::ptime timeout = bpt::microsec_clock::universal_time() + bpt::millisec(100);

		if (!request_queue.timed_receive(
			&consumer_id, sizeof(int), bytes_read, priority, timeout))
		{
			continue;
		}

		// Process request
		accept_consumer(consumer_id);
	}

	bip::message_queue::remove(rq_name.c_str());
}

void ias::producer::accept_consumer(int consumer_id)
{
	// Ignore if the consumer already exists in the list
	auto iter = std::find_if(consumers.begin(), consumers.end(),
		[consumer_id](consumer_info info) -> bool
	{
		return info.id == consumer_id;
	}
	);
	if (iter != consumers.end())
	{
		return;
	}

	// Try opening audio stream
	bip::managed_shared_memory* shm = nullptr;
	ring_buffer* queue = nullptr;
	try
	{
		std::string shm_name = stream_shm_prefix + std::to_string(consumer_id);
		shm = new bip::managed_shared_memory(bip::open_only, shm_name.c_str());
		queue = shm->find_or_construct<ring_buffer>("queue")();
	}
	catch (...)
	{
		if (shm) delete shm;
		return;
	}

	// Register consumer
	{
		std::lock_guard<std::mutex> lock(consumers_mutex);

		consumer_info info;
		info.id = consumer_id;
		info.shm = shm;
		info.queue = queue;
		info.last_consumed = (bpt::microsec_clock::universal_time() - epoch).total_milliseconds();
		info.timed_out = false;

		consumers.push_back(info);
	}
}







// -----------------------------------------------------------------------------------------------------------------
// consumer
//
//


ias::consumer::consumer(int producer_id)
	: producer_id(producer_id), shm(nullptr), queue(nullptr)
{
	registry reg;

	auto producers = reg.get_registered_producers();
	auto iter = std::find_if(producers.begin(), producers.end(),
		[producer_id](ias::producer_info info) -> bool
	{
		return info.id == producer_id;
	}
	);

	if (iter == producers.end())
	{
		throw "Specified producer is not registered.";
	}

	// Create shared memory for streaming
	consumer_id = reg.get_unique_id();
	std::string shm_name = stream_shm_prefix + std::to_string(consumer_id);
	shm = new bip::managed_shared_memory(bip::open_or_create, shm_name.c_str(), stream_buffer_size);
	queue = ((bip::managed_shared_memory*)shm)->find_or_construct<ring_buffer>("queue")();

	// Submit request
	std::string rq_name = request_queue_prefix + std::to_string(producer_id);
	bip::message_queue request_queue(bip::open_only, rq_name.c_str());

	const bpt::ptime timeout = bpt::ptime(boost::posix_time::microsec_clock::universal_time()) + bpt::milliseconds(1);
	if (!request_queue.timed_send(&consumer_id, sizeof(consumer_id), 0, timeout))
	{
		delete shm;
		throw "Request timed-out.";
	}
}

ias::consumer::~consumer()
{
	if (shm) delete shm;

	std::string shm_name = stream_shm_prefix + std::to_string(consumer_id);
	bip::shared_memory_object::remove(shm_name.c_str());
}

size_t ias::consumer::pop(audio_sample* samples, size_t count)
{
	ring_buffer* q = (ring_buffer*)queue;
	return q->pop(samples, count);
}

void ias::consumer::empty()
{
	ring_buffer* q = (ring_buffer*)queue;
	q->empty();
}


int ias::consumer::get_consumer_id()
{
	return consumer_id;
}

int ias::consumer::get_producer_id()
{
	return producer_id;
}

std::vector<ias::producer_info> ias::get_registered_producers()
{
	registry reg;
	return reg.get_registered_producers();
}