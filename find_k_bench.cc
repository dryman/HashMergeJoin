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
#include "papi_setup.h"

static int size = 10000000; // 10M

static void BM_radix_sort_int_seq(benchmark::State& state) {
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
    ::radix_int_inplace<std::size_t, uint64_t>(work.begin(), size, 1, state.range(0));
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = (u_after.ru_minflt - u_before.ru_minflt)
    / state.iterations();
  state.counters["Major"] = (u_after.ru_majflt - u_before.ru_majflt)
    / state.iterations();
  state.counters["Swap"] = (u_after.ru_nswap - u_before.ru_nswap)
    / state.iterations();
}

static void BM_radix_inplace_seq(benchmark::State& state) {
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
    radix_hash::radix_inplace_seq<std::string,uint64_t>(dst.begin(), size, state.range(0));
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_non_inplace_seq(benchmark::State& state) {
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    radix_hash::radix_non_inplace_par<std::string,uint64_t>(src.begin(),
                                                            src.end(), dst.begin(),
                                                            1, state.range(0));
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);
  getrusage(RUSAGE_SELF, &u_after);

  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

BENCHMARK(BM_radix_sort_int_seq)
    ->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)
    ->Arg(6)->Arg(7)->Arg(8)->Arg(9)->Arg(10)
    ->Arg(11)->Arg(12) ->Arg(13)->Arg(14)->Arg(15);

BENCHMARK(BM_radix_inplace_seq)
    ->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)
    ->Arg(6)->Arg(7)->Arg(8)->Arg(9)->Arg(10)
    ->Arg(11)->Arg(12) ->Arg(13)->Arg(14)->Arg(15);

BENCHMARK(BM_radix_non_inplace_seq)
    ->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(5)
    ->Arg(6)->Arg(7)->Arg(8)->Arg(9)->Arg(10)
    ->Arg(11)->Arg(12) ->Arg(13)->Arg(14)->Arg(15);

BENCHMARK_MAIN();
