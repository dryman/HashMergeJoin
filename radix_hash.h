#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

#include <iterator>
#include <utility>
#include <functional>

// TODO: Should benchmark the speed difference between array or vector.
// TODO: use boolean template to create an export_hash variant
// TODO: use integer key and identity hash function for unit testing
template <typename Key,
  typename Value,
  typename BidirectionalIterator,
  typename RandomAccessIterator,
  typename Hash = std::hash<Key>>
  void sort_hash(BidirectionalIterator begin,
                 BidirectionalIterator end,
                 RandomAccessIterator dst,
                 int input_num,
                 int partition_power,
                 int nosort_power) {
  std::size_t h;
  int input_power, dst_idx;
  auto iter = begin;
  int partitions = 1 << partition_power;
  std::size_t mask;

  if (input_num & (input_num - 1) == 0) {
    input_power = input_num;
  } else {
    input_power = 63 - __builtin_clzll(input);
  }

  if (input_power < nosort_power) {
    // TODO simply copy and exit.
    return;
  }

  num_iter = (input_power - nosort_power) / partition_power;
  nosort_power = input_power - num_iter * partition_power;
  mask = 1ULL << input_power;

  int counter_stack[num_iter][partitions] = {};
  int iter_stack[num_iter] = {};

  std::vector<std::tuple<std::size_t, Key, Value>> buffers[2];
  buffers[0].reserve(input_num);
  buffers[1].reserve(input_num);

  for (iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[h & mask]++;
  }
  for (int i = 1; i < partitions; i++) {
    counters[i] += counters[i - 1];
  }
  for (int i = partitions - 1; i != 0; i--) {
    counters[i] = counters[i - 1];
  }
  counters[0] = 0;
  for (int i = 0; i < partitions; i++)
    counter_stack[0][i] = counters;

  for (iter = begin; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    dst_idx = counters[h & mask]++; // make this a template inline function
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
    if (iter_stack[i] == input_num - 1) {
      if (i == 0) return;
      iter_stack[i] = 0;
      iter_stack[i - 1]++;
      i--;
      continue;
    }
    mask = 1ULL << (input_power - (i + 1) * partition_power);
    shift = nosort_power + (num_iter - i - 1) * partition_power;

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
    for (int j = partitions - 1; j != 0; j--) {
      counters[j] = counters[j - 1] + prev_idx;;
    }
    counters[0] = prev_idx;

    if (i < num_iter - 1) {
      for (int j = 0; j < partitions; j++)
        counter_stack[i + 1][j] = counters;
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
        dst[counters[(h & mask) >> shift]++] = buffers[i & 1][j];
        iter_stack[i]++;
      }
    }
  }
}

#endif
