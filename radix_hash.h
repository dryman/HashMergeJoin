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
  void radix_hash_df1(BidirectionalIterator begin,
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

  /*
  std::vector<std::tuple<std::size_t, Key, Value>> buffers[2];
  //buffers[0].reserve(input_num);
  //buffers[1].reserve(input_num);
  // we need to initialize array else we'll get segmentation fault.
  // Yet, using vector initialization makes our benchmark slow..
  buffers[0] = std::vector<std::tuple<std::size_t, Key, Value>>(input_num);
  buffers[1] = std::vector<std::tuple<std::size_t, Key, Value>>(input_num);
  */
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
  void radix_hash_df2(BidirectionalIterator begin,
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

  // std::vector<std::tuple<std::size_t, Key, Value>> buffers[2];
  // buffers[0] = std::vector<std::tuple<std::size_t, Key, Value>>(input_num);
  // buffers[1] = std::vector<std::tuple<std::size_t, Key, Value>>(input_num);
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

template <typename Key,
  typename Value,
  typename Hash = std::hash<Key>,
  typename BidirectionalIterator,
  typename RandomAccessIterator>
  void radix_hash_bf1(BidirectionalIterator begin,
      BidirectionalIterator end,
      RandomAccessIterator dst,
      int input_num,
      int partition_power,
      int nosort_power) {
  int input_power, num_iter, iter, shift, shift1, shift2, dst_idx, anchor_idx;
  int partitions = 1 << partition_power;
  std::size_t h, h1, h2, mask, mask1, mask2, anchor_h1;

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

  for (int i = 0; i < partitions + 1; i++)
    counters[i] = 0;

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

  if (num_iter == 1) {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      // TODO make this a template inline function
      dst_idx = counters[(h & mask) >> shift]++;
      dst[dst_idx] = *iter;
    }
    return;
  } else {
    for (auto iter = begin; iter != end; ++iter) {
      h = Hash{}(std::get<0>(*iter));
      // TODO make this a template inline function
      dst_idx = counters[(h & mask) >> shift]++;
      std::get<0>(buffers[0][dst_idx]) = h;
      std::get<1>(buffers[0][dst_idx]) = iter->first;
      std::get<2>(buffers[0][dst_idx]) = iter->second;
    }
  }

  for (iter = 0; iter < num_iter - 2; iter++) {
    mask1 = (1ULL << (input_power - partition_power * iter)) - 1;
    shift1 = input_power - partition_power * (iter + 1);
    shift1 = shift1 < 0 ? 0 : shift1;
    mask2 = (1ULL << (input_power - partition_power * (iter + 1))) - 1;
    shift2 = input_power - partition_power * (iter + 2);
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
        for (int k = 1; k < partitions; k++) {
          counters[k] += counters[k - 1];
        }
        for (int k = partitions; k != 0; k--) {
          counters[k] = counters[k - 1] + anchor_idx;;
        }
        counters[0] = anchor_idx;
        for (int k = anchor_idx; k < j; k++) {
          h = std::get<0>(buffers[iter & 1][k]);
          buffers[(iter + 1) & 1][counters[(h & mask2) >> shift2]++] =
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
    for (int k = 1; k < partitions; k++) {
      counters[k] += counters[k - 1];
    }
    for (int k = partitions; k != 0; k--) {
      counters[k] = counters[k - 1] + anchor_idx;;
    }
    counters[0] = anchor_idx;
    for (int k = anchor_idx; k < input_num; k++) {
      h = std::get<0>(buffers[iter & 1][k]);
      buffers[(iter + 1) & 1][counters[(h & mask2) >> shift2]++] =
        buffers[iter & 1][k];
    }
  }

  // flush the last buffer to dst.
  mask1 = (1ULL << (input_power - partition_power * iter)) - 1;
  shift1 = input_power - partition_power * (iter + 1);
  shift1 = shift1 < 0 ? 0 : shift1;
  mask2 = (1ULL << (input_power - partition_power * (iter + 1))) - 1;
  shift2 = input_power - partition_power * (iter + 2);
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
      for (int k = 1; k < partitions; k++) {
        counters[k] += counters[k - 1];
      }
      for (int k = partitions; k != 0; k--) {
        counters[k] = counters[k - 1] + anchor_idx;;
      }
      counters[0] = anchor_idx;
      for (int k = anchor_idx; k < j; k++) {
        h = std::get<0>(buffers[iter & 1][k]);
        dst_idx = counters[(h & mask2) >> shift2]++;
        std::get<0>(dst[dst_idx]) = std::get<1>(buffers[iter & 1][k]);
        std::get<1>(dst[dst_idx]) = std::get<2>(buffers[iter & 1][k]);
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
  for (int k = 1; k < partitions; k++) {
    counters[k] += counters[k - 1];
  }
  for (int k = partitions; k != 0; k--) {
    counters[k] = counters[k - 1] + anchor_idx;;
  }
  counters[0] = anchor_idx;
  for (int k = anchor_idx; k < input_num; k++) {
    h = std::get<0>(buffers[iter & 1][k]);
    dst_idx = counters[(h & mask2) >> shift2]++;
    std::get<0>(dst[dst_idx]) = std::get<1>(buffers[iter & 1][k]);
    std::get<1>(dst[dst_idx]) = std::get<2>(buffers[iter & 1][k]);
  }
}

#endif
