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
