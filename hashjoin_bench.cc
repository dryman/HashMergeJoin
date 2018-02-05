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
    ::single_table_join(r, s, [](std::string const& k,
                                 uint64_t r_val, uint64_t s_val) {
                          return;
                        });
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_table_join(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::radix_hash_table_join(r, s, [](std::string const& k,
                                     uint64_t r_val, uint64_t s_val) {
                              return;
                            });
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_join(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::radix_hash_join(r, s, [](std::string const& k,
                               uint64_t r_val, uint64_t s_val) {
                        return;
                      });
  }
  state.SetComplexityN(state.range(0));
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
  state.SetComplexityN(state.range(0));
}

static void BM_hash_join_raw_orderd(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::unordered_map<std::string, uint64_t> r_map;
  std::unordered_map<std::string, uint64_t> s_map;
  r_map.reserve(r.size());
  s_map.reserve(s.size());
  for (auto r_kv : r) {
    r_map[r_kv.first] = r_kv.second;
  }
  for (auto s_kv : s) {
    s_map[s_kv.first] = s_kv.second;
  }

  for (auto _ : state) {
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
  auto s_sorted = HashKeyValVec(s.size());

  ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          ::compute_power(r.size()), 11, 0);
  ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(), s_sorted.begin(),
                                          ::compute_power(s.size()), 11, 0);

  for (auto _ : state) {
    for (auto r_it = r_sorted.begin(),
           s_it = s_sorted.begin();
         r_it != r_sorted.end() || s_it != s_sorted.end();) {
      benchmark::DoNotOptimize(std::get<1>(*r_it) ==
                               std::get<1>(*s_it));
      r_it++;
      s_it++;
    }
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_join_df_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);

  auto r_sorted = HashKeyValVec(r.size());
  auto s_sorted = HashKeyValVec(s.size());

  ::radix_hash_df1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          ::compute_power(r.size()), 11, 0);
  ::radix_hash_df1<std::string, uint64_t>(s.begin(), s.end(), s_sorted.begin(),
                                          ::compute_power(s.size()), 11, 0);

  for (auto _ : state) {
    for (auto r_it = r_sorted.begin(),
           s_it = s_sorted.begin();
         r_it != r_sorted.end() || s_it != s_sorted.end();) {
      benchmark::DoNotOptimize(std::get<1>(*r_it) ==
                               std::get<1>(*s_it));
      r_it++;
      s_it++;
    }
  }
  state.SetComplexityN(state.range(0));
}

static void BM_radix_hash_join_templated(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  for (auto _ : state) {
    ::radix_hash_join_templated(r, s, [](std::string const& k,
                                         uint64_t r_val, uint64_t s_val) {
                                  return;
                                });
  }
  state.SetComplexityN(state.range(0));
}

BENCHMARK(BM_single_table_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_table_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_hash_join_raw)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_hash_join_raw_orderd)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join_raw)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join_df_raw)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join_templated)->Range(1<<10, 1<<18)->Complexity();

BENCHMARK_MAIN();
