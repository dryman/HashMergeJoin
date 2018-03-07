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

// TODO: Should benchmark the speed difference between array or vector.
// TODO: use boolean template to create an export_hash variant
// TODO: use integer key and identity hash function for unit testing
template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_df1(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int mask_bits,
      int partition_bits,
      int nosort_bits) {
  int input_num, num_iter, shift, dst_idx, prev_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, mask;

  input_num = std::distance(begin, end);

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
  int counters[partitions + 1];
  int counter_stack[num_iter - 1][partitions + 1];
  int iter_stack[num_iter - 1];

  for (int i = 0; i < partitions + 1; i++)
    counters[i] = 0;
  for (int i = 0; i < num_iter - 1; i++)
    for (int j = 0; j < partitions + 1; j++)
      counter_stack[i][j] = 0;
  for (int i = 0; i < num_iter - 1; i++)
    iter_stack[i] = 0;

  std::unique_ptr<std::tuple<std::size_t, Key, Value>[]> buffers[2];
  buffers[0] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);
  buffers[1] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h & mask) >> shift]++;
  }
  for (int i = 1; i < partitions; i++) {
    counters[i] += counters[i - 1];
  }
  for (int i = partitions; i != 0; i--) {
    counters[i] = counters[i - 1];
  }
  counters[0] = 0;
  for (int i = 0; i < partitions + 1; i++)
    counter_stack[0][i] = counters[i];

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    // TODO make this a template inline function
    dst_idx = counters[(h & mask) >> shift]++;
    if (num_iter == 1) {
      std::get<0>(dst[dst_idx]) = h;
      std::get<1>(dst[dst_idx]) = iter->first;
      std::get<2>(dst[dst_idx]) = iter->second;
    } else {
      std::get<0>(buffers[0][dst_idx]) = h;
      std::get<1>(buffers[0][dst_idx]) = iter->first;
      std::get<2>(buffers[0][dst_idx]) = iter->second;
    }
  }
  if (num_iter == 1) return;


  for (int i = 0; true;) {
    if (counter_stack[i][iter_stack[i]] ==
        counter_stack[i][partitions]) {
      if (i == 0) {
        return;
      }
      iter_stack[i] = 0;
      iter_stack[i - 1]++;
      i--;
      continue;
    }
    mask = (1ULL << (mask_bits - partition_bits * (i + 1))) - 1;
    shift = mask_bits - partition_bits * (i + 2);
    shift = shift < 0 ? 0 : shift;

    // clear counters
    for (int j = 0; j < partitions; j++)
      counters[j] = 0;
    // accumulate counters
    for (int j = counter_stack[i][iter_stack[i]];
         j < counter_stack[i][iter_stack[i] + 1];
         j++) {
      h = std::get<0>(buffers[i & 1][j]);
      counters[(h & mask) >> shift]++;
    }
    prev_idx = counter_stack[i][iter_stack[i]];
    for (int j = 1; j < partitions; j++) {
      counters[j] += counters[j - 1];
    }
    for (int j = partitions; j != 0; j--) {
      counters[j] = counters[j - 1] + prev_idx;;
    }
    counters[0] = prev_idx;

    if (i < num_iter - 2) {
      for (int j = 0; j < partitions + 1; j++)
        counter_stack[i + 1][j] = counters[j];
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        h = std::get<0>(buffers[i & 1][j]);
        buffers[(i + 1) & 1][counters[(h & mask) >> shift]++] =
          buffers[i & 1][j];
      }
      i++;
    } else {
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        h = std::get<0>(buffers[i & 1][j]);
        dst_idx = counters[(h & mask) >> shift]++;
        *(dst + dst_idx) = buffers[i & 1][j];
      }
      iter_stack[i]++;
    }
  }
}

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_df2(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int mask_bits,
      int partition_bits,
      int nosort_bits) {
  int input_num, num_iter, shift, dst_idx, prev_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, mask;

  input_num = std::distance(begin, end);

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
  int counters[partitions + 1];
  int counter_stack[num_iter][partitions + 1];
  int iter_stack[num_iter];

  for (int i = 0; i < partitions + 1; i++)
    counters[i] = 0;
  for (int i = 0; i < num_iter; i++)
    for (int j = 0; j < partitions + 1; j++)
      counter_stack[i][j] = 0;
  for (int i = 0; i < num_iter; i++)
    iter_stack[i] = 0;

  std::unique_ptr<std::tuple<std::size_t, Key, Value>[]> buffers[2];
  buffers[0] = std::make_unique<
    std::tuple<std::size_t, Key, Value>[]>(input_num);
  buffers[1] = std::make_unique<
    std::tuple<std::size_t, Key, Value>[]>(input_num);

  // Major difference to previous hash_sort: we calculate hash
  // value only once, but do an extra copy. Copy is likely to be cheaper
  // than hashing.
  dst_idx = 0;
  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    std::get<0>(buffers[1][dst_idx]) = h;
    std::get<1>(buffers[1][dst_idx]) = iter->first;
    std::get<2>(buffers[1][dst_idx]) = iter->second;
    dst_idx++;
  }

  for (int i = 0; i < input_num; i++) {
    h = std::get<0>(buffers[1][i]);
    counters[(h & mask) >> shift]++;
  }
  for (int i = 1; i < partitions; i++) {
    counters[i] += counters[i - 1];
  }
  for (int i = partitions; i != 0; i--) {
    counters[i] = counters[i - 1];
  }
  counters[0] = 0;
  for (int i = 0; i < partitions + 1; i++)
    counter_stack[0][i] = counters[i];

  if (num_iter == 1) {
    for (int i = 0; i < input_num; i++) {
      h = std::get<0>(buffers[1][i]);
      dst_idx = counters[(h & mask) >> shift]++;
      *(dst + dst_idx) = buffers[1][i];
    }
    return;
  } else {
    for (int i = 0; i < input_num; i++) {
      h = std::get<0>(buffers[1][i]);
      buffers[0][counters[(h & mask) >> shift]++] = buffers[1][i];
    }
  }

  for (int i = 0; true;) {
    if (counter_stack[i][iter_stack[i]] ==
        counter_stack[i][partitions]) {
      if (i == 0) {
        return;
      }
      iter_stack[i] = 0;
      iter_stack[i - 1]++;
      i--;
      continue;
    }
    mask = (1ULL << (mask_bits - partition_bits * (i + 1))) - 1;
    shift = mask_bits - partition_bits * (i + 2);
    shift = shift < 0 ? 0 : shift;

    // clear counters
    for (int j = 0; j < partitions; j++)
      counters[j] = 0;
    // accumulate counters
    for (int j = counter_stack[i][iter_stack[i]];
         j < counter_stack[i][iter_stack[i] + 1];
         j++) {
      h = std::get<0>(buffers[i & 1][j]);
      counters[(h & mask) >> shift]++;
    }
    prev_idx = counter_stack[i][iter_stack[i]];
    for (int j = 1; j < partitions; j++) {
      counters[j] += counters[j - 1];
    }
    for (int j = partitions; j != 0; j--) {
      counters[j] = counters[j - 1] + prev_idx;;
    }
    counters[0] = prev_idx;

    if (i < num_iter - 2) {
      for (int j = 0; j < partitions + 1; j++)
        counter_stack[i + 1][j] = counters[j];
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        h = std::get<0>(buffers[i & 1][j]);
        buffers[(i + 1) & 1][counters[(h & mask) >> shift]++] =
          buffers[i & 1][j];
      }
      i++;
    } else {
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        h = std::get<0>(buffers[i & 1][j]);
        dst_idx = counters[(h & mask) >> shift]++;
        *(dst + dst_idx) = buffers[i & 1][j];
      }
      iter_stack[i]++;
    }
  }
}

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf1(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int mask_bits,
      int partition_bits,
      int nosort_bits) {
  int input_num, num_iter, iter, shift, shift1, shift2, dst_idx, anchor_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, h1, h2, h_cnt, mask, mask1, mask2, anchor_h1;

  input_num = std::distance(begin, end);

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
  int counters[partitions];
  int flush_counters[partitions];
  int idx_counters[partitions];

  for (int i = 0; i < partitions; i++)
    counters[i] = 0;

  std::unique_ptr<std::tuple<std::size_t, Key, Value>[]> buffers[2];
  buffers[0] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);
  buffers[1] = std::make_unique<
  std::tuple<std::size_t, Key, Value>[]>(input_num);

  /*
  madvise(&*begin, input_num * (sizeof(begin->first) + sizeof(begin->second)),
          MADV_SEQUENTIAL);
  madvise(&buffers[0], input_num *
          (sizeof(begin->first) + sizeof(begin->second) + sizeof(std::size_t)),
          MADV_SEQUENTIAL);
  madvise(&buffers[1], input_num *
          (sizeof(begin->first) + sizeof(begin->second) + sizeof(std::size_t)),
          MADV_SEQUENTIAL);
  */

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h & mask) >> shift]++;
  }
  for (int i = 1; i < partitions; i++) {
    counters[i] += counters[i - 1];
  }
  for (int i = partitions - 1; i != 0; i--) {
    counters[i] = counters[i - 1];
  }
  counters[0] = 0;

  if (num_iter == 1) {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      dst_idx = counters[(h & mask) >> shift]++;
      std::get<0>(dst[dst_idx]) = h;
      std::get<1>(dst[dst_idx]) = std::move(iter->first);
      std::get<2>(dst[dst_idx]) = std::move(iter->second);
    }
    return;
  } else {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      dst_idx = counters[(h & mask) >> shift]++;
      std::get<0>(buffers[0][dst_idx]) = h;
      std::get<1>(buffers[0][dst_idx]) = std::move(iter->first);
      std::get<2>(buffers[0][dst_idx]) = std::move(iter->second);
    }
  }

  madvise(&*begin, input_num * (sizeof(begin->first) + sizeof(begin->second)),
          MADV_DONTNEED);

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
            std::move(buffers[iter & 1][k]);
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
      std::move(buffers[iter & 1][k]);
    }
  }

  madvise(&*dst, input_num *
          (sizeof(begin->first) + sizeof(begin->second) + sizeof(std::size_t)),
          MADV_WILLNEED);

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
        *(dst + dst_idx) = std::move(buffers[iter & 1][k]);
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
    *(dst + dst_idx) = std::move(buffers[iter & 1][k]);
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

template <typename Key,
  typename Value,
  typename RandomAccessIterator>
  void bf2_helper(RandomAccessIterator begin,
                  RandomAccessIterator end,
                  int mask_bits,
                  int partition_bits,
                  int nosort_bits) {
  std::tuple<std::size_t, Key, Value> tmp_bucket;
  uint64_t h, mask;
  //std::size_t h, mask;
  unsigned int partitions, shift, idx_i, idx_j;
  int new_mask_bits;

  partitions = 1 << partition_bits;
  mask = (1ULL << mask_bits) - 1ULL;
  shift = mask_bits < partition_bits ? 0 : mask_bits - partition_bits;

  int counters[partitions];
  int indexes[partitions][2];

  for (int i = 0; i < partitions; i++)
    counters[i] = 0;
  indexes[0][0] = 0;

  //  std::cout << "bf2_helper mask: " << mask << " shift: "
  //            << shift << " before: ";
  for (auto iter = begin; iter != end; ++iter) {
    h = std::get<0>(*iter);
    unsigned int counter_idx = (h & mask) >> shift;
    if (counter_idx >= partitions) {
      std::cout << "mask_bits: " << mask_bits
                << " partition_bits: " << partition_bits
                << " shift: " << shift
                << " h: " << h
                << " counter_idx: " << counter_idx
                << " partitions: " << partitions << "\n";
      assert(0);
    }
    //assert(counter_idx >= 0);
    //assert(counter_idx < partitions);
    counters[counter_idx]++;
    //std::cout << std::get<0>(*iter) << "[" << counter_idx << "], ";
  }
  //std::cout << "\nindexes: ";
  for (int i = 0; i < partitions - 1; i++) {
    indexes[i][1] = indexes[i+1][0] = indexes[i][0] + counters[i];
  }
  indexes[partitions-1][1] = indexes[partitions-1][0] + counters[partitions-1];
  //for (int i = 0; i < partitions; i++) {
  //  std::cout << "[" << indexes[i][0] << "," << indexes[i][1] << "], ";
  //}
  //std::cout << "\n";
  int i = 0;
  while (i < partitions) {
    idx_i = indexes[i][0];
    if (idx_i >= indexes[i][1]) {
      i++;
      continue;
    }
    tmp_bucket = begin[idx_i];
    do {
      h = std::get<0>(tmp_bucket);
      idx_j = indexes[(h & mask) >> shift][0]++;
      std::swap(begin[idx_j], tmp_bucket);
    } while (idx_j > idx_i);
  }

  //  std::cout << "bf2_helper after: ";
  //  for (auto iter = begin; iter != end; ++iter) {
  //    std::cout << std::get<1>(*iter) << ", ";
  //  }
  //  std::cout << "indexes: ";
  //  for (int i = 0; i < partitions; i++) {
  //    std::cout << "[" << indexes[i][0] << "," << indexes[i][1] << "], ";
  //  }
  //  std::cout << "\n";

  new_mask_bits = mask_bits - partition_bits;
  if (new_mask_bits <= nosort_bits)
    return;

  if (indexes[0][0] > 1) {
    bf2_helper<Key, Value, RandomAccessIterator>
      (begin, begin + indexes[0][0],
       new_mask_bits, partition_bits, nosort_bits);
  }
  for (int i = 1; i < partitions; i++) {
    if (indexes[i-1][0] + 1 >= indexes[i][0])
      continue;
    bf2_helper<Key, Value, RandomAccessIterator>
      (begin + indexes[i-1][0], begin + indexes[i][0],
       new_mask_bits, partition_bits, nosort_bits);
  }
}

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf2(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int mask_bits,
      int partition_bits,
      int nosort_bits) {
  int input_num, num_iter, shift, dst_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, mask;
  int new_mask_bits;

  input_num = std::distance(begin, end);

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
  int counters[partitions];

  for (int i = 0; i < partitions + 1; i++)
    counters[i] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h & mask) >> shift]++;
  }
  for (int i = 1; i < partitions; i++) {
    counters[i] += counters[i - 1];
  }
  for (int i = partitions - 1; i != 0; i--) {
    counters[i] = counters[i - 1];
  }
  counters[0] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = counters[(h & mask) >> shift]++;
    std::get<0>(dst[dst_idx]) = h;
    //std::get<1>(dst[dst_idx]) = std::move(iter->first);
    //std::get<2>(dst[dst_idx]) = std::move(iter->second);
    std::get<1>(dst[dst_idx]) = iter->first;
    std::get<2>(dst[dst_idx]) = iter->second;
  }

  new_mask_bits = mask_bits - partition_bits;
  if (new_mask_bits <= nosort_bits)
    return;

  if (counters[0] > 0) {
    bf2_helper<Key, Value, RandomAccessIterator>
      (dst, dst + counters[0], new_mask_bits, partition_bits, nosort_bits);
  }

  for (int i = 1; i < partitions; i++) {
    if (counters[i-1] == counters[i])
      continue;
    bf2_helper<Key, Value, RandomAccessIterator>
      (dst + counters[i-1], dst + counters[i],
       new_mask_bits, partition_bits, nosort_bits);
  }
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
  for (int s = 0; s < partitions; s++) {
    s_begin = super_indexes[s][0];
    s_end = super_indexes[s][1];
    if (s_end - s_begin < 2)
      continue;
    // Partition too small, use insertion sort instead.
    if (s_end - s_begin < sqrt_partitions) {
      //std::cout << "Use insertion sort for "
      //<< s_begin << ", " << s_end << "\n";
      bf3_insertion_outer<RandomAccessIterator>(dst, s_begin, s_end, mask);
      continue;
    }
    // Setup counters for counting sort.
    for (int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (int i = 0; i < partitions - 1; i++) {
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
      tmp_bucket = dst[idx_i];
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
    for (int i = 1; i < partitions; i++) {
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
  int input_num, num_iter, shift, dst_idx;
  int partitions = 1 << partition_bits;
  std::size_t h, mask;
  int new_mask_bits;

  input_num = std::distance(begin, end);

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
  unsigned int counters[partitions];
  unsigned int indexes[partitions][2];

  for (int i = 0; i < partitions + 1; i++)
    counters[i] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h & mask) >> shift]++;
  }

  indexes[0][0] = 0;
  for (int i = 0; i < partitions - 1; i++) {
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

  indexes[0][0] = 0;
  for (int i = 1; i < partitions; i++) {
    indexes[i][0] = indexes[i-1][1];
  }

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
    for (int i = 0; i < partitions; i++)
      counters[i] = 0;
    indexes[0][0] = s_begin;
    for (unsigned int i = s_begin; i < s_end; i++) {
      h = std::get<0>(dst[i]);
      counters[(h & mask) >> shift]++;
    }
    for (int i = 0; i < partitions - 1; i++) {
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
      tmp_bucket = dst[idx_i];
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
    for (int i = 1; i < partitions; i++) {
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
  for (int i = 0; i < partitions; i++)
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

/*
  // single thread approach
  unsigned int counters[partitions];
  for (int i = 0; i < partitions; i++)
    counters[i] = 0;

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[(h&mask)>>shift]++;
  }

  indexes[0].first=0;
  for (int i = 0; i < partitions - 1; i++) {
    indexes[i].second =
      indexes[i+1].first =
      indexes[i].first + counters[i];
  }
  indexes[partitions-1].second = indexes[partitions-1].first +
    counters[partitions-1];

  for (auto iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    auto dst_idx = indexes[(h&mask)>>shift].first++;
    std::get<0>(dst[dst_idx]) = h;
    std::get<0>(dst[dst_idx]) = iter->first;
    std::get<0>(dst[dst_idx]) = iter->second;
  }
  indexes[0].first = 0;
  for (int i = 1; i< partitions; i++) {
    indexes[i].first = indexes[i-1].second;
  }
  */


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

#endif
