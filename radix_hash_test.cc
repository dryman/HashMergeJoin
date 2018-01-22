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


TEST(radix_hash_test, no_sort) {
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

TEST(radix_hash_test, full_sort) {
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

/*
TEST(radix_hash_test, pair_test) {
  std::pair<int, int[]> mypair = std::make_pair(2, new int[3]);
  mypair.second[1] = 2;
  ASSERT_EQ(mypair.second[1], 2);
}
0, [0, 0, 0]
1, [0, 0, 0]
2, [0, 0, 0], 2, [0, 0, 1], ...

recursion is probably easier?
Every recursion should be able to be rewritten as loop

exit cond:
2, [n-1, n-1, n-1]
or
0, [n, .., ..]

if data[i] == n {
  if i == 0 => exit
  data[i] = 0;
  data[i - 1] = 0;
  i--;
  continue;
}
if i < i_max - 1 {
  process data[i] -> data[i+1]
  i++;
  continue;
}
process data[i-1] -> data[i];
data[i]++;
*/


