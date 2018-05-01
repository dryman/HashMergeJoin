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

#include "radix_hash.h"
#include "gtest/gtest.h"
#include <vector>
#include <string>
#include <random>

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};

bool tuple_cmp (std::tuple<std::size_t, int, int> a,
                std::tuple<std::size_t, int, int> b) {
  return std::get<0>(a) < std::get<0>(b);
}

bool str_tuple_cmp (std::tuple<std::size_t, std::string, uint64_t> a,
                    std::tuple<std::size_t, std::string, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

TEST(radix_non_inplace_par, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           3, 0, 2);
  EXPECT_EQ(0, std::get<0>(dst[0]));
  EXPECT_EQ(1, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(3, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_non_inplace_par, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           1, 0, 2);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_non_inplace_par, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           2, 0, 2);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_non_inplace_par, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  unsigned int cores = std::thread::hardware_concurrency();
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           1, 0, cores);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_non_inplace_par, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  unsigned int cores = std::thread::hardware_concurrency();
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           5, 0, cores);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_non_inplace_par, multi_pass_large_num3) {
  int size = 1 << 18;
  std::vector<std::pair<int, int>> src(size);
  std::vector<std::tuple<std::size_t, int, int>> dst(size);
  unsigned int cores = std::thread::hardware_concurrency();
  for (int i = size; i > 0; i--) {
    src[size-i] = std::make_pair(i, i);
  }
  radix_hash::radix_non_inplace_par<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                                           10, 0, cores);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_inplace_seq_test, full_sort) {
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = 4; i > -1; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), 5, 3);
  EXPECT_EQ(0, std::get<0>(dst[0]));
  EXPECT_EQ(1, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(3, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_inplace_seq_test, multi_pass_sort) {
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = 5; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), 5, 1);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_inplace_seq_test, multi_pass_sort2) {
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = 5; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), 5, 2);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_inplace_seq_test, multi_pass_large_num) {
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = 12345; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), 12345, 1);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_inplace_seq_test, multi_pass_large_num2) {
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = 12345; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), 12345, 5);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_inplace_seq_test, multi_pass_large_num3) {
  int size = 1 << 18;
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = size; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_seq<int,int>(dst.begin(), size, 10);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_inplace_seq_test, random_num) {
  int size = 1 << 16;
  std::vector<std::tuple<std::size_t, int, int>> input;
  std::vector<std::tuple<std::size_t, int, int>> std_sorted;
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_tuple(r, i, i));
  }
  std_sorted = input;
  std::sort(std_sorted.begin(), std_sorted.end(), tuple_cmp);

  radix_hash::radix_inplace_seq<int,int>(input.begin(), size, 10);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(std::get<0>(std_sorted[i]), std::get<0>(input[i]));
  }
}


TEST(radix_inplace_par_test, multi_pass_large_num3) {
  int size = 1 << 18;
  std::vector<std::tuple<std::size_t, int, int>> dst;
  for (int i = size; i > 0; i--) {
    dst.push_back(std::make_tuple((std::size_t)i, i, i));
  }
  radix_hash::radix_inplace_par(dst.begin(), size, 10, 4);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_inplace_par_test, random_num) {
  int size = 1<<16;
  std::vector<std::tuple<std::size_t, int, int>> input;
  std::vector<std::tuple<std::size_t, int, int>> std_sorted;
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  unsigned int cores = std::thread::hardware_concurrency();
  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_tuple(r, i, i));
  }
  std_sorted = input;
  std::sort(std_sorted.begin(), std_sorted.end(), tuple_cmp);

  radix_hash::radix_inplace_par(input.begin(), size, 11, cores);
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(std::get<0>(std_sorted[i]), std::get<0>(input[i]));
  }
}
