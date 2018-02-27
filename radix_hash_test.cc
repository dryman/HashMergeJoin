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

TEST(radix_hash_df1_test, no_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 3);
  EXPECT_EQ(4, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(1, std::get<0>(dst[3]));
  EXPECT_EQ(0, std::get<0>(dst[4]));
}

TEST(radix_hash_df1_test, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 0);
  EXPECT_EQ(0, std::get<0>(dst[0]));
  EXPECT_EQ(1, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(3, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_df1_test, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 1, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_df1_test, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 2, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_df1_test, multi_pass_nosort_last) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     5, 1, 1);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(5, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_df1_test, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 1, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_hash_df1_test, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 5, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

// radix_hash version 2
TEST(radix_hash_df2_test, no_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 3);
  EXPECT_EQ(4, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(1, std::get<0>(dst[3]));
  EXPECT_EQ(0, std::get<0>(dst[4]));
}

TEST(radix_hash_df2_test, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 0);
  EXPECT_EQ(0, std::get<0>(dst[0]));
  EXPECT_EQ(1, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(3, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_df2_test, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 1, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_df2_test, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 2, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_df2_test, multi_pass_nosort_last) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 1, 1);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(5, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_df2_test, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 1, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_hash_df2_test, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_df2<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 5, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

// bf1 test
TEST(radix_hash_bf1_test, no_sort) {
  ASSERT_TRUE(1);
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 3);
  EXPECT_EQ(4, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(1, std::get<0>(dst[3]));
  EXPECT_EQ(0, std::get<0>(dst[4]));
}

TEST(radix_hash_bf1_test, full_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 4; i > -1; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 3, 0);
  EXPECT_EQ(0, std::get<0>(dst[0]));
  EXPECT_EQ(1, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(3, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_bf1_test, multi_pass_sort) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 1, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_bf1_test, multi_pass_sort2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 2, 0);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(2, std::get<0>(dst[1]));
  EXPECT_EQ(3, std::get<0>(dst[2]));
  EXPECT_EQ(4, std::get<0>(dst[3]));
  EXPECT_EQ(5, std::get<0>(dst[4]));
}

TEST(radix_hash_bf1_test, multi_pass_nosort_last) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(5);
  for (int i = 5; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     3, 1, 1);
  EXPECT_EQ(1, std::get<0>(dst[0]));
  EXPECT_EQ(3, std::get<0>(dst[1]));
  EXPECT_EQ(2, std::get<0>(dst[2]));
  EXPECT_EQ(5, std::get<0>(dst[3]));
  EXPECT_EQ(4, std::get<0>(dst[4]));
}

TEST(radix_hash_bf1_test, multi_pass_large_num) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 1, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_hash_bf1_test, multi_pass_large_num2) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(12345);
  for (int i = 12345; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     14, 5, 0);
  for (int i = 0; i < 12345; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}

TEST(radix_hash_bf1_test, multi_pass_large_num3) {
  int size = 1 << 18;
  std::vector<std::pair<int, int>> src;
  std::vector<std::tuple<std::size_t, int, int>> dst(size);
  for (int i = size; i > 0; i--) {
    src.push_back(std::make_pair(i, i));
  }
  for (int i = 0; i < 20; i++) {
  ::radix_hash_bf1<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                     19, 11, 0);
  }
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(i + 1, std::get<0>(dst[i]));
  }
}
