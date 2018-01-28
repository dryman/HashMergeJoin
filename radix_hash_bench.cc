#include <algorithm>
#include <vector>
#include <utility>
#include <benchmark/benchmark.h>
#include "radix_hash.h"

struct identity_hash
{
  std::size_t operator()(const int& k) const {
    return k;
  }
};

bool pair_cmp (std::pair<int, int> a, std::pair<int, int> b) {
  return a.first < b.first;
}

static void BM_qsort(benchmark::State& state) {
  std::vector<std::pair<int, int>> data;
  int size = state.range(0);
  data.reserve(size);

  for (auto _ : state)
    {
      state.PauseTiming();
      for (int i = size; i > 0; i--) {
        data[i - 1].first = i;
        data[i - 1].second = i;
      }
      state.ResumeTiming();
      std::sort(data.begin(), data.end(), pair_cmp);
    }
}

static void BM_radix_sort_3(benchmark::State& state) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  int size = state.range(0);
  src.reserve(size);
  dst.reserve(size);

  for (int i = size; i > 0; i--) {
    src[i - 1].first = i;
    src[i - 1].second = i;
  }

  for (auto _ : state)
    {
      ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                         size, 3, 0);
    }
}

static void BM_radix_sort_4(benchmark::State& state) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  int size = state.range(0);
  src.reserve(size);
  dst.reserve(size);

  for (int i = size; i > 0; i--) {
    src[i - 1].first = i;
    src[i - 1].second = i;
  }

  for (auto _ : state)
    {
      ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                         size, 4, 0);
    }
}

static void BM_radix_sort_5(benchmark::State& state) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  int size = state.range(0);
  src.reserve(size);
  dst.reserve(size);

  for (int i = size; i > 0; i--) {
    src[i - 1].first = i;
    src[i - 1].second = i;
  }

  for (auto _ : state)
    {
      ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                         size, 5, 0);
    }
}

static void BM_radix_sort_8(benchmark::State& state) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  int size = state.range(0);
  src.reserve(size);
  dst.reserve(size);

  for (int i = size; i > 0; i--) {
    src[i - 1].first = i;
    src[i - 1].second = i;
  }

  for (auto _ : state)
    {
      ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                         size, 8, 0);
    }
}

static void BM_radix_sort_16(benchmark::State& state) {
  std::vector<std::pair<int, int>> src;
  std::vector<std::pair<int, int>> dst;
  int size = state.range(0);
  src.reserve(size);
  dst.reserve(size);

  for (int i = size; i > 0; i--) {
    src[i - 1].first = i;
    src[i - 1].second = i;
  }

  for (auto _ : state)
    {
      ::sort_hash<int,int,identity_hash>(src.begin(), src.end(), dst.begin(),
                                         size, 16, 0);
    }
}

//BENCHMARK(BM_qsort)->Range(1 << 10, 1 << 18)->Complexity();
BENCHMARK(BM_radix_sort_3)->Range(1 << 10, 1 << 22)->Complexity();
BENCHMARK(BM_radix_sort_4)->Range(1 << 10, 1 << 22)->Complexity();
BENCHMARK(BM_radix_sort_5)->Range(1 << 10, 1 << 22)->Complexity();
BENCHMARK(BM_radix_sort_8)->Range(1 << 10, 1 << 22)->Complexity();
BENCHMARK(BM_radix_sort_16)->Range(1 << 10, 1 << 22)->Complexity();

BENCHMARK_MAIN();
