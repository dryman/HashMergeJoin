#ifndef RADIX_HASH_H
#define RADIX_HASH_H 1

#include <iterator>
#include <utility>
#include <functional>

// Should benchmark the speed difference between array or vector.
template <typename Key,
  typename BidirectionalIterator,
  typename RandomAccessIterator,
  typename Hash = std::hash<Key>>
  void sort_hash(BidirectionalIterator begin,
                 BidirectionalIterator end,
                 RandomAccessIterator dst,
                 int partition_power) {
  int counters[1 << partition_power];
  std::size_t mask = (1ULL << partition_power) - 1;

  auto iter = begin;
  std::size_t h;

  for (; iter != end; ++iter) {
    h = Hash{}(std::get<0>(*iter));
    counters[h & mask]++;
  }
  for (int i = 1; i < 1 << partition_power; i++) {
    counters[i] += counters[i - 1];
  }
  iter--;
  for (; iter != begin; --iter) {
    h = Hash{}(std::get<0>(*iter));
    dst[counters[h & mask]++] = *iter;
  }
  h = Hash{}(std::get<0>(*iter));
  dst[counters[h & mask]++] = *iter;
}

#endif
