#include "radix_sort.h"
#include "gtest/gtest.h"
#include <vector>
#include <random>

bool pair_cmp (std::pair<std::size_t, int> a,
               std::pair<std::size_t, int> b) {
  return std::get<0>(a) < std::get<0>(b);
}

TEST(radix_sort_1_test, random_num) {
  int size = 1<<16;
  std::vector<std::pair<std::size_t, int>> input;
  std::vector<std::pair<std::size_t, int>> std_sorted;
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  unsigned int cores = std::thread::hardware_concurrency();

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }
  std_sorted = input;
  std::sort(std_sorted.begin(), std_sorted.end(), pair_cmp);

  ::radix_sort_1<std::size_t,int>(input.begin(), size, 11, cores);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(std::get<0>(std_sorted[i]), std::get<0>(input[i]));
  }
}
