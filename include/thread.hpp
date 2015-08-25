#ifndef INCLUDE_OCTTREE_THREAD_HPP
#define INCLUDE_OCTTREE_THREAD_HPP

#ifdef __GNUC__
#  include <features.h>
#  if __GNUC_PREREQ(4, 8)
#  else
#define _GLIBCXX_USE_SCHED_YIELD
#  endif
#endif


#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace OCTree {
	typedef std::recursive_mutex recursive_mutex_sync_object;
	typedef std::mutex           mutex_sync_object; 
	
	class spin_lock_sync_object {
		std::atomic<bool>                 state;
	public:
		spin_lock_sync_object() : state(false) {}
		void lock() {
			while( state.exchange(true,  std::memory_order_acquire))
				std::this_thread::yield();
				//std::this_thread::sleep_for( std::chrono::milliseconds(1) );
		}
		void unlock() {
			state.store(false,  std::memory_order_release);
		}
	};

	class empty_sync_object {
	public:
		void lock()     { return;      }
		void unlock()   { return;      }
	};
	
	template< class __Sync >
	class barrier {
	private:
		typedef __Sync           sync_object_type;
		std::atomic<int>                  counter;
		std::condition_variable_any            cv;
		std::atomic<bool>                    flag;
		sync_object_type                     sync;
	private:
		barrier(const barrier&);
	public:
		barrier() : counter(0), cv(), flag(false), sync() {}
		barrier(size_t thread_num) : counter(thread_num), cv(), flag(false), sync() {}
		void lock  () { counter++; }
		void unlock() {
			if ( --counter == 0 ) {
				cv.notify_all();
			} else { 
				std::unique_lock<sync_object_type>    barrier_lock(sync);
				while(counter != 0)
					cv.wait_for(barrier_lock, std::chrono::milliseconds(1));
			}
		}
	};
	template< class __Sync >
	class critical_section {
	private:
		typedef __Sync           sync_object_type;
		sync_object_type                     sync;
	private:
		critical_section(const critical_section&);
	public:
		critical_section() : sync() {}
		void lock  () { 
			sync.lock();
		}
		void unlock() {
			sync.unlock();
		}
	};
}
#endif //INCLUDE_OCTTREE_THREAD_HPP


