#include <iterator>
#include <utility>
#include <vector>
#include <string>
#include "gtest/gtest.h"

template<typename IIter, typename OIter>
void mycopy (IIter iter_begin, IIter iter_end, OIter dst) {
  std::size_t size = std::distance(iter_begin, iter_end);
  std::unique_ptr<std::string[]>buffers[1];
  buffers[0] = std::make_unique<std::string[]>(size);

  int i = 0;
  for (auto x = iter_begin; x != iter_end; ++x) {
    buffers[0][i++] = *x;
  }
  for (i = 0; i < size; i++) {
    *dst = buffers[0][i];
    ++dst;
  }
}

TEST(iter_cpy_test, mycopy) {
  std::vector<std::string> src;
  std::vector<std::string> dst(3);
  src.push_back("abc");
  src.push_back("def");
  src.push_back("ghi");
  mycopy(src.begin(), src.end(), dst.begin());
  EXPECT_EQ("abc", dst[0]);
  EXPECT_EQ("def", dst[1]);
  EXPECT_EQ("ghi", dst[2]);
  ASSERT_TRUE(true);
}
