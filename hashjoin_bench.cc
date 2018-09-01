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

#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
#include "radix_hash.h"
#include "hashjoin.h"
#include "partitioned_hash.h"
#include "strgen.h"
#include <assert.h>
#include <sys/resource.h>
#include "papi_setup.h"


static void BM_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::unordered_map<std::string, uint64_t> s_map;

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    s_map.clear();
    s_map.reserve(s.size());
    state.ResumeTiming();

    START_COUNTERS;
    uint64_t sum = 0;
    for (auto s_kv : s) {
      s_map[s_kv.first] = s_kv.second;
    }
    for (auto r_kv : r) {
      benchmark::DoNotOptimize
        (sum += r_kv.second + s_map[r_kv.first]);
    }
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void BM_partitioned_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  const auto r = ::create_strvec(size);
  const auto s = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  std::vector<std::vector<std::pair<std::string, uint64_t>>> r_vectors(1024);
  std::vector<std::unordered_map<std::string, uint64_t>> s_tables(1024);

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (auto v : r_vectors) {
      v.clear();
    }
    for (auto v : s_tables) {
      v.clear();
    }
    state.ResumeTiming();

    START_COUNTERS;
    radix_hash::partition_only(r.begin(), r.end(), &r_vectors, cores, 10);
    radix_hash::partitioned_hash_table(s.begin(), s.end(), &s_tables, cores, 10);
    uint64_t sum = 0;

    for (int i = 0; i < 1024; ++i) {
      for (auto r_pair : r_vectors[i]) {
        benchmark::DoNotOptimize(sum += r_pair.second + s_tables[i][r_pair.first]);
      }
    }

    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HashMergeJoin(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  HashMergeJoin<KeyValVec::iterator,KeyValVec::iterator> hmj;

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    hmj.clear();
    state.ResumeTiming();

    START_COUNTERS;
    hmj = HashMergeJoin<KeyValVec::iterator,
        KeyValVec::iterator>(r.begin(), r.end(),
            s.begin(), s.end(),
            std::thread::hardware_concurrency());
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HashMergeJoin_1thread(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  HashMergeJoin<KeyValVec::iterator,KeyValVec::iterator> hmj;

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    hmj.clear();
    state.ResumeTiming();

    START_COUNTERS;
    hmj = HashMergeJoin<KeyValVec::iterator,
        KeyValVec::iterator>(r.begin(), r.end(), s.begin(), s.end(), 1);
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
    ACCUMULATE_COUNTERS;
  }

  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HashMergeJoin2(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> r_buffer(size);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> s_buffer(size);
  HashMergeJoin2<HashKeyValVec::iterator,HashKeyValVec::iterator> hmj;

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < size; i++) {
      std::get<0>(r_buffer[i]) = std::hash<std::string>{}(std::get<0>(r[i]));
      std::get<1>(r_buffer[i]) = std::get<0>(r[i]);
      std::get<2>(r_buffer[i]) = std::get<1>(r[i]);
      std::get<0>(s_buffer[i]) = std::hash<std::string>{}(std::get<0>(s[i]));
      std::get<1>(s_buffer[i]) = std::get<0>(s[i]);
      std::get<2>(s_buffer[i]) = std::get<1>(s[i]);
    }
    state.ResumeTiming();

    START_COUNTERS;
    hmj = HashMergeJoin2<HashKeyValVec::iterator,
        HashKeyValVec::iterator>(r_buffer.begin(), r_buffer.end(),
            s_buffer.begin(), s_buffer.end(),
            std::thread::hardware_concurrency());
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void BM_HashMergeJoin2_1thread(benchmark::State& state) {
  int size = state.range(0);
  uint64_t sum = 0;
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> r_buffer(size);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> s_buffer(size);
  HashMergeJoin2<HashKeyValVec::iterator,HashKeyValVec::iterator> hmj;

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < size; i++) {
      std::get<0>(r_buffer[i]) = std::hash<std::string>{}(std::get<0>(r[i]));
      std::get<1>(r_buffer[i]) = std::get<0>(r[i]);
      std::get<2>(r_buffer[i]) = std::get<1>(r[i]);
      std::get<0>(s_buffer[i]) = std::hash<std::string>{}(std::get<0>(s[i]));
      std::get<1>(s_buffer[i]) = std::get<0>(s[i]);
      std::get<2>(s_buffer[i]) = std::get<1>(s[i]);
    }
    state.ResumeTiming();

    START_COUNTERS;
    hmj = HashMergeJoin2<HashKeyValVec::iterator,
        HashKeyValVec::iterator>(r_buffer.begin(), r_buffer.end(),
                                 s_buffer.begin(), s_buffer.end(), 1);
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

static void RadixArguments(benchmark::internal::Benchmark* b) {
  uint64_t i = 10*1000;
  uint64_t max = 1000*1000*1000;
  for (;i <= max; i*=10) {
    b->Args({(int)i});
    if (i <= max)
      b->Args({(int)i*5});
    else
      return;
  }
}

BENCHMARK(BM_hash_join_raw)->Apply(RadixArguments);
BENCHMARK(BM_partitioned_hash_join_raw)->Apply(RadixArguments);
BENCHMARK(BM_HashMergeJoin)->Apply(RadixArguments);

// BENCHMARK(BM_hash_join_raw)->RangeMultiplier(2)
// ->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
// ->UseRealTime();
// BENCHMARK(BM_partitioned_hash_join_raw)->RangeMultiplier(2)
// ->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
// ->UseRealTime();
// BENCHMARK(BM_HashMergeJoin)->RangeMultiplier(2)
// ->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
// ->UseRealTime();

/*
BENCHMARK(BM_HashMergeJoin_1thread)->RangeMultiplier(2)
->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin2)->RangeMultiplier(2)
->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin2_1thread)->RangeMultiplier(2)
->Range(1<<18, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();
*/

BENCHMARK_MAIN();
