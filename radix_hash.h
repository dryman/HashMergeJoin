#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

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

// namespace radix_hash?

static inline
int compute_power(int input_num) {
  return 64 - __builtin_clzll(input_num);
}

static inline
std::size_t compute_mask(int input_num) {
  return (1ULL << ::compute_power(input_num)) - 1;
}

template<typename Key,
  typename Value>
  bool HashTupleCmp(std::tuple<std::size_t, Key, Value>const& a,
                    std::tuple<std::size_t, Key, Value>const& b,
                    std::size_t mask) {
  std::size_t a_hash, b_hash, a_mask_hash, b_mask_hash;
  a_hash = std::get<0>(a);
  b_hash = std::get<0>(b);
  a_mask_hash = a_hash & mask;
  b_mask_hash = b_hash & mask;
  if (a_mask_hash != b_mask_hash) {
    return a_mask_hash < b_mask_hash;
  }
  if (a_hash != b_hash) {
    return a_hash < b_hash;
  }
  return std::get<1>(a) < std::get<1>(b);
}

template<typename Key,
  typename Value>
  bool HashTupleEquiv(std::tuple<std::size_t, Key, Value>const& a,
                    std::tuple<std::size_t, Key, Value>const& b) {
  return std::get<0>(a) == std::get<0>(b) &&
    std::get<1>(a) == std::get<1>(b);
}

template<typename RandomAccessIterator>
static inline
void bf3_insertion_inner(RandomAccessIterator dst,
                         unsigned int idx,
                         unsigned int limit,
                         std::size_t mask) {
  std::size_t h1, h2;
  while (idx > limit) {
    h1 = std::get<0>(dst[idx]);
    h2 = std::get<0>(dst[idx-1]);
    if ((h1 & mask) > (h2 & mask))
      break;
    if ((h1 & mask) < (h2 & mask)) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    if (h1 > h2)
      break;
    if (h1 < h2) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    if (std::get<1>(dst[idx]) < std::get<1>(dst[idx-1])) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    break;
  }
}

template<typename RandomAccessIterator>
static inline
void bf3_insertion_outer(RandomAccessIterator dst,
                         unsigned int idx_begin,
                         unsigned int idx_end,
                         std::size_t mask) {
  for (unsigned int idx = idx_begin + 1;
       idx < idx_end; idx++) {
    bf3_insertion_inner<RandomAccessIterator>(dst, idx, idx_begin, mask);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void bf3_helper(RandomAccessIterator dst,
                  const unsigned int (*super_indexes)[2],
                  int mask_bits,
                  int partition_bits,
                  int nosort_bits) {
  std::tuple<std::size_t, Key, Value> tmp_bucket;
  std::size_t h, mask;
  unsigned int partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits) - 1ULL;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

  // TODO change s to a atomic variable and use a boolean to determine
  // to use it or not.
  for (unsigned int s = 0; s < partitions; s++) {
    s_begin = super_indexes[s][0];
    s_end = super_indexes[s][1];
    if (s_end - s_begin < 2)
      continue;
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < sqrt_partitions) {
      bf3_insertion_outer<RandomAccessIterator>(dst, s_begin, s_end, mask);
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

    iter = 0;
    while (iter < partitions) {
      idx_i = indexes[iter][0];
      if (idx_i >= indexes[iter][1]) {
        iter++;
        continue;
      }
      tmp_bucket = std::move(dst[idx_i]);
      do {
        h = std::get<0>(tmp_bucket);
        idx_c = (h & mask) >> shift;
        idx_j = indexes[idx_c][0]++;
        std::swap(dst[idx_j], tmp_bucket);
        if (idx_c == 0) {
          bf3_insertion_inner<RandomAccessIterator>
            (dst, idx_j, s_begin, mask);
        } else {
          bf3_insertion_inner<RandomAccessIterator>
            (dst, idx_j, indexes[idx_c-1][1], mask);
        }
      } while (idx_j > idx_i);
    }

    // End of counting sort. Recurse to next level.
    new_mask_bits = mask_bits - partition_bits;
    if (new_mask_bits <= nosort_bits)
      continue;
    // Reset indexes
    indexes[0][0] = s_begin;
    for (unsigned int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    bf3_helper<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits, nosort_bits);
  }
}

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf3(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int mask_bits,
      int partition_bits,
      int nosort_bits) {
  int shift, dst_idx;
  unsigned int partitions = 1 << partition_bits;
  std::size_t h, mask;
  int new_mask_bits;

  if (mask_bits <= nosort_bits) {
    for (auto iter = begin; iter != end; iter++) {
      h = Hash{}(std::get<0>(*iter));
      std::get<0>(*dst) = h;
      std::get<1>(*dst) = iter->first;
      std::get<2>(*dst) = iter->second;
      ++dst;
    }
    return;
  }

  mask = (1ULL << mask_bits) - 1;
  shift = mask_bits - partition_bits;
  shift = shift < 0 ? 0 : shift;

  // invariant: counters[partitions] is the total count
  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

  for (unsigned int i = 0; i < partitions + 1; i++)
    counters[i] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h & mask) >> shift]++;
  }

  indexes[0][0] = 0;
  for (unsigned int i = 0; i < partitions - 1; i++) {
    indexes[i][1] = indexes[i+1][0] = indexes[i][0] + counters[i];
  }
  indexes[partitions-1][1] = indexes[partitions-1][0]
  + counters[partitions-1];

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = indexes[(h & mask) >> shift][0]++;
    std::get<0>(dst[dst_idx]) = h;
    std::get<1>(dst[dst_idx]) = iter->first;
    std::get<2>(dst[dst_idx]) = iter->second;
  }

  new_mask_bits = mask_bits - partition_bits;
  if (new_mask_bits <= nosort_bits)
    return;

  /*
   * This logic should be equivalent to the one implemented below.
   * Unfortunately on OSX clang-800.0.42.1 it optimizes it in a wrong
   * way such that the conent in indexes are wrong.
  indexes[0][0] = 0;
  for (unsigned int i = 1; i < partitions; i++) {
    indexes[i][0] = indexes[i-1][1];
  }
  */
  indexes[0][0] = 0;
  for (unsigned int i = 0; i < partitions - 1; i++) {
    indexes[i][1] = indexes[i+1][0] = indexes[i][0] + counters[i];
  }
  indexes[partitions-1][1] = indexes[partitions-1][0]
  + counters[partitions-1];

  bf3_helper<Key, Value, RandomAccessIterator>
  (dst, indexes, new_mask_bits, partition_bits, nosort_bits);
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void bf4_helper(RandomAccessIterator dst,
                  const std::vector<std::pair<unsigned int,
                                              unsigned int>>& super_indexes,
                  int mask_bits,
                  int partition_bits,
                  int nosort_bits,
                  std::atomic_uint* super_counter) {
  std::tuple<std::size_t, Key, Value> tmp_bucket;
  std::size_t h, mask;
  unsigned int s_idx, partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits) - 1ULL;
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
      bf3_insertion_outer<RandomAccessIterator>(dst, s_begin, s_end, mask);
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

    iter = 0;
    while (iter < partitions) {
      idx_i = indexes[iter][0];
      if (idx_i >= indexes[iter][1]) {
        iter++;
        continue;
      }
      tmp_bucket = std::move(dst[idx_i]);
      do {
        h = std::get<0>(tmp_bucket);
        idx_c = (h & mask) >> shift;
        idx_j = indexes[idx_c][0]++;
        std::swap(dst[idx_j], tmp_bucket);
        if (idx_c == 0) {
          bf3_insertion_inner<RandomAccessIterator>
            (dst, idx_j, s_begin, mask);
        } else {
          bf3_insertion_inner<RandomAccessIterator>
            (dst, idx_j, indexes[idx_c-1][1], mask);
        }
      } while (idx_j > idx_i);
    }

    // End of counting sort. Recurse to next level.
    new_mask_bits = mask_bits - partition_bits;
    if (new_mask_bits <= nosort_bits) {
      s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
      continue;
    }
    // Reset indexes
    indexes[0][0] = s_begin;
    for (unsigned int i = 1; i < partitions; i++) {
      indexes[i][0] = indexes[i-1][1];
    }
    bf3_helper<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits, nosort_bits);
    s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
  }
}

template<typename Key,
  typename Value,
  typename Hash,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf4_worker(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             RandomAccessIterator dst,
                             ThreadBarrier* barrier,
                             std::vector<std::atomic_uint>* shared_counters,
                             std::vector<std::pair<unsigned int,
                             unsigned int>>* indexes,
                             unsigned int partitions,
                             std::size_t mask,
                             int shift) {
  std::size_t h;
  int dst_idx;
  int local_counters[partitions];
  for (unsigned int i = 0; i < partitions; i++)
    local_counters[i] = 0;

  // TODO maybe we can make no sort version in worker as well.
  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    local_counters[(h & mask) >> shift]++;
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
      (*shared_counters)[i].store(local_counters[i], std::memory_order_relaxed);
    }
    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = (*shared_counters)[(h & mask) >> shift]
      .fetch_add(1, std::memory_order_relaxed);
    std::get<0>(dst[dst_idx]) = h;
    std::get<1>(dst[dst_idx]) = iter->first;
    std::get<2>(dst[dst_idx]) = iter->second;
  }
}

// Features:
// * workers use atomic.
template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf4(BidirectionalIterator begin,
                      BidirectionalIterator end,
                      RandomAccessIterator dst,
                      int mask_bits,
                      int partition_bits,
                      int nosort_bits,
                      int num_threads) {
  int input_num, shift, partitions, thread_partition, new_mask_bits;
  std::size_t h, mask;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  if (mask_bits <= nosort_bits) {
    for (auto iter = begin; iter != end; iter++) {
      h = Hash{}(std::get<0>(*iter));
      std::get<0>(*dst) = h;
      std::get<1>(*dst) = iter->first;
      std::get<2>(*dst) = iter->second;
      ++dst;
    }
    return;
  }

  mask = (1ULL << mask_bits) - 1;
  shift = mask_bits - partition_bits;
  shift = shift < 0 ? 0 : shift;

  std::vector<std::atomic_uint> shared_counters(partitions);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(radix_hash_bf4_worker<Key,Value,Hash,
                             BidirectionalIterator, RandomAccessIterator>,
                             begin + i * thread_partition,
                             begin + (i+1) * thread_partition,
                             dst, &barrier, &shared_counters, &indexes,
                             partitions, mask, shift);
  }

  radix_hash_bf4_worker<Key,Value,Hash>(begin+(num_threads-1)*thread_partition,
                                        end,
                                        dst, &barrier,
                                        &shared_counters, &indexes,
                                        partitions, mask, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = mask_bits - partition_bits;
  if (new_mask_bits <= nosort_bits)
    return;

  for (int i = 0; i < num_threads; i++) {
    threads[i] = std::thread(bf4_helper<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, nosort_bits,
                             &a_counter);
  }
  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }
}

template<typename Key,
  typename Value,
  typename Hash,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf5_worker(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             RandomAccessIterator dst,
                             unsigned int thread_id,
                             unsigned int thread_num,
                             ThreadBarrier* barrier,
                             std::vector<unsigned int>* shared_counters,
                             std::vector<std::pair<unsigned int,
                             unsigned int>>* indexes,
                             unsigned int partitions,
                             std::size_t mask,
                             int shift) {
  std::size_t h;
  unsigned int dst_idx, tmp_cnt;


  // TODO maybe we can make no sort version in worker as well.
  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*shared_counters)[thread_id*partitions + ((h&mask)>>shift)]++;
  }

  // in barrier
  if (barrier->wait()) {
    tmp_cnt = 0;
    for (unsigned int i = 0; i < partitions; i++) {
      for (unsigned int j = 0; j < thread_num; j++) {
        tmp_cnt += (*shared_counters)[j*partitions + i];
        (*shared_counters)[j*partitions + i] =
          tmp_cnt - (*shared_counters)[j*partitions + i];
      }
    }
    (*indexes)[0].first = 0;
    for (unsigned int i = 1; i < partitions; i++) {
      (*indexes)[i-1].second = (*indexes)[i].first = (*shared_counters)[i];
    }
    (*indexes)[partitions-1].second = tmp_cnt;

    /*
    std::cout << "shared_counters:\n";
    for (int i = 0; i < thread_num; i++) {
      std::cout << "t" << i << ": ";
      for (int j = 0; j < partitions; j++) {
        std::cout << (*shared_counters)[i*partitions + j] << ", ";
      }
      std::cout << "\n";
    }
    */

    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = (*shared_counters)[thread_id*partitions + ((h&mask)>>shift)]++;
    std::get<0>(dst[dst_idx]) = h;
    std::get<1>(dst[dst_idx]) = iter->first;
    std::get<2>(dst[dst_idx]) = iter->second;
  }
}

// Features:
// * workers do not use atomic (less memory sync)
template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf5(BidirectionalIterator begin,
                      BidirectionalIterator end,
                      RandomAccessIterator dst,
                      int mask_bits,
                      int partition_bits,
                      int nosort_bits,
                      int num_threads) {
  int input_num, shift, partitions, thread_partition, new_mask_bits;
  std::size_t h, mask;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  if (mask_bits <= nosort_bits) {
    for (auto iter = begin; iter != end; iter++) {
      h = Hash{}(std::get<0>(*iter));
      std::get<0>(*dst) = h;
      std::get<1>(*dst) = iter->first;
      std::get<2>(*dst) = iter->second;
      ++dst;
    }
    return;
  }

  mask = (1ULL << mask_bits) - 1;
  shift = mask_bits - partition_bits;
  shift = shift < 0 ? 0 : shift;

  std::vector<unsigned int> shared_counters(partitions*num_threads);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(radix_hash_bf5_worker<Key,Value,Hash,
                             BidirectionalIterator, RandomAccessIterator>,
                             begin + i * thread_partition,
                             begin + (i+1) * thread_partition,
                             dst, i, num_threads,
                             &barrier, &shared_counters, &indexes,
                             partitions, mask, shift);
  }

  radix_hash_bf5_worker<Key,Value,Hash>(begin+(num_threads-1)*thread_partition,
                                        end, dst, num_threads-1, num_threads,
                                        &barrier, &shared_counters, &indexes,
                                        partitions, mask, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = mask_bits - partition_bits;
  if (new_mask_bits <= nosort_bits)
    return;

  for (int i = 0; i < num_threads; i++) {
    threads[i] = std::thread(bf4_helper<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, nosort_bits,
                             &a_counter);
  }
  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }
}

template<typename RandomAccessIterator>
static inline
void bf6_insertion_inner(RandomAccessIterator dst,
                         unsigned int idx,
                         unsigned int limit) {
  std::size_t h1, h2;
  while (idx > limit) {
    h1 = std::get<0>(dst[idx]);
    h2 = std::get<0>(dst[idx-1]);
    if (h1 > h2)
      break;
    if (h1 < h2) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    if (std::get<1>(dst[idx]) < std::get<1>(dst[idx-1])) {
      std::swap(dst[idx], dst[idx-1]);
      idx--;
      continue;
    }
    break;
  }
}

template<typename RandomAccessIterator>
static inline
void bf6_insertion_outer(RandomAccessIterator dst,
                         unsigned int idx_begin,
                         unsigned int idx_end) {
  for (unsigned int idx = idx_begin + 1;
       idx < idx_end; idx++) {
    bf6_insertion_inner<RandomAccessIterator>(dst, idx, idx_begin);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void bf6_helper_s(RandomAccessIterator dst,
                    const unsigned int (*super_indexes)[2],
                    int mask_bits,
                    int partition_bits) {
  std::tuple<std::size_t, Key, Value> tmp_bucket;
  std::size_t h, mask;
  unsigned int partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits) - 1ULL;
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
      bf6_insertion_outer<RandomAccessIterator>(dst, s_begin, s_end);
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
    bf6_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void bf6_helper_p(RandomAccessIterator dst,
                    const std::vector<std::pair<unsigned int,
                    unsigned int>>& super_indexes,
                    int mask_bits,
                    int partition_bits,
                    std::atomic_uint* super_counter) {
  std::tuple<std::size_t, Key, Value> tmp_bucket;
  std::size_t h, mask;
  unsigned int s_idx, partitions, sqrt_partitions, shift,
    idx_i, idx_j, idx_c, s_begin, s_end, iter;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  sqrt_partitions = 1 << (partition_bits / 2);
  mask = (1ULL << mask_bits) - 1ULL;
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
      bf6_insertion_outer<RandomAccessIterator>(dst, s_begin, s_end);
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
    bf6_helper_s<Key,Value,RandomAccessIterator>
      (dst, indexes, new_mask_bits, partition_bits);
    s_idx = super_counter->fetch_add(1, std::memory_order_relaxed);
  }
}

template<typename Key,
  typename Value,
  typename Hash,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf6_worker(BidirectionalIterator begin,
                             BidirectionalIterator end,
                             RandomAccessIterator dst,
                             unsigned int thread_id,
                             unsigned int thread_num,
                             ThreadBarrier* barrier,
                             std::vector<unsigned int>* shared_counters,
                             std::vector<std::pair<unsigned int,
                             unsigned int>>* indexes,
                             unsigned int partitions,
                             int shift) {
  std::size_t h;
  unsigned int dst_idx, tmp_cnt;

  // TODO maybe we can make no sort version in worker as well.
  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    (*shared_counters)[thread_id*partitions + (h>>shift)]++;
  }

  // in barrier
  if (barrier->wait()) {
    tmp_cnt = 0;
    for (unsigned int i = 0; i < partitions; i++) {
      for (unsigned int j = 0; j < thread_num; j++) {
        tmp_cnt += (*shared_counters)[j*partitions + i];
        (*shared_counters)[j*partitions + i] =
          tmp_cnt - (*shared_counters)[j*partitions + i];
      }
    }
    (*indexes)[0].first = 0;
    for (unsigned int i = 1; i < partitions; i++) {
      (*indexes)[i-1].second = (*indexes)[i].first = (*shared_counters)[i];
    }
    (*indexes)[partitions-1].second = tmp_cnt;

    barrier->wait();
  } else {
    barrier->wait();
  }

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = (*shared_counters)[thread_id*partitions + (h>>shift)]++;
    std::get<0>(dst[dst_idx]) = h;
    std::get<1>(dst[dst_idx]) = iter->first;
    std::get<2>(dst[dst_idx]) = iter->second;
  }
}

// Features:
// * Use all bits to sort
// * worker do not use atomic (less memory sync)
template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf6(BidirectionalIterator begin,
                      BidirectionalIterator end,
                      RandomAccessIterator dst,
                      int partition_bits,
                      int nosort_bits,
                      int num_threads) {
  int input_num, shift, partitions, thread_partition, new_mask_bits;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  input_num = std::distance(begin, end);
  thread_partition = input_num / num_threads;

  shift = 64 - partition_bits;

  std::vector<unsigned int> shared_counters(partitions*num_threads);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(radix_hash_bf6_worker<Key,Value,Hash,
                             BidirectionalIterator, RandomAccessIterator>,
                             begin + i * thread_partition,
                             begin + (i+1) * thread_partition,
                             dst, i, num_threads,
                             &barrier, &shared_counters, &indexes,
                             partitions, shift);
  }

  radix_hash_bf6_worker<Key,Value,Hash>(begin+(num_threads-1)*thread_partition,
                                        end, dst, num_threads-1, num_threads,
                                        &barrier, &shared_counters, &indexes,
                                        partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = 64 - partition_bits;

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(bf6_helper_p<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, &a_counter);
  }
  bf6_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_hash_bf7(RandomAccessIterator dst,
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

template<typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_hash_bf8_worker(RandomAccessIterator dst,
                             unsigned int begin,
                             unsigned int end,
                             unsigned int thread_id,
                             ThreadBarrier* barrier,
                             std::vector<std::atomic_ullong>* shared_counters,
                             std::vector<std::pair<unsigned int,
                             unsigned int>>* indexes,
                             unsigned int partitions,
                             int shift) {
  std::size_t h;
  int local_counters[partitions];
  unsigned long long old_val, new_val, unsort, sort, flag, counter_mask;
  unsigned int iter, idx_i, idx_j, idx_c;
  std::tuple<std::size_t, Key, Value> tmp_bucket;

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

  /*
  std::cout << "after sort:\n" << std::dec;
  for (unsigned int i = begin; i < end; i++) {
    h = std::get<0>(dst[i]);
    std::cout << (h >> shift) << ": " << h << "\n";
  }
  */
}

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void radix_hash_bf8(RandomAccessIterator dst,
                      unsigned int input_num,
                      int partition_bits,
                      int num_threads) {
  int shift, partitions, thread_partition, new_mask_bits;
  std::atomic_uint a_counter(0);
  ThreadBarrier barrier(num_threads);

  partitions = 1 << partition_bits;
  thread_partition = input_num / num_threads;

  shift = 64 - partition_bits;

  std::vector<std::atomic_ullong> shared_counters(partitions);
  std::vector<std::pair<unsigned int, unsigned int>> indexes(partitions);
  std::vector<std::thread> threads(num_threads);

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(radix_hash_bf8_worker<Key,Value,
                             RandomAccessIterator>,
                             dst, i * thread_partition,
                             (i+1) * thread_partition,
                             i, &barrier, &shared_counters, &indexes,
                             partitions, shift);
  }

  radix_hash_bf8_worker<Key,Value>(dst, (num_threads-1)*thread_partition,
                                   input_num, num_threads-1, &barrier,
                                   &shared_counters, &indexes,
                                   partitions, shift);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }

  new_mask_bits = 64 - partition_bits;

  for (int i = 0; i < num_threads-1; i++) {
    threads[i] = std::thread(bf6_helper_p<Key,Value, RandomAccessIterator>,
                             dst, indexes, new_mask_bits,
                             partition_bits, &a_counter);
  }
  bf6_helper_p<Key,Value, RandomAccessIterator>(
      dst, indexes, new_mask_bits,
      partition_bits, &a_counter);
  for (int i = 0; i < num_threads-1; i++) {
    threads[i].join();
  }
}
#endif
