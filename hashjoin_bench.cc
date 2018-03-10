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
    for (auto s_kv : s) {
      s_map[s_kv.first] = s_kv.second;
    }
    for (auto r_kv : r) {
      benchmark::DoNotOptimize(s_map[r_kv.first]);
    }
  }
  state.SetComplexityN(state.range(0));
}

static void BM_hash_join_raw_orderd(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::unordered_map<std::string, uint64_t> r_map;
  r_map.reserve(r.size());
  for (auto r_kv : r) {
    r_map[r_kv.first] = r_kv.second;
  }
  for (auto _ : state) {
    std::unordered_map<std::string, uint64_t> s_map;
    s_map.reserve(s.size());
    for (auto s_kv : s) {
      s_map[s_kv.first] = s_kv.second;
    }

    for (auto r_it = r_map.begin(); r_it != r_map.end(); ++r_it) {
      benchmark::DoNotOptimize(s_map[r_it->first]);
    }
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  auto r_sorted = HashKeyValVec(r.size());
  auto s_sorted = HashKeyValVec(size);

  for (auto _ : state) {
    ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(),
                                            r_sorted.begin(),
                                            ::compute_power(size), 11, 0);
    ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(),
                                            s_sorted.begin(),
                                            ::compute_power(s.size()), 11, 0);
    for (auto r_it = r_sorted.begin(),
           s_it = s_sorted.begin();
         r_it != r_sorted.end() && s_it != s_sorted.end();) {
      //benchmark::DoNotOptimize(std::get<1>(*r_it) ==
      //                       std::get<1>(*s_it));
      r_it++;
      s_it++;
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
    //std::cout << "hmj built\n";
    auto hmj = HashMergeJoin<KeyValVec::iterator,
                             KeyValVec::iterator>(r.begin(), r.end(),
                                                  s.begin(), s.end());
    counter = 0;
    for (auto tuple : hmj) {
      sum += *std::get<1>(tuple) + *std::get<2>(tuple);
      counter++;
      //std::cout << "counter: " << counter << "\n";
      //sum += *std::get<1>(tuple);
    }
    std::cout << "counter: " << counter << "\n";
    //assert(counter == size);
  }
  state.SetComplexityN(state.range(0));
}

/*
static void BM_HashMergeJoinSimpleThread(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    auto hmj = HashMergeJoinSimpleThread<KeyValVec::iterator,
                             KeyValVec::iterator>
      (r.begin(), r.end(), s.begin(), s.end());
    for (auto tuple : hmj) {
      sum += *std::get<1>(tuple) + *std::get<2>(tuple);
    }
  }
  state.SetComplexityN(state.range(0));
}
*/

BENCHMARK(BM_hash_join_raw)->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin)->Range(1<<18, 1<<23)->Complexity(benchmark::oN)
->UseRealTime();
//BENCHMARK(BM_HashMergeJoinSimpleThread)->
//Range(1<<18, 1<<23)->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK_MAIN();
