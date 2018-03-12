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
  state.SetComplexityN(state.range(0)*2);
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
      //benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
      benchmark::DoNotOptimize(sum++);
    }
  }
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HMJ_breakdown(benchmark::State& state) {
  int size = state.range(0);
  unsigned int cores = std::thread::hardware_concurrency();
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
    //std::vector<std::tuple<std::size_t, std::string, uint64_t>>r_sorted(size);
    //std::vector<std::tuple<std::size_t, std::string, uint64_t>>s_sorted(size);
  //std::vector<std::tuple<std::size_t, std::string, uint64_t>>s_sorted;
  for (auto _ : state) {
    auto r_sorted = new std::tuple<std::size_t, std::string, uint64_t>[size];
    auto s_sorted = new std::tuple<std::size_t, std::string, uint64_t>[size];
    ::radix_hash_bf6<std::string, uint64_t>
        (r.begin(), r.end(), r_sorted, 11, 0, cores);
    ::radix_hash_bf6<std::string, uint64_t>
        (s.begin(), s.end(), s_sorted, 11, 0, cores);

    uint64_t sum = 0;
    for (int i = 0; i < size; i++) {
      benchmark::DoNotOptimize(sum+=std::get<2>(r_sorted[i]));
      benchmark::DoNotOptimize(sum+=std::get<2>(s_sorted[i]));
    }
    delete[] r_sorted;
    delete[] s_sorted;
  }
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HashMergeJoin2(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  int counter;
  for (auto _ : state) {
    auto hmj = HashMergeJoin2<KeyValVec::iterator,
         KeyValVec::iterator>(r.begin(), r.end(),
             s.begin(), s.end());
    sum = 0;
    //benchmark::DoNotOptimize(hmj.begin());
    for (auto tuple : hmj) {
      //benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
      benchmark::DoNotOptimize(sum++);
    }
  }
  state.SetComplexityN(state.range(0)*2);
}

BENCHMARK(BM_hash_join_raw)->RangeMultiplier(2)
->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin)->RangeMultiplier(2)
->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HMJ_breakdown)->RangeMultiplier(2)
->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin2)->RangeMultiplier(2)
->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();

BENCHMARK_MAIN();
