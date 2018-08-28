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

#ifndef PARTITIONED_HASH_H
#define PARTITIONED_HASH_H 1

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <functional>
#include <sys/mman.h>
#include <memory>
#include <assert.h>
#include <atomic>
#include <thread>
#include <unordered_map>
#include "thread_barrier.h"

namespace radix_hash {

template<
  typename BidirectionalIterator,
  typename ItemType = typename BidirectionalIterator::value_type,
  typename Key = typename std::tuple_element<0,ItemType>::type,
  typename Hash = std::hash<Key>
  >
  void partitioned_only_worker(
      BidirectionalIterator begin,
                             BidirectionalIterator end,
                             std::vector<std::vector<ItemType>>* dst,
                             int thread_id,
                             int thread_num,
                             ThreadBarrier* barrier,
                             std::vector<std::size_t>* shared_counters,
                             int partitions,
                             int shift) {
  std::size_t h;
  std::size_t partition_sum;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*shared_counters)[thread_id*partitions + (h>>shift)]++;
  }

  if (barrier->wait()) {
    for (int i = 0; i < partitions; i++) {
      partition_sum = 0;
      for (int j = 0; j < thread_num; j++) {
        partition_sum += (*shared_counters)[j*partitions + i];
      }
      (*dst)[i].reserve(partition_sum);
    }
    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*dst)[h >> shift].push_back(*iter);
  }
}

template <
  typename BidirectionalIterator,
  typename ItemType = typename BidirectionalIterator::value_type,
  typename Key = typename std::tuple_element<0,ItemType>::type,
  typename Value = typename std::tuple_element<1,ItemType>::type,
  typename Hash = std::hash<Key>
  >
   void partition_only(BidirectionalIterator begin,
                       BidirectionalIterator end,
                       std::vector<std::vector<ItemType>>* dst,
                       int num_threads,
                       int partition_bits) {
  int input_num, shift, partitions, thread_partition;
  std::atomic_int a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  shift = 64 - partition_bits;
  std::vector<std::size_t> shared_counters(partitions*num_threads);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(partitioned_only_worker<BidirectionalIterator,
                             ItemType, Key, Hash>,
                             begin + i * thread_partition,
                             begin + (i+1) * thread_partition,
                             dst, i, num_threads,
                             &barrier, &shared_counters,
                             partitions, shift);
  }

  partitioned_only_worker(begin+(num_threads-1)*thread_partition,
                          end, dst, num_threads-1, num_threads,
                          &barrier, &shared_counters,
                          partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}

template<
  typename BidirectionalIterator,
  typename ItemType = typename BidirectionalIterator::value_type,
  typename Key = typename std::tuple_element<0,ItemType>::type,
  typename Value = typename std::tuple_element<1,ItemType>::type,
  typename Hash = std::hash<Key>
  >
  void partitioned_table_worker(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             std::vector<std::unordered_map<Key,Value>>* tables,
                             int thread_id,
                             int thread_num,
                             ThreadBarrier* barrier,
                             std::vector<std::size_t>* shared_counters,
                             int partitions,
                             int shift) {
  std::size_t h;
  std::size_t partition_sum;

  // TODO maybe we can make no sort version in worker as well.
  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*shared_counters)[thread_id*partitions + (h>>shift)]++;
  }

  // in barrier
  if (barrier->wait()) {
    for (int i = 0; i < partitions; i++) {
      partition_sum = 0;
      for (int j = 0; j < thread_num; j++) {
        partition_sum += (*shared_counters)[j*partitions + i];
      }
      (*tables)[i].reserve(partition_sum);
    }
    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*tables)[h >> shift].insert(*iter);
  }
}

template<
  typename BidirectionalIterator,
  typename ItemType = typename BidirectionalIterator::value_type,
  typename Key = typename std::tuple_element<0,ItemType>::type,
  typename Value = typename std::tuple_element<1,ItemType>::type,
  typename Hash = std::hash<Key>
  >
  void partitioned_hash_table(BidirectionalIterator begin,
                              BidirectionalIterator end,
                              std::vector<std::unordered_map<Key,Value>>* tables,
                              int num_threads,
                              int partition_bits) {
  int input_num, shift, partitions, thread_partition;
  std::atomic_int a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  shift = 64 - partition_bits;
  std::vector<std::size_t> shared_counters(partitions*num_threads);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(partitioned_table_worker<BidirectionalIterator,
                             ItemType, Key, Value, Hash>,
                             begin + i * thread_partition,
                             begin + (i+1) * thread_partition,
                             tables, i, num_threads,
                             &barrier, &shared_counters,
                             partitions, shift);
  }

  partitioned_table_worker(begin+(num_threads-1)*thread_partition,
                          end, tables, num_threads-1, num_threads,
                          &barrier, &shared_counters,
                          partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}


} // namespace radix_hash

#endif
