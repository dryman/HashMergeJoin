#include <benchmark/benchmark.h>
#include <string>
#include <unordered_map>
#include "radix_hash.h"
#include "hashjoin.h"
#include "strgen.h"
#include <assert.h>
#include <sys/resource.h>
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
#endif

static void BM_hash_join_raw(benchmark::State& state) {
  int size = state.range(0);
  auto r = ::create_strvec(size);
  auto s = ::create_strvec(size);

  struct rusage u_before, u_after;
  getrusage(RUSAGE_SELF, &u_before);

#ifdef HAVE_PAPI
  assert(PAPI_start_counters(events, e_num) == PAPI_OK);
#endif

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

#ifdef HAVE_PAPI
  assert(PAPI_stop_counters(papi_values, e_num) == PAPI_OK);
  state.counters["L1 miss"] = papi_values[0];
  state.counters["L2 miss"] = papi_values[1];
  state.counters["L3 miss"] = papi_values[2];
#endif

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

#ifdef HAVE_PAPI
  assert(PAPI_start_counters(events, e_num) == PAPI_OK);
#endif

  for (auto _ : state) {
    hmj = HashMergeJoin<KeyValVec::iterator,
        KeyValVec::iterator>(r.begin(), r.end(),
            s.begin(), s.end(),
            std::thread::hardware_concurrency());
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
  }

#ifdef HAVE_PAPI
  assert(PAPI_stop_counters(papi_values, e_num) == PAPI_OK);
  state.counters["L1 miss"] = papi_values[0];
  state.counters["L2 miss"] = papi_values[1];
  state.counters["L3 miss"] = papi_values[2];
#endif

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

#ifdef HAVE_PAPI
  assert(PAPI_start_counters(events, e_num) == PAPI_OK);
#endif

  for (auto _ : state) {
    hmj = HashMergeJoin<KeyValVec::iterator,
        KeyValVec::iterator>(r.begin(), r.end(), s.begin(), s.end(), 1);
    sum = 0;
    for (auto tuple : hmj) {
      benchmark::DoNotOptimize(sum += *std::get<1>(tuple)+*std::get<2>(tuple));
    }
  }

#ifdef HAVE_PAPI
  assert(PAPI_stop_counters(papi_values, e_num) == PAPI_OK);
  state.counters["L1 miss"] = papi_values[0];
  state.counters["L2 miss"] = papi_values[1];
  state.counters["L3 miss"] = papi_values[2];
#endif

  getrusage(RUSAGE_SELF, &u_after);
  state.counters["Minor"] = u_after.ru_minflt - u_before.ru_minflt;
  state.counters["Major"] = u_after.ru_majflt - u_before.ru_majflt;
  state.counters["Swap"] = u_after.ru_nswap - u_before.ru_nswap;
  state.SetComplexityN(state.range(0)*2);
}

BENCHMARK(BM_hash_join_raw)->RangeMultiplier(2)
->Range(1<<10, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin)->RangeMultiplier(2)
->Range(1<<10, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();
BENCHMARK(BM_HashMergeJoin_1thread)->RangeMultiplier(2)
->Range(1<<10, 1<<24)->Complexity(benchmark::oN)
->UseRealTime();

BENCHMARK_MAIN();
