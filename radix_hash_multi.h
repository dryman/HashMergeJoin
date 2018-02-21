#ifndef RADIX_HASH_MULTI_H
#define RADIX_HASH_MULTI_H 1

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include "radix_hash.h"
#include <atomic>
#include "pthread_barrier.h"

template <typename Key,
  typename Value,
  typename Hash,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_worker(BidirectionalIterator begin,
                         BidirectionalIterator end,
                         RandomAccessIterator dst,
                         pthread_barrier_t* barrier,
                         std::atomic_int* shared_counters,
                         std::tuple<std::size_t, Key, Value>* buffer,
                         int num_iter,
                         int partitions,
                         std::size_t mask,
                         int shift) {
  std::size_t h;
  int dst_idx;
  int local_counters[partitions];
  for (int i = 0; i < partitions; i++)
    local_counters[i] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    local_counters[(h & mask) >> shift]++;
  }
  for (int i = 0; i < partitions; i++) {
    shared_counters[i].fetch_add(local_counters[i], std::memory_order_relaxed);
  }

  if (pthread_barrier_wait(barrier) ==
      PTHREAD_BARRIER_SERIAL_THREAD) {
    local_counters[0] = 0;
    for (unsigned int i = 1; i < partitions; i++) {
      local_counters[i] = shared_counters[i-1].load(std::memory_order_relaxed) +
        local_counters[i-1];
    }
    for (unsigned int i = 0; i < partitions; i++) {
      shared_counters[i].store(local_counters[i], std::memory_order_relaxed);
    }
    pthread_barrier_wait(barrier);
  } else {
    pthread_barrier_wait(barrier);
  }

  if (num_iter == 1) {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      dst_idx = shared_counters[(h & mask) >> shift]
        .fetch_add(1, std::memory_order_relaxed);
      std::get<0>(dst[dst_idx]) = h;
      std::get<1>(dst[dst_idx]) = iter->first;
      std::get<2>(dst[dst_idx]) = iter->second;
    }
  } else {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      dst_idx = shared_counters[(h & mask) >> shift]
        .fetch_add(1, std::memory_order_relaxed);
      std::get<0>(buffer[dst_idx]) = h;
      std::get<1>(buffer[dst_idx]) = iter->first;
      std::get<2>(buffer[dst_idx]) = iter->second;
    }
  }
}

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_multi(BidirectionalIterator begin,
                        BidirectionalIterator end,
                        RandomAccessIterator dst,
                        int mask_bits,
                        int partition_bits,
                        int nosort_bits,
                        int num_threads) {
  int input_num, num_iter, iter, shift, shift1, shift2, dst_idx, anchor_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, h1, h2, h_cnt, mask, mask1, mask2, anchor_h1;

  input_num = std::distance(begin, end);
  int thread_partition = input_num / num_threads;
  std::vector<std::thread> threads(num_threads);
  pthread_barrier_t barrier;

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

  num_iter = (mask_bits - nosort_bits
              + partition_bits - 1) / partition_bits;
  mask = (1ULL << mask_bits) - 1;
  shift = mask_bits - partition_bits;
  shift = shift < 0 ? 0 : shift;

  // invariant: counters[partitions] is the total count
  std::atomic_int shared_counters[partitions];
  int counters[partitions];
  int flush_counters[partitions];
  int idx_counters[partitions];

  for (int i = 0; i < partitions; i++)
    shared_counters[i] = 0;

  std::unique_ptr<std::tuple<std::size_t, Key, Value>[]> buffers[2];
  buffers[0] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);
  buffers[1] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);

  pthread_barrier_init(&barrier, NULL, num_threads);

  for (int i = 0; i < num_threads - 1; i++) {
    threads[i] = std::thread(radix_hash_worker<Key, Value, Hash,
                             BidirectionalIterator, RandomAccessIterator>,
                             begin + i * thread_partition,
                             begin + (i + 1) * thread_partition,
                             dst, &barrier, shared_counters,
                             buffers[0], num_iter, partitions, mask, shift);
  }
  threads[num_threads - 1] =
  std::thread(radix_hash_worker<Key, Value, Hash,
              BidirectionalIterator, RandomAccessIterator>,
              begin + (num_threads - 1) * thread_partition,
              end, dst, &barrier, shared_counters,
              buffers[0], num_iter, partitions, mask, shift);
  for (int i = 0; i < num_threads; i++) {
    threads[i].join();
  }
  pthread_barrier_destroy(&barrier);
  if (num_iter == 1)
    return;

  for (int i = 0; i < partitions; i++) {
    counters[i] = shared_counters[i].load(std::memory_order_relaxed);
  }

  for (iter = 0; iter < num_iter - 2; iter++) {
    mask1 = (1ULL << (mask_bits - partition_bits * iter)) - 1;
    shift1 = mask_bits - partition_bits * (iter + 1);
    shift1 = shift1 < 0 ? 0 : shift1;
    mask2 = (1ULL << (mask_bits - partition_bits * (iter + 1))) - 1;
    shift2 = mask_bits - partition_bits * (iter + 2);
    shift2 = shift2 < 0 ? 0 : shift2;
    for (int k = 0; k < partitions; k++) {
      counters[k] = 0;
    }
    anchor_h1 = (std::get<0>(buffers[iter & 1][0]) & mask1) >> shift1;
    anchor_idx = 0;

    for (int j = 0; j < input_num; j++) {
      h = std::get<0>(buffers[iter & 1][j]);
      h1 = (h & mask1) >> shift1;
      h2 = (h & mask2) >> shift2;
      if (h1 == anchor_h1) {
        counters[h2]++;
      } else {
        // flush counters
        idx_counters[0] = anchor_idx;
        for (unsigned int k = 1; k < partitions; k++) {
          idx_counters[k] = idx_counters[k-1] + counters[k-1];
        }
        for (int k = anchor_idx; k < j; k++) {
          h = std::get<0>(buffers[iter & 1][k]);
          buffers[(iter + 1) & 1][idx_counters[(h & mask2) >> shift2]++] =
          buffers[iter & 1][k];
        }
        // clear up counters and reset anchor
        for (int k = 0; k < partitions; k++) {
          counters[k] = 0;
        }
        anchor_idx = j;
        anchor_h1 = h1;
        counters[h2]++;
      }
    }
    // flush counters
    idx_counters[0] = anchor_idx;
    for (unsigned int k = 1; k < partitions; k++) {
      idx_counters[k] = idx_counters[k-1] + counters[k-1];
    }
    for (int k = anchor_idx; k < input_num; k++) {
      h = std::get<0>(buffers[iter & 1][k]);
      buffers[(iter + 1) & 1][idx_counters[(h & mask2) >> shift2]++] =
        buffers[iter & 1][k];
    }
  }

  // flush the last buffer to dst.
  mask1 = (1ULL << (mask_bits - partition_bits * iter)) - 1;
  shift1 = mask_bits - partition_bits * (iter + 1);
  shift1 = shift1 < 0 ? 0 : shift1;
  mask2 = (1ULL << (mask_bits - partition_bits * (iter + 1))) - 1;
  shift2 = mask_bits - partition_bits * (iter + 2);
  shift2 = shift2 < 0 ? 0 : shift2;

  for (int k = 0; k < partitions; k++) {
    counters[k] = 0;
  }
  anchor_h1 = (std::get<0>(buffers[iter & 1][0]) & mask1) >> shift1;
  anchor_idx = 0;
  for (int j = 0; j < input_num; j++) {
    h = std::get<0>(buffers[iter & 1][j]);
    h1 = (h & mask1) >> shift1;
    h2 = (h & mask2) >> shift2;
    if (h1 == anchor_h1) {
      counters[h2]++;
    } else {
      // flush counters
      for (int k = 0; k < partitions; k++) {
        flush_counters[k] = 0;
      }
      idx_counters[0] = anchor_idx;
      for (unsigned int k = 1; k < partitions; k++) {
        idx_counters[k] = idx_counters[k-1] + counters[k-1];
      }

      for (int k = anchor_idx; k < j; k++) {
        h = std::get<0>(buffers[iter & 1][k]);
        h_cnt = (h & mask2) >> shift2;
        dst_idx = idx_counters[h_cnt]++;
        flush_counters[h_cnt]++;
        *(dst + dst_idx) = buffers[iter & 1][k];
        if (nosort_bits == 0) {
          for (int m = flush_counters[h_cnt]; m < counters[h_cnt]; m++) {
            if (HashTupleCmp(*(dst + dst_idx), *(dst + dst_idx - 1), mask)) {
              std::swap(*(dst + dst_idx), *(dst + dst_idx - 1));
              dst_idx--;
            } else break;
          }
        }
      }
      // clear up counters and reset anchor
      for (int k = 0; k < partitions; k++) {
        counters[k] = 0;
      }
      anchor_idx = j;
      anchor_h1 = h1;
      counters[h2]++;
    }
  }
  // flush counters
  for (int k = 0; k < partitions; k++) {
    flush_counters[k] = 0;
  }
  idx_counters[0] = anchor_idx;
  for (unsigned int k = 1; k < partitions; k++) {
    idx_counters[k] = idx_counters[k-1] + counters[k-1];
  }

  for (int k = anchor_idx; k < input_num; k++) {
    h = std::get<0>(buffers[iter & 1][k]);
    h_cnt = (h & mask2) >> shift2;
    dst_idx = idx_counters[h_cnt]++;
    flush_counters[h_cnt]++;
    *(dst + dst_idx) = buffers[iter & 1][k];
    if (nosort_bits == 0) {
      for (int m = flush_counters[h_cnt]; m < counters[h_cnt]; m++) {
        if (HashTupleCmp(*(dst + dst_idx), *(dst + dst_idx - 1), mask)) {
          std::swap(*(dst + dst_idx), *(dst + dst_idx - 1));
          dst_idx--;
        } else break;
      }
    }
  }
}

#endif // RADIX_HASH_MULTI_H
