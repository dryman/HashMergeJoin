#include <algorithm>
#include <vector>
#include <utility>
#include <benchmark/benchmark.h>
#include <assert.h>
#include <sys/resource.h>
#include <stdio.h>

#include "radix_hash.h"
#include "strgen.h"
#include "tbb/parallel_sort.h"
#include "pdqsort/pdqsort.h"
#include "config.h"

#ifdef HAVE_PAPI
#include "papi.h"
const int e_num = 3;
int events[e_num] = {
  PAPI_L1_DCM,
  PAPI_L2_DCM,
  PAPI_L3_TCM,
};
long long papi_values[e_num];
long long acc_values[e_num];

#define RESET_ACC_COUNTERS do { \
  for (int i = 0; i < e_num; i++) \
    acc_values[i] = 0; \
  } while (0)

#define START_COUNTERS do { \
    assert(PAPI_start_counters(events, e_num) == PAPI_OK); \
  } while (0)

#define ACCUMULATE_COUNTERS do { \
  assert(PAPI_stop_counters(papi_values, e_num) == PAPI_OK); \
  for (int i = 0; i < e_num; i++) \
    acc_values[i] += papi_values[i]; \
  } while (0)

#define REPORT_COUNTERS(STATE) do { \
  STATE.counters["L1 miss"] = acc_values[0] / STATE.iterations(); \
  STATE.counters["L2 miss"] = acc_values[1] / STATE.iterations(); \
  STATE.counters["L3 miss"] = acc_values[2] / STATE.iterations(); \
  } while (0)

#else
#define RESET_ACC_COUNTERS (void)0
#define START_COUNTERS (void)0
#define ACCUMULATE_COUNTERS (void)0
#define REPORT_COUNTERS(STATE) (void)0
#endif

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};

bool tuple_cmp (std::tuple<std::size_t, int, int> a,
                std::tuple<std::size_t, int, int> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_radix_hash_bf3_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
  START_COUNTERS;
    ::radix_hash_bf3<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0);
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
  state.SetComplexityN(state.range(0));
}

bool str_tuple_cmp (std::tuple<std::size_t, std::string, uint64_t> a,
                    std::tuple<std::size_t, std::string, uint64_t> b) {
  return std::get<0>(a) < std::get<0>(b);
}

static void BM_qsort_string(benchmark::State& state) {
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

static void BM_radix_hash_bf4_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    ::radix_hash_bf4<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0, cores);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_hash_bf5_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    ::radix_hash_bf5<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           ::compute_power(size),
                                           state.range(1), 0, cores);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_radix_hash_bf6_str(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  unsigned int cores = std::thread::hardware_concurrency();
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    ::radix_hash_bf6<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           state.range(1), 0, cores);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_tbb_sort_string(benchmark::State& state) {
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
    tbb::parallel_sort(dst.begin(), dst.end(), str_tuple_cmp);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);
  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_pdqsort_string(benchmark::State& state) {
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

static void RadixArguments(benchmark::internal::Benchmark* b) {
  for (int i = 11; i < 12; i+=1) {
    for (int j = (1 << 18); j <= (1 << 23); j <<= 1) {
      b->Args({j, i});
    }
  }
}

static void BM_bf6_1_thread(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  auto src = ::create_strvec(size);
  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

  RESET_ACC_COUNTERS;
  for (auto _ : state) {
    START_COUNTERS;
    ::radix_hash_bf6<std::string,uint64_t>(src.begin(),
                                           src.end(), dst.begin(),
                                           state.range(1), 0, 1);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);
  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_bf7(benchmark::State& state) {
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
    ::radix_hash_bf7<std::string,uint64_t>(dst.begin(),
                                           state.range(0), state.range(1));
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}

static void BM_bf8(benchmark::State& state) {
  int size = state.range(0);
  std::vector<std::tuple<std::size_t, std::string, uint64_t>> dst(size);
  unsigned int cores = std::thread::hardware_concurrency();
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
    ::radix_hash_bf8<std::string,uint64_t>(dst.begin(),
                                           state.range(0),
                                           state.range(1), cores);
    ACCUMULATE_COUNTERS;
  }
  REPORT_COUNTERS(state);

  getrusage(RUSAGE_SELF, &u_after);

  state.SetComplexityN(state.range(0));
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
}


BENCHMARK(BM_qsort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_tbb_sort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_pdqsort_string)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf3_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf4_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf5_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();
BENCHMARK(BM_radix_hash_bf6_str)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK(BM_bf6_1_thread)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK(BM_bf7)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK(BM_bf8)->Apply(RadixArguments)
->Complexity(benchmark::oN)->UseRealTime();

BENCHMARK_MAIN();
