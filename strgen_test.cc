#include "strgen.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <string>

TEST(strgen_test, uniqueness) {
  int size = 1<<18;
  auto random_pairs = ::create_strvec(size);
  std::unordered_set<std::string> unique_strings;
  for (auto pair : random_pairs) {
    //std::cout << pair.first << "\n";
    unique_strings.insert(pair.first);
  }
  ASSERT_EQ(size, unique_strings.size());
}
