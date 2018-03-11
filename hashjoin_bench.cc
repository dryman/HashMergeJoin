#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
#include "radix_hash.h"
#include "hashjoin.h"
#include "strgen.h"
#include <assert.h>


static void BM_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    std::unordered_map<std::string, uint64_t> s_map;
    s_map.reserve(s.size());
    uint64_t sum = 0;
    for (auto s_kv : s) {
      s_map[s_kv.first] = s_kv.second;
    }
    for (auto r_kv : r) {
      benchmark::DoNotOptimize
        (sum += r_kv.second + s_map[r_kv.first]);
    }
  }
  state.SetComplexityN(state.range(0));
}

static void BM_HashMergeJoin(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  int counter;
  for (auto _ : state) {
    auto hmj = HashMergeJoin<KeyValVec::iterator,
                             KeyValVec::iterator>(r.begin(), r.end(),
                                                  s.begin(), s.end());
    sum = 0;
    //benchmark::DoNotOptimize(hmj.begin());
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
  }
  state.SetComplexityN(state.range(0));
}


BENCHMARK(BM_hash_join_raw)->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin)->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();

BENCHMARK_MAIN();
