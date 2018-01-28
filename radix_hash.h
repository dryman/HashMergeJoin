#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <functional>

// namespace radix_hash?

/*
  need to benchmark how to get good template setup.
template<typename RandomAccessIterator, typename TupleVector>
void export_hash_tuple(RansomAccessIterator dst, int dst_idx,
                       TupleVector& buffer) {
}

template<typename RandomAccessIterator>
void export_kv_pair () {
}
*/

// TODO: Should benchmark the speed difference between array or vector.
// TODO: use boolean template to create an export_hash variant
// TODO: use integer key and identity hash function for unit testing
template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void sort_hash(BidirectionalIterator begin,
                 BidirectionalIterator end,
                 RandomAccessIterator dst,
                 int input_num,
                 int partition_power,
                 int nosort_power) {
  int input_power, num_iter, shift, dst_idx, prev_idx;
  int partitions = 1 << partition_power;
  std::size_t h, mask;

  // invariant: (1 << input_power) - 1 can hold all values.
  input_power = 64 - __builtin_clzll(input_num);

  if (input_power <= nosort_power) {
    for (auto iter = begin; iter != end; iter++) {
      *dst++ = *iter;
      // TODO export_hash variant
    }
    return;
  }

  num_iter = (input_power - nosort_power
              + partition_power - 1) / partition_power;
  mask = (1ULL << input_power) - 1;
  shift = input_power - partition_power;
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

  std::vector<std::tuple<std::size_t, Key, Value>> buffers[2];
  buffers[0].reserve(input_num);
  buffers[1].reserve(input_num);

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
      dst[dst_idx] = *iter;
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
    mask = (1ULL << (input_power - partition_power * (i + 1))) - 1;
    shift = input_power - partition_power * (i + 2);
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
        std::get<0>(dst[dst_idx]) = std::get<1>(buffers[i & 1][j]);
        std::get<1>(dst[dst_idx]) = std::get<2>(buffers[i & 1][j]);
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
  void radix_hash(BidirectionalIterator begin,
                  BidirectionalIterator end,
                  RandomAccessIterator dst,
                  int input_num,
                  int partition_power,
                  int nosort_power) {
  int input_power, num_iter, shift, dst_idx, prev_idx;
  int partitions = 1 << partition_power;
  std::size_t h, mask;

  // invariant: (1 << input_power) - 1 can hold all values.
  input_power = 64 - __builtin_clzll(input_num);

  if (input_power <= nosort_power) {
    for (auto iter = begin; iter != end; iter++) {
      *dst++ = *iter;
      // TODO export_hash variant
    }
    return;
  }

  num_iter = (input_power - nosort_power
              + partition_power - 1) / partition_power;
  mask = (1ULL << input_power) - 1;
  shift = input_power - partition_power;
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

  std::vector<std::tuple<std::size_t, Key, Value>> buffers[2];
  buffers[0].reserve(input_num);
  buffers[1].reserve(input_num);

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
      std::get<0>(dst[dst_idx]) = std::get<1>(buffers[1][i]);
      std::get<1>(dst[dst_idx]) = std::get<2>(buffers[1][i]);
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
    mask = (1ULL << (input_power - partition_power * (i + 1))) - 1;
    shift = input_power - partition_power * (i + 2);
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
        std::get<0>(dst[dst_idx]) = std::get<1>(buffers[i & 1][j]);
        std::get<1>(dst[dst_idx]) = std::get<2>(buffers[i & 1][j]);
      }
      iter_stack[i]++;
    }
  }
}

#endif
