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

#ifndef RADIX_SORT_H
#define RADIX_SORT_H

#include <algorithm>
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
#include <mutex>
#include "thread_barrier.h"
#include "radix_hash.h"

template<typename RandomAccessIterator, typename Key>
static inline
void rs1_insertion_inner(RandomAccessIterator dst,
                         std::size_t idx,
                         std::size_t limit) {
  Key h1, h2;
  while (idx > limit) {
    h1 = std::get<0>(dst[idx]);
    h2 = std::get<0>(dst[idx-1]);
    if (h1 < h2) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    break;
  }
}

template<typename RandomAccessIterator, typename Key>
static inline
void rs1_insertion_outer(RandomAccessIterator dst,
                         std::size_t idx_begin,
                         std::size_t idx_end) {
  for (std::size_t idx = idx_begin + 1;
       idx < idx_end; idx++) {
    rs1_insertion_inner<RandomAccessIterator, Key>(dst, idx, idx_begin);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs1_helper_s(RandomAccessIterator dst,
                    const std::size_t (*super_indexes)[2],
                    int mask_bits,
                    int partition_bits) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  int partitions, sqrt_partitions, shift;
  std::size_t idx_i, idx_j, s_begin, s_end;
  int new_mask_bits, iter, idx_c;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  std::size_t counters[partitions];
  std::size_t indexes[partitions][2];

  for (int s = 0; s < partitions; s++) {
    s_begin = super_indexes[s][0];
    s_end = super_indexes[s][1];
    if (s_end - s_begin < 2)
      continue;
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < static_cast<std::size_t>(sqrt_partitions)) {
      rs1_insertion_outer<RandomAccessIterator, Key>(dst, s_begin, s_end);
      continue;
    }
    // Setup counters for counting sort.
    for (int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (std::size_t i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (int i = 0; i < partitions - 1; i++) {
      indexes[i][1] = indexes[i+1][0] = indexes[i][0] + counters[i];
    }
    indexes[partitions-1][1] = indexes[partitions-1][0]
      + counters[partitions-1];

    new_mask_bits = mask_bits - partition_bits;
    iter = 0;
    while (iter < partitions) {
      idx_i = indexes[iter][0];
      if (idx_i >= indexes[iter][1]) {
        iter++;
        continue;
      }
      h = std::get<0>(dst[idx_i]);
      idx_c = static_cast<int>((h & mask) >> shift);
      if (idx_c == iter) {
        indexes[iter][0]++;
        continue;
      }
      tmp_bucket = std::move(dst[idx_i]);
      do {
        h = std::get<0>(tmp_bucket);
        idx_c = static_cast<int>((h & mask) >> shift);
        idx_j = indexes[idx_c][0]++;
        std::swap(dst[idx_j], tmp_bucket);
      } while (idx_j > idx_i);
    }

    if (new_mask_bits <= 0) {
      continue;
    }

    // Reset indexes
    indexes[0][0] = s_begin;
    for (int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    rs1_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs1_helper_p(RandomAccessIterator dst,
                    const std::vector<std::pair<std::size_t, std::size_t>>& super_indexes,
                    int mask_bits,
                    int partition_bits,
                    std::atomic_int* super_counter) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  int s_idx, partitions, sqrt_partitions, shift, new_mask_bits, iter, idx_c;
  std::size_t idx_i, idx_j, s_begin, s_end;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  std::size_t counters[partitions];
  std::size_t indexes[partitions][2];

  s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);

  // TODO change s to a atomic variable and use a boolean to determine
  // to use it or not.
  while (s_idx < partitions) {
    s_begin = super_indexes[s_idx].first;
    s_end = super_indexes[s_idx].second;
    if (s_end - s_begin < 2) {
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < static_cast<std::size_t>(sqrt_partitions)) {
      rs1_insertion_outer<RandomAccessIterator, Key>(dst, s_begin, s_end);
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    // Setup counters for counting sort.
    for (int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (std::size_t i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (int i = 0; i < partitions - 1; i++) {
      indexes[i][1] = indexes[i+1][0] = indexes[i][0] + counters[i];
    }
    indexes[partitions-1][1] = indexes[partitions-1][0]
      + counters[partitions-1];

    new_mask_bits = mask_bits - partition_bits;
    iter = 0;

    while (iter < partitions) {
      idx_i = indexes[iter][0];
      if (idx_i >= indexes[iter][1]) {
        iter++;
        continue;
      }
      h = std::get<0>(dst[idx_i]);
      idx_c = static_cast<int>((h & mask) >> shift);
      if (idx_c == iter) {
        indexes[iter][0]++;
        continue;
      }
      tmp_bucket = std::move(dst[idx_i]);
      do {
        h = std::get<0>(tmp_bucket);
        idx_c = (h & mask) >> shift;
        idx_j = indexes[idx_c][0]++;
        std::swap(dst[idx_j], tmp_bucket);
      } while (idx_j > idx_i);
    }

    if (new_mask_bits <= 0) {
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }

    // Reset indexes
    indexes[0][0] = s_begin;
    for (int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    rs1_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
    s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
  }
}

template<typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs1_worker(RandomAccessIterator dst,
                  std::size_t begin,
                  std::size_t end,
                  int thread_id,
                  ThreadBarrier* barrier,
                  std::vector<std::mutex>* locks,
                  std::vector<std::atomic_ullong>* shared_counters,
                  std::vector<std::pair<std::size_t, std::size_t>>* sort_indexes,
                  std::vector<std::pair<std::size_t, std::size_t>>* indexes,
                  int partitions,
                  int shift) {
  Key h;
  std::size_t local_counters[partitions];
  std::size_t idx_i, idx_j;
  std::pair<Key, Value> tmp_bucket;
  int iter, idx_c;

  for (int i = 0; i < partitions; i++)
    local_counters[i] = 0;

  for (std::size_t i = begin; i < end; ++i) {
    h = std::get<0>(dst[i]);
    local_counters[h >> shift]++;
  }
  for (int i = 0; i < partitions; i++) {
    (*shared_counters)[i].fetch_add(local_counters[i],
                                    std::memory_order_relaxed);
  }

  // in barrier
  if (barrier->wait()) {
    local_counters[0] = 0;
    (*indexes)[0].first = 0;
    for (int i = 0; i < partitions - 1; i++) {
      (*indexes)[i].second =
       (*sort_indexes)[i+1].first =
       (*sort_indexes)[i+1].second = 
       (*indexes)[i+1].first = (*indexes)[i].first +
       (*shared_counters)[i].load(std::memory_order_relaxed);
    }
    (*indexes)[partitions-1].second = (*indexes)[partitions-1].first
      + (*shared_counters)[partitions-1].load(std::memory_order_relaxed);
    barrier->wait();
  } else {
    barrier->wait();
  }

  iter = (thread_id * 17) % partitions;
  while (iter < partitions) {
    (*locks)[iter].lock();
   retry_no_lock:
    if ((*sort_indexes)[iter].first >= (*indexes)[iter].second) {
      (*locks)[iter].unlock();
      iter++;
      continue;
    }
    idx_i = (*sort_indexes)[iter].first++;

    h = std::get<0>(dst[idx_i]);
    idx_c = static_cast<int>(h >> shift);
    if (idx_c == iter) {
      idx_j = (*sort_indexes)[iter].second++;
      if (idx_i != idx_j) {
        std::swap(dst[idx_i], dst[idx_j]);
      }
      goto retry_no_lock;
    }
    tmp_bucket = std::move(dst[idx_i]);
    (*locks)[iter].unlock();
    while (true) {
      h = std::get<0>(tmp_bucket);
      idx_c = static_cast<int>(h >> shift);
      (*locks)[idx_c].lock();
      if ((*sort_indexes)[idx_c].first > (*sort_indexes)[idx_c].second) {
        // We have a blank spot to fill tmp_bucket in
        idx_j = (*sort_indexes)[idx_c].second++;
        dst[idx_j] = std::move(tmp_bucket);
        (*locks)[idx_c].unlock();
        break;
      }
      idx_j = (*sort_indexes)[idx_c].first;
      (*sort_indexes)[idx_c].first++;
      (*sort_indexes)[idx_c].second++;
      std::swap(tmp_bucket, dst[idx_j]);
      (*locks)[idx_c].unlock();
    }
  }
}

// radix_sort_1 use insertion sort when input is smaller than sqrt(p)
template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_int_inplace(RandomAccessIterator dst,
                         std::size_t input_num,
                         int num_threads,
                         int partition_bits) {
  static_assert(std::is_unsigned<Key>::value, "Key must be an unsigned arithmic type.");
  int shift, partitions, thread_partition, new_mask_bits;
  std::atomic_int a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  thread_partition = input_num / num_threads;

  shift = sizeof(Key)*8 - partition_bits;

  std::vector<std::atomic_ullong> shared_counters(partitions);
  std::vector<std::mutex> locks(partitions);
  std::vector<std::pair<std::size_t, std::size_t>> indexes(partitions);
  std::vector<std::pair<std::size_t, std::size_t>> sort_indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs1_worker<Key,Value,
                             RandomAccessIterator>,
                             dst, i * thread_partition,
                             (i+1) * thread_partition,
                             i, &barrier, &locks, &shared_counters,
                             &sort_indexes, &indexes,
                             partitions, shift);
  }

  rs1_worker<Key,Value>(dst, (num_threads-1)*thread_partition,
                        input_num, num_threads-1, &barrier, &locks,
                        &shared_counters, &sort_indexes, &indexes,
                        partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = sizeof(Key)*8 - partition_bits;

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs1_helper_p<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, &a_counter);
  }
  rs1_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_int_inplace(RandomAccessIterator dst,
                         unsigned int input_num,
                         int num_threads) {
  int partition_bits = radix_hash::optimal_partition(input_num);
  radix_int_inplace<Key,Value,RandomAccessIterator>
   (dst, input_num, num_threads, partition_bits);
}

template<typename Key,
  typename Value,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_sort_ni_worker(BidirectionalIterator begin,
                            BidirectionalIterator end,
                            RandomAccessIterator dst,
                            int thread_id,
                            int thread_num,
                            ThreadBarrier* barrier,
                            std::vector<std::size_t>* shared_counters,
                            std::vector<std::pair<std::size_t, std::size_t>>* indexes,
                            int partitions,
                            int shift) {
  Key h;
  std::size_t dst_idx, tmp_cnt;

  // TODO maybe we can make no sort version in worker as well.
  for (auto iter = begin; iter != end; ++iter) {
    h = iter->first;
    (*shared_counters)[thread_id*partitions + (h>>shift)]++;
  }

  // in barrier
  if (barrier->wait()) {
    tmp_cnt = 0;
    for (int i = 0; i < partitions; i++) {
      for (int j = 0; j < thread_num; j++) {
        tmp_cnt += (*shared_counters)[j*partitions + i];
        (*shared_counters)[j*partitions + i] =
          tmp_cnt - (*shared_counters)[j*partitions + i];
      }
    }
    (*indexes)[0].first = 0;
    for (int i = 1; i < partitions; i++) {
      (*indexes)[i-1].second = (*indexes)[i].first = (*shared_counters)[i];
    }
    (*indexes)[partitions-1].second = tmp_cnt;

    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = iter->first;
    dst_idx = (*shared_counters)[thread_id*partitions + (h>>shift)]++;
    std::get<0>(dst[dst_idx]) = h;
    std::get<1>(dst[dst_idx]) = iter->second;
  }
}

template <typename Key,
  typename Value,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_int_non_inplace(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             RandomAccessIterator dst,
                             int num_threads,
                             int partition_bits) {
  static_assert(std::is_unsigned<Key>::value, "Key must be an unsigned arithmic type.");
  int input_num, shift, partitions, thread_partition, new_mask_bits;
  std::atomic_int a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  shift = sizeof(Key)*8 - partition_bits;

  std::vector<std::size_t> shared_counters(partitions*num_threads);
  std::vector<std::pair<std::size_t, std::size_t>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread
     (radix_sort_ni_worker<Key, Value, BidirectionalIterator, RandomAccessIterator>,
      begin + i * thread_partition,
      begin + (i+1)*thread_partition,
      dst, i, num_threads,
      &barrier, &shared_counters, &indexes, partitions, shift);
  }

  radix_sort_ni_worker<Key, Value, BidirectionalIterator, RandomAccessIterator>
   (begin + (num_threads-1)*thread_partition, end,
    dst, num_threads-1, num_threads,
    &barrier, &shared_counters, &indexes, partitions, shift);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = sizeof(Key)*8 - partition_bits;

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs1_helper_p<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, &a_counter);
  }
  rs1_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}

template <typename Key,
  typename Value,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_int_non_inplace(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             RandomAccessIterator dst,
                             int num_threads) {
  int input_num, partition_bits;
  input_num = std::distance(begin, end);
  partition_bits = radix_hash::optimal_partition(input_num);
  radix_int_non_inplace<Key,Value,BidirectionalIterator,RandomAccessIterator>
   (begin, end, dst, num_threads, partition_bits);
}

#endif
