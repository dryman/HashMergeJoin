#include "partitioned_hash.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <random>

struct identity_hash
{
  std::size_t operator()(const uint64_t& k) const {
    return k;
  }
};

TEST(partitioned_hash_test, partition_only_test) {
  std::vector<std::pair<uint64_t, uint64_t>> src;
  std::vector<std::vector<std::pair<uint64_t, uint64_t>>> dst(2);
  uint64_t top_bit = 1ULL << 63;
  for (uint64_t i = 0; i < 5; i++) {
    src.push_back(std::make_tuple(i,i));
    src.push_back(std::make_tuple(i|top_bit,i|top_bit));
    src.push_back(std::make_tuple(i|top_bit|1024,i|top_bit|1024));
  }
  radix_hash::partition_only<
   decltype(src.begin()),
   std::pair<uint64_t, uint64_t>, uint64_t, uint64_t,
   identity_hash>(src.begin(), src.end(), &dst, 2, 1);
  EXPECT_EQ(5, dst[0].size());
  EXPECT_EQ(10, dst[1].size());
}

TEST(partitioned_hash_test, partition_table_test) {
  std::vector<std::pair<uint64_t, uint64_t>> src;
  std::vector<std::unordered_map<uint64_t, uint64_t>> dst(2);
  uint64_t top_bit = 1ULL << 63;
  for (uint64_t i = 0; i < 5; i++) {
    src.push_back(std::make_tuple(i,i));
    src.push_back(std::make_tuple(i|top_bit,i|top_bit));
    src.push_back(std::make_tuple(i|top_bit|1024,i|top_bit|1024));
  }
  radix_hash::partitioned_hash_table<
   decltype(src.begin()),
   std::pair<uint64_t, uint64_t>, uint64_t, uint64_t,
   identity_hash>(src.begin(), src.end(), &dst, 1, 1);
  EXPECT_EQ(5, dst[0].size());
  EXPECT_EQ(10, dst[1].size());
}
