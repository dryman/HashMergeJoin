/*
 * Copyright 2018 Felix Chern
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
