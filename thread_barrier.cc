#include "thread_barrier.h"

bool ThreadBarrier::wait() {
  std::unique_lock<std::mutex> lock(_mutex);
  unsigned int gen_id = _generation_id;

  if (++_count < _num_threads) {
    while (gen_id == _generation_id &&
           _count < _num_threads) {
      _cond.wait(lock);
    }
    return false;
  } else {
    _count = 0;
    _generation_id++;
    lock.unlock();
    _cond.notify_all();
    return true;
  }
}
