#include "radix_hash.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};


TEST(sort_hash_test, no_sort) {
  ASSERT_TRUE(1);
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 3, 3);
  EXPECT_EQ(4, dst[0].first);
  EXPECT_EQ(3, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(1, dst[3].first);
  EXPECT_EQ(0, dst[4].first);
}

TEST(sort_hash_test, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 3, 0);
  EXPECT_EQ(0, dst[0].first);
  EXPECT_EQ(1, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(3, dst[3].first);
  EXPECT_EQ(4, dst[4].first);
}

TEST(sort_hash_test, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 1, 0);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(2, dst[1].first);
  EXPECT_EQ(3, dst[2].first);
  EXPECT_EQ(4, dst[3].first);
  EXPECT_EQ(5, dst[4].first);
}

TEST(sort_hash_test, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 2, 0);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(2, dst[1].first);
  EXPECT_EQ(3, dst[2].first);
  EXPECT_EQ(4, dst[3].first);
  EXPECT_EQ(5, dst[4].first);
}

TEST(sort_hash_test, multi_pass_nosort_last) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 1, 1);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(3, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(5, dst[3].first);
  EXPECT_EQ(4, dst[4].first);
}

TEST(sort_hash_test, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     12345, 1, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, dst[i].first);
  }
}

TEST(sort_hash_test, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     12345, 5, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, dst[i].first);
  }
}

// radix_hash version 2
TEST(radix_hash_test, no_sort) {
  ASSERT_TRUE(1);
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 3, 3);
  EXPECT_EQ(4, dst[0].first);
  EXPECT_EQ(3, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(1, dst[3].first);
  EXPECT_EQ(0, dst[4].first);
}

TEST(radix_hash_test, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 3, 0);
  EXPECT_EQ(0, dst[0].first);
  EXPECT_EQ(1, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(3, dst[3].first);
  EXPECT_EQ(4, dst[4].first);
}

TEST(radix_hash_test, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 1, 0);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(2, dst[1].first);
  EXPECT_EQ(3, dst[2].first);
  EXPECT_EQ(4, dst[3].first);
  EXPECT_EQ(5, dst[4].first);
}

TEST(radix_hash_test, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 2, 0);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(2, dst[1].first);
  EXPECT_EQ(3, dst[2].first);
  EXPECT_EQ(4, dst[3].first);
  EXPECT_EQ(5, dst[4].first);
}

TEST(radix_hash_test, multi_pass_nosort_last) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 1, 1);
  EXPECT_EQ(1, dst[0].first);
  EXPECT_EQ(3, dst[1].first);
  EXPECT_EQ(2, dst[2].first);
  EXPECT_EQ(5, dst[3].first);
  EXPECT_EQ(4, dst[4].first);
}

TEST(radix_hash_test, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     12345, 1, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, dst[i].first);
  }
}

TEST(radix_hash_test, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  dst.reserve(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     12345, 5, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, dst[i].first);
  }
}
