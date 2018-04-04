/*
 * Copyright 2018 Felix Chern
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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

TEST(radix_sort_2_test, random_num) {
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

  ::radix_sort_2<std::size_t,int>(input.begin(), size, 11, cores);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(std::get<0>(std_sorted[i]), std::get<0>(input[i]));
  }
}
