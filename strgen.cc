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

#include <fstream>
#include <iostream>
#include <string>
#include <math.h>
//#include <benchmark/benchmark.h>
#include <algorithm>
#include <assert.h>

#include "strgen.h"

std::vector<std::pair<std::string, uint64_t>>
create_strvec(const int number) {
  std::vector<std::pair<std::string, uint64_t>> pairs;
  pairs.reserve(number);
  int sqrt_num = (int)ceil(sqrt((double)number));

  std::fstream fs;
  fs.open("/usr/share/dict/words", std::fstream::in);

  // TODO
  // To get better cache performance, we should consider
  // chop the string so that each string fit in 22 characters.
  for (int i = 0; i < sqrt_num; i++) {
    std::pair<std::string, uint64_t> p;
    p.second = i;
    getline(fs, p.first);
    pairs.push_back(p);
  }
  fs.close();

  for (int i = 0; i < sqrt_num; i++) {
    for (int j = 0; j < sqrt_num; j++) {
      if (pairs.size() == number) {
        //std::cout << "end of strgen\n";
        std::random_shuffle(pairs.begin(), pairs.end());
        return pairs;
      }
      std::pair<std::string, uint64_t> p;
      p.first = pairs[i].first + "-" + pairs[j].first;
      p.second = i + j;
      pairs.push_back(std::move(p));
    }
  }
  assert(0);
}
