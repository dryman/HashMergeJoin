#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
#include "radix_hash.h"
#include "hashjoin.h"
#include "strgen.h"

static void BM_single_table_join(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::single_table_join(r, s, [](std::string k,
                                 uint64_t r_val, uint64_t s_val) {
                          return;
                        });
  }
}

static void BM_radix_hash_table_join(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::radix_hash_table_join(r, s, [](std::string k,
                                     uint64_t r_val, uint64_t s_val) {
                              return;
                            });
  }
}

static void BM_radix_hash_join(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::radix_hash_join(r, s, [](std::string k,
                               uint64_t r_val, uint64_t s_val) {
                        return;
                      });
  }
}

static void BM_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::unordered_map<std::string, uint64_t> s_map;
  s_map.reserve(s.size());
  for (auto s_kv : s) {
    s_map[s_kv.first] = s_kv.second;
  }
  for (auto _ : state) {
    for (auto r_kv : r) {
      benchmark::DoNotOptimize(s_map[r_kv.first]);
    }
  }
}

static void BM_radix_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::unordered_map<std::string, uint64_t> s_map;
  s_map.reserve(s.size());
  for (auto s_kv : s) {
    s_map[s_kv.first] = s_kv.second;
  }
  auto r_sorted = KeyValVec(r.size());
  ::radix_hash_df1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          r.size(), 11, 0);

  for (auto _ : state) {
    for (auto r_kv : r_sorted) {
      benchmark::DoNotOptimize(s_map[r_kv.first]);
    }
  }
}

//BENCHMARK(BM_single_table_join)->Range(1<<10, 1<<18)->Complexity();
//BENCHMARK(BM_radix_hash_table_join)->Range(1<<10, 1<<18)->Complexity();
//BENCHMARK(BM_radix_hash_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_hash_join_raw)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join_raw)->Range(1<<10, 1<<18)->Complexity();

BENCHMARK_MAIN();
