#include "radix_hash.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>

TEST(radix_hash_test, simple_test) {
  ASSERT_TRUE(1);
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


TEST(radix_hash_test, custom_iter) {
  std::vector<std::pair<std::string, int>> src;
  std::vector<std::pair<std::string, int>> dst;
  dst.reserve(5);
  src.push_back(std::make_pair("abc", 0));
  src.push_back(std::make_pair("def", 1));
  src.push_back(std::make_pair("ghi", 2));
  src.push_back(std::make_pair("jkl", 3));
  src.push_back(std::make_pair("mno", 4));

  ::sort_hash<std::string>(src.begin(), src.end(), dst.begin(), 5, 1);
  ASSERT_TRUE(true);
}
