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
  fs.open("/usr/shared/dict/words", std::fstream::in);

  // TODO
  // To get better cache performance, we should consider
  // chop the string so that each string fit in 22 characters.
  for (int i = 0; i < sqrt_num; i++) {
    std::pair<std::string, uint64_t> p;
    p.second = i;
    getline(fs, p.first);
    pairs.push_back(p);
  }

  for (int i = 0; i < sqrt_num; i++) {
    for (int j = 0; j < sqrt_num; j++) {
      if (pairs.size() == number) {
        //std::cout << "end of strgen\n";
        std::random_shuffle(pairs.begin(), pairs.end());
        return pairs;
      }
      std::pair<std::string, uint64_t> p;
      p.first = pairs[i].first + pairs[j].first;
      p.second = i + j;
      pairs.push_back(p);
    }
  }
  assert(0);
}

/*
static void BM_genstr(benchmark::State& state) {
  for (auto _ : state) {
    auto v = create_strvec(state.range(0));
    v[0].second = 5;
    assert(v.size() == state.range(0));
  }
}

BENCHMARK(BM_genstr)->Arg(65536)->Arg(1000000)->Arg(4000000);
BENCHMARK_MAIN();
*/
