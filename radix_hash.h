#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <functional>

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

  /*
  if ((input_num & (input_num - 1)) == 0) {
    input_power = input_num;
  } else {
    // TODO we may reduce one bit from input_power
    input_power = 64 - __builtin_clzll(input_num);
  }
  */
    input_power = 63 - __builtin_clzll(input_num);
  std::cout << "nosort_power: " << nosort_power << "\n";

  if (input_power <= nosort_power) {
    for (auto iter = begin; iter != end; iter++) {
      *dst++ = *iter;
      // TODO export_hash variant
    }
    return;
  }

  num_iter = (input_power - nosort_power + partition_power - 1)
  / partition_power;
  //nosort_power = input_power - num_iter * partition_power;
  mask = (1ULL << (input_power + 1)) - 1;
  shift = nosort_power + (num_iter - 1) * partition_power;

  std::cout << "num_iter: " << num_iter << "\n";
  std::cout << "input_power: " << input_power << "\n";
  std::cout << "nosort_power: " << nosort_power << "\n";
  std::cout << "mask: " << std::hex << mask << "\n";
  std::cout << "shift: " << std::hex << shift << "\n";

  int counters[partitions];
  int counter_stack[num_iter][partitions];
  int iter_stack[num_iter];

  for (int i = 0; i < partitions; i++)
    counters[i] = 0;
  for (int i = 0; i < num_iter; i++)
    for (int j = 0; j < partitions; j++)
      counter_stack[i][j] = 0;
  for (int i = 0; i < num_iter; i++)
    iter_stack[i] = 0;

  std::vector<std::vector<std::tuple<std::size_t, Key, Value>>> buffers;
  buffers.reserve(num_iter);
  for (int i = 0; i < num_iter; i++) {
    buffers[i].reserve(input_num);
  }
  /*
  buffers[0].reserve(input_num);
  buffers[1].reserve(input_num);
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
  for (int i = 0; i < partitions; i++)
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

  for (int i = 0; i < input_num; i++) {
    std::cout << "{"
              << std::get<0>(buffers[0][i]) << ", "
              << std::get<1>(buffers[0][i]) << ", "
              << std::get<2>(buffers[0][i]) << "},  ";
  }
  std::cout << "\n";

  for (int i = 0; true;) {
    std::cout << "i: " << i;
    std::cout << " num_iter: " << num_iter;
    std::cout << " iter_stack[i]: " << iter_stack[i] << "\n";
    if (iter_stack[i] == input_num - 1) {
      if (i == 0) return;
      iter_stack[i] = 0;
      iter_stack[i - 1]++;
      i--;
      std::cout << "reaching early cont.\n";
      continue;
    }
    mask = (1ULL << (input_power - i * partition_power)) - 1;
    shift = nosort_power + (num_iter - i - 1) * partition_power;
    std::cout << "input_power: " << input_power << " ";
    std::cout << "partition_power: " << partition_power << " ";
    std::cout << "mask: " << mask << " shift: " << shift << "\n";

    for (int j = 0; j < num_iter; j++) {
      for (int k = 0; k < input_num; k++) {
        std::cout << "{"
                  << std::get<0>(buffers[j][k]) << ", "
                  << std::get<1>(buffers[j][k]) << ", "
                  << std::get<2>(buffers[j][k]) << "},  ";
      }
      std::cout << "\n";
    }

    // clear counters
    for (int j = 0; j < partitions; j++)
      counters[j] = 0;
    // accumulate counters
    for (int j = counter_stack[i][iter_stack[i]];
         j < counter_stack[i][iter_stack[i] + 1];
         j++) {
      //h = std::get<0>(buffers[i & 1][j]);
      h = std::get<0>(buffers[i][j]);
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
        counter_stack[i + 1][j] = counters[j];
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        //h = std::get<0>(buffers[i & 1][j]);
        h = std::get<0>(buffers[i][j]);
        buffers[i + 1][counters[(h & mask) >> shift]++] = buffers[i][j];
        /*
        buffers[(i + 1) & 1][counters[(h & mask) >> shift]++] =
          buffers[i & 1][j];
        */
      }
      i++;
    } else {
      std::cout << "tip iter\n";
      for (int j = counter_stack[i][iter_stack[i]];
           j < counter_stack[i][iter_stack[i] + 1];
           j++) {
        //h = std::get<0>(buffers[i & 1][j]);
        h = std::get<0>(buffers[i][j]);
        dst_idx = counters[(h & mask) >> shift]++;
        //std::get<0>(dst[dst_idx]) = std::get<1>(buffers[i & 1][j]);
        //std::get<1>(dst[dst_idx]) = std::get<2>(buffers[i & 1][j]);
        std::get<0>(dst[dst_idx]) = std::get<1>(buffers[i][j]);
        std::get<1>(dst[dst_idx]) = std::get<2>(buffers[i][j]);
      }
      iter_stack[i]++;
    }
  }
}

#endif
