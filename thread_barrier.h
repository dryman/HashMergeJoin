#ifndef THREAD_BARRIER_H
#define THREAD_BARRIER_H 1

#include <condition_variable>
#include <mutex>

class ThreadBarrier {
 public:
  explicit ThreadBarrier(unsigned int num_threads)
    : _num_threads(num_threads) {}
  ThreadBarrier(const ThreadBarrier&) = delete;
  bool wait();
 private:
  std::mutex _mutex;
  std::condition_variable _cond;
  const unsigned int _num_threads;
  unsigned int _count = 0;
  unsigned int _generation_id = 0;
};

#endif
