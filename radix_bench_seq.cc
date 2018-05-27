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

#include "radix_hash.h"
#include "radix_sort.h"
#include "strgen.h"
#include "pdqsort/pdqsort.h"
#include "papi_setup.h"

bool str_tuple_cmp (std::tuple<std::size_t, std::string, uint64_t> a,
                    std::tuple<std::size_t, std::string, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

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

static void BM_radix_inplace_seq_int(benchmark::State& state) {
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
    ::radix_int_inplace<std::size_t, uint64_t>(work.begin(), size, 1);
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

static void BM_std_sort_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    state.ResumeTiming();
    START_COUNTERS;
    std::sort(dst.begin(), dst.end(), str_tuple_cmp);
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

static void BM_pdqsort_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    state.ResumeTiming();
    START_COUNTERS;
    pdqsort(dst.begin(), dst.end(), str_tuple_cmp);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_inplace_seq_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  int partition_bits = radix_hash::optimal_partition(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    state.PauseTiming();
    for (int i = 0; i < size; i++) {
      std::get<0>(dst[i]) = std::hash<std::string>{}(std::get<0>(src[i]));
      std::get<1>(dst[i]) = std::get<0>(src[i]);
      std::get<2>(dst[i]) = std::get<1>(src[i]);
    }
    state.ResumeTiming();
    START_COUNTERS;
    radix_hash::radix_inplace_seq<std::string,uint64_t>(dst.begin(),
                                           state.range(0), partition_bits);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_non_inplace_seq_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  int partition_bits = radix_hash::optimal_partition(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    radix_hash::radix_non_inplace_par<std::string,uint64_t>(src.begin(),
                                                            src.end(), dst.begin(),
                                                            partition_bits, 1);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);
  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
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

BENCHMARK(BM_std_sort_int)->Apply(RadixArguments);
BENCHMARK(BM_pdqsort_int)->Apply(RadixArguments);
BENCHMARK(BM_radix_inplace_seq_int)->Apply(RadixArguments);
BENCHMARK(BM_std_sort_str)->Apply(RadixArguments);
BENCHMARK(BM_pdqsort_str)->Apply(RadixArguments);
BENCHMARK(BM_radix_inplace_seq_str)->Apply(RadixArguments);
BENCHMARK(BM_radix_non_inplace_seq_str)->Apply(RadixArguments);

BENCHMARK_MAIN();
