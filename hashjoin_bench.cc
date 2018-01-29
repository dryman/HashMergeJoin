#include <benchmark/benchmark.h>
#include <string>
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

BENCHMARK(BM_single_table_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_table_join)->Range(1<<10, 1<<18)->Complexity();
BENCHMARK(BM_radix_hash_join)->Range(1<<10, 1<<18)->Complexity();

BENCHMARK_MAIN();
