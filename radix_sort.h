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
#include "thread_barrier.h"
#include "radix_hash.h"

template<typename RandomAccessIterator, typename Key>
static inline
void rs1_insertion_inner(RandomAccessIterator dst,
                         unsigned int idx,
                         unsigned int limit) {
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
                         unsigned int idx_begin,
                         unsigned int idx_end) {
  for (unsigned int idx = idx_begin + 1;
       idx < idx_end; idx++) {
    rs1_insertion_inner<RandomAccessIterator, Key>(dst, idx, idx_begin);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs1_helper_s(RandomAccessIterator dst,
                    const unsigned int (*super_indexes)[2],
                    int mask_bits,
                    int partition_bits) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  unsigned int partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

  for (unsigned int s = 0; s < partitions; s++) {
    s_begin = super_indexes[s][0];
    s_end = super_indexes[s][1];
    if (s_end - s_begin < 2)
      continue;
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < sqrt_partitions) {
      rs1_insertion_outer<RandomAccessIterator, Key>(dst, s_begin, s_end);
      continue;
    }
    // Setup counters for counting sort.
    for (unsigned int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (unsigned int i = 0; i < partitions - 1; i++) {
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
      idx_c = (h & mask) >> shift;
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
      continue;
    }

    // Reset indexes
    indexes[0][0] = s_begin;
    for (unsigned int i = 1; i < partitions; i++) {
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
                    const std::vector<std::pair<unsigned int,
                    unsigned int>>& super_indexes,
                    int mask_bits,
                    int partition_bits,
                    std::atomic_uint* super_counter) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  unsigned int s_idx, partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

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
    if (s_end - s_begin < sqrt_partitions) {
      rs1_insertion_outer<RandomAccessIterator, Key>(dst, s_begin, s_end);
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    // Setup counters for counting sort.
    for (unsigned int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (unsigned int i = 0; i < partitions - 1; i++) {
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
      idx_c = (h & mask) >> shift;
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
    for (unsigned int i = 1; i < partitions; i++) {
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
                  unsigned int begin,
                  unsigned int end,
                  unsigned int thread_id,
                  ThreadBarrier* barrier,
                  std::vector<std::atomic_ullong>* shared_counters,
                  std::vector<std::pair<unsigned int,
                  unsigned int>>* indexes,
                  unsigned int partitions,
                  int shift) {
  Key h;
  int local_counters[partitions];
  unsigned long long old_val, new_val, unsort, sort, flag, counter_mask;
  unsigned int iter, idx_i, idx_j, idx_c;
  std::pair<Key, Value> tmp_bucket;

  for (unsigned int i = 0; i < partitions; i++)
    local_counters[i] = 0;

  for (unsigned int i = begin; i < end; ++i) {
    h = std::get<0>(dst[i]);
    local_counters[h >> shift]++;
  }
  for (unsigned int i = 0; i < partitions; i++) {
    (*shared_counters)[i].fetch_add(local_counters[i],
                                    std::memory_order_relaxed);
  }

  // in barrier
  if (barrier->wait()) {
    local_counters[0] = 0;
    (*indexes)[0].first = 0;
    for (unsigned int i = 0; i < partitions - 1; i++) {
      local_counters[i+1] =
        (*indexes)[i].second =
        (*indexes)[i+1].first = (*indexes)[i].first +
        (*shared_counters)[i].load(std::memory_order_relaxed);
    }
    (*indexes)[partitions-1].second = (*indexes)[partitions-1].first
      + (*shared_counters)[partitions-1].load(std::memory_order_relaxed);
    for (unsigned int i = 0; i < partitions; i++) {
      new_val = (((uint64_t)local_counters[i]) << 32) |
        (uint64_t)local_counters[i];
      (*shared_counters)[i].store(new_val, std::memory_order_relaxed);
    }
    barrier->wait();
  } else {
    barrier->wait();
  }

  counter_mask = (1ULL << 31) - 1;
  iter = thread_id % partitions;
  while (iter < partitions) {
    old_val = (*shared_counters)[iter].load(std::memory_order_acquire);
  outer_reload:
    unsort = old_val & counter_mask;
    flag = (old_val >> 31) & 1;
    sort = old_val >> 32;
    if (flag)
      continue;
    if (unsort >= (*indexes)[iter].second ||
        sort >= (*indexes)[iter].second) {
      iter++;
      continue;
    }
    idx_i = unsort++;
    flag = 1;
    new_val = (sort << 32) | (flag << 31) | unsort;
    if (!(*shared_counters)[iter].compare_exchange_strong
        (old_val, new_val,
         std::memory_order_acq_rel,
         std::memory_order_acq_rel)) {
      goto outer_reload;
    }
    old_val = new_val;
    h = std::get<0>(dst[idx_i]);
    idx_c = h >> shift;
    if (idx_c == iter) {
      sort++;
      flag = 0;
      new_val = (sort << 32) | unsort;
      (*shared_counters)[iter].store(new_val, std::memory_order_release);
      continue;
    }
    tmp_bucket = std::move(dst[idx_i]);
    (*shared_counters)[iter].fetch_and(~(1ULL<<31), std::memory_order_release);

    while (true) {
      h = std::get<0>(tmp_bucket);
      idx_c = h >> shift;
    inner_reload_1:
      old_val = (*shared_counters)[idx_c].load(std::memory_order_acquire);
    inner_reload_2:
      unsort = old_val & counter_mask;
      flag = (old_val >> 31) & 1;
      sort = old_val >> 32;
      if (flag)
        goto inner_reload_1;
      if (unsort > sort) {
        idx_j = sort++;
        new_val = (sort << 32) | unsort;
        if (!(*shared_counters)[idx_c].compare_exchange_strong
            (old_val, new_val,
             std::memory_order_acq_rel,
             std::memory_order_acq_rel)) {
          goto inner_reload_2;
        }
        dst[idx_j] = std::move(tmp_bucket);
        break;
      }
      assert(unsort == sort);
      idx_j = sort++;
      unsort++;
      new_val = (sort << 32) | unsort;
      if (!(*shared_counters)[idx_c].compare_exchange_strong
          (old_val, new_val,
           std::memory_order_acq_rel,
           std::memory_order_acq_rel)) {
        goto inner_reload_2;
      }
      std::swap(tmp_bucket, dst[idx_j]);
    }
  }
}

// radix_sort_1 use insertion sort when input is smaller than sqrt(p)
template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_sort_1(RandomAccessIterator dst,
                    unsigned int input_num,
                    int partition_bits,
                    int num_threads) {
  static_assert(std::is_unsigned<Key>::value, "Key must be an unsigned arithmic type.");
  int shift, partitions, thread_partition, new_mask_bits;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  thread_partition = input_num / num_threads;

  shift = sizeof(Key)*8 - partition_bits;

  std::vector<std::atomic_ullong> shared_counters(partitions);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs1_worker<Key,Value,
                             RandomAccessIterator>,
                             dst, i * thread_partition,
                             (i+1) * thread_partition,
                             i, &barrier, &shared_counters, &indexes,
                             partitions, shift);
  }

  rs1_worker<Key,Value>(dst, (num_threads-1)*thread_partition,
                        input_num, num_threads-1, &barrier,
                        &shared_counters, &indexes,
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

template <typename Key, typename Value>
  bool rs2_pair_cmp (std::pair<Key, Value> a,
                     std::pair<Key, Value> b) {
  return std::get<0>(a) < std::get<0>(b);
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs2_helper_s(RandomAccessIterator dst,
                    const unsigned int (*super_indexes)[2],
                    int mask_bits,
                    int partition_bits) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  unsigned int partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

  for (unsigned int s = 0; s < partitions; s++) {
    s_begin = super_indexes[s][0];
    s_end = super_indexes[s][1];
    if (s_end - s_begin < 2)
      continue;
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < partitions) {
      std::sort(dst + s_begin, dst + s_end, rs2_pair_cmp<Key, Value>);
      continue;
    }
    // Setup counters for counting sort.
    for (unsigned int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (unsigned int i = 0; i < partitions - 1; i++) {
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
      idx_c = (h & mask) >> shift;
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
      continue;
    }

    // Reset indexes
    indexes[0][0] = s_begin;
    for (unsigned int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    rs2_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void rs2_helper_p(RandomAccessIterator dst,
                    const std::vector<std::pair<unsigned int,
                    unsigned int>>& super_indexes,
                    int mask_bits,
                    int partition_bits,
                    std::atomic_uint* super_counter) {
  std::pair<Key, Value> tmp_bucket;
  Key h, mask;
  unsigned int s_idx, partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  mask = (1ULL << mask_bits);
  mask -= 1;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

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
    if (s_end - s_begin < partitions) {
      std::sort(dst + s_begin, dst + s_end, rs2_pair_cmp<Key, Value>);
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    // Setup counters for counting sort.
    for (unsigned int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (unsigned int i = 0; i < partitions - 1; i++) {
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
      idx_c = (h & mask) >> shift;
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
    for (unsigned int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    rs2_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
    s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
  }
}

// radix_sort_2 use std::sort when input size is smaller than p.
template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_sort_2(RandomAccessIterator dst,
                    unsigned int input_num,
                    int partition_bits,
                    int num_threads) {
  static_assert(std::is_unsigned<Key>::value, "Key must be an unsigned arithmic type.");
  int shift, partitions, thread_partition, new_mask_bits;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  thread_partition = input_num / num_threads;

  shift = sizeof(Key)*8 - partition_bits;

  std::vector<std::atomic_ullong> shared_counters(partitions);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs1_worker<Key,Value,
                             RandomAccessIterator>,
                             dst, i * thread_partition,
                             (i+1) * thread_partition,
                             i, &barrier, &shared_counters, &indexes,
                             partitions, shift);
  }

  rs1_worker<Key,Value>(dst, (num_threads-1)*thread_partition,
                        input_num, num_threads-1, &barrier,
                        &shared_counters, &indexes,
                        partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = sizeof(Key)*8 - partition_bits;

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(rs2_helper_p<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, &a_counter);
  }
  rs2_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_sort_3(RandomAccessIterator dst,
                    unsigned int input_num,
                    int partition_bits) {
  int shift, new_mask_bits;
  unsigned int iter, idx_i, idx_j, idx_c, partitions;
  std::size_t h;
  std::atomic_uint a_counter(0);
  std::tuple<std::size_t, Key, Value> tmp_bucket;

  partitions = 1 << partition_bits;
  shift = 64 - partition_bits;

  unsigned int counters[partitions];
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);

  // Setup counters for counting sort.
  for (unsigned int i = 0; i < partitions; i++)
    counters[i] = 0;
  indexes[0].first = 0;
  for (unsigned int i = 0; i < input_num; i++) {
    h = std::get<0>(dst[i]);
    counters[h >> shift]++;
  }
  for (unsigned int i = 0; i < partitions - 1; i++) {
    indexes[i].second = indexes[i+1].first = indexes[i].first + counters[i];
  }
  indexes[partitions-1].second = indexes[partitions-1].first
  + counters[partitions-1];

  iter = 0;
  while (iter < partitions) {
    idx_i = indexes[iter].first;
    if (idx_i >= indexes[iter].second) {
      iter++;
      continue;
    }
    tmp_bucket = std::move(dst[idx_i]);
    do {
      h = std::get<0>(tmp_bucket);
      idx_c = h  >> shift;
      idx_j = indexes[idx_c].first++;
      std::swap(dst[idx_j], tmp_bucket);
    } while (idx_j > idx_i);
  }

  // Reset indexes
  indexes[0].first = 0;
  for (unsigned int i = 1; i < partitions; i++) {
    indexes[i].first = indexes[i-1].second;
  }
  new_mask_bits = 64 - partition_bits;

  bf6_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
}
#endif
