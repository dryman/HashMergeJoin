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

#include <algorithm>
#include <vector>
#include <utility>
#include <benchmark/benchmark.h>
#include <assert.h>
#include <sys/resource.h>
#include <stdio.h>
#include <random>

#include "radix_sort.h"
#include "tbb/parallel_sort.h"
#include "pdqsort/pdqsort.h"
#include "papi_setup.h"

bool pair_cmp (std::pair<std::size_t, uint64_t> a,
               std::pair<std::size_t, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_std_sort_int(benchmark::State& state) {
  int size = state.range(0);
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  std::vector<std::pair<std::size_t, uint64_t>> input;
  std::vector<std::pair<std::size_t, uint64_t>> work;

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    work = input;
    state.ResumeTiming();
    START_COUNTERS;
    std::sort(work.begin(), work.end(), pair_cmp);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}

static void BM_tbb_int(benchmark::State& state) {
  int size = state.range(0);
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  std::vector<std::pair<std::size_t, uint64_t>> input;
  std::vector<std::pair<std::size_t, uint64_t>> work;

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    work = input;
    state.ResumeTiming();
    START_COUNTERS;
    tbb::parallel_sort(work.begin(), work.end(), pair_cmp);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}

static void BM_pdqsort_int(benchmark::State& state) {
  int size = state.range(0);
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  std::vector<std::pair<std::size_t, uint64_t>> input;
  std::vector<std::pair<std::size_t, uint64_t>> work;

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    work = input;
    state.ResumeTiming();
    START_COUNTERS;
    pdqsort(work.begin(), work.end(), pair_cmp);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}

static void BM_radix_sort_1_single(benchmark::State& state) {
  int size = state.range(0);
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  std::vector<std::pair<std::size_t, uint64_t>> input;
  std::vector<std::pair<std::size_t, uint64_t>> work;

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    work = input;
    state.ResumeTiming();
    START_COUNTERS;
    ::radix_int_inplace<std::size_t, uint64_t>(work.begin(), size,
                                          state.range(1), 1);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}

static void BM_radix_sort_1_multi(benchmark::State& state) {
  int size = state.range(0);
  std::default_random_engine generator;
  std::uniform_int_distribution<std::size_t> distribution;
  std::vector<std::pair<std::size_t, uint64_t>> input;
  std::vector<std::pair<std::size_t, uint64_t>> work;
  unsigned int cores = std::thread::hardware_concurrency();

  for (int i = 0; i < size; i++) {
    std::size_t r = distribution(generator);
    input.push_back(std::make_pair(r, i));
  }

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    work = input;
    state.ResumeTiming();
    START_COUNTERS;
    ::radix_int_inplace<std::size_t, uint64_t>(work.begin(), size,
                                          state.range(1), cores);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}


BENCHMARK(BM_std_sort_int)->RangeMultiplier(2)->Range(1<<18, 1<<23)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_tbb_int)->RangeMultiplier(2)->Range(1<<18, 1<<23)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_pdqsort_int)->RangeMultiplier(2)->Range(1<<18, 1<<23)
->Complexity(benchmark::oN)->UseRealTime();

/********************  radix sort 1 single ***********************/

/*
BENCHMARK(BM_radix_sort_1_single)
->Args({1<<18, 3})
->Args({1<<19, 3})
->Args({1<<20, 3})
->Args({1<<21, 3})
->Args({1<<22, 3})
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK(BM_radix_sort_1_single)
->Args({1<<18, 6})
->Args({1<<19, 6})
->Args({1<<20, 6})
->Args({1<<21, 6})
->Args({1<<22, 6})
->Complexity(benchmark::oN)->UseRealTime();


BENCHMARK(BM_radix_sort_1_single)
->Args({1<<18, 8})
->Args({1<<19, 8})
->Args({1<<20, 8})
->Args({1<<21, 8})
->Args({1<<22, 8})
->Complexity(benchmark::oN)->UseRealTime();
*/


BENCHMARK(BM_radix_sort_1_single)
->Args({1<<18, 11})
->Args({1<<19, 11})
->Args({1<<20, 11})
->Args({1<<21, 11})
->Args({1<<22, 11})
->Complexity(benchmark::oN)->UseRealTime();

// BENCHMARK(BM_radix_sort_1_single)
// ->Args({1<<18, 13})
// ->Args({1<<19, 13})
// ->Args({1<<20, 13})
// ->Args({1<<21, 13})
// ->Args({1<<22, 13})
// ->Complexity(benchmark::oN)->UseRealTime();

/********************  radix sort 1 multi ***********************/

/*
BENCHMARK(BM_radix_sort_1_multi)
->Args({1<<18, 3})
->Args({1<<19, 3})
->Args({1<<20, 3})
->Args({1<<21, 3})
->Args({1<<22, 3})
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK(BM_radix_sort_1_multi)
->Args({1<<18, 6})
->Args({1<<19, 6})
->Args({1<<20, 6})
->Args({1<<21, 6})
->Args({1<<22, 6})
->Complexity(benchmark::oN)->UseRealTime();


BENCHMARK(BM_radix_sort_1_multi)
->Args({1<<18, 8})
->Args({1<<19, 8})
->Args({1<<20, 8})
->Args({1<<21, 8})
->Args({1<<22, 8})
->Complexity(benchmark::oN)->UseRealTime();
*/

BENCHMARK(BM_radix_sort_1_multi)
->Args({1<<18, 11})
->Args({1<<19, 11})
->Args({1<<20, 11})
->Args({1<<21, 11})
->Args({1<<22, 11})
->Complexity(benchmark::oN)->UseRealTime();

//BENCHMARK(BM_radix_sort_1_multi)
//->Args({1<<18, 13})
//->Args({1<<19, 13})
//->Args({1<<20, 13})
//->Args({1<<21, 13})
//->Args({1<<22, 13})
//->Complexity(benchmark::oN)->UseRealTime();

/********************  radix sort 2 ***********************/


BENCHMARK_MAIN();
