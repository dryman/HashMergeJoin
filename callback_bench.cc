#include <benchmark/benchmark.h>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>
#include "strgen.h"

void std_function_cb(std::vector<std::pair<std::string, uint64_t>> pairs,
                     std::function<void(std::string const& val)> callback) {
  for (auto p : pairs) {
    callback(p.first);
  }
}

template<typename F>
void template_function_cb(std::vector<std::pair<std::string, uint64_t>> pairs,
                          F const& callback) {
  for (auto p : pairs) {
    callback(p.first);
  }
}

class ClassCB {
public:
  ClassCB(std::vector<std::pair<std::string, uint64_t>> pairs) {
    this->pairs = pairs;
  }

  class iterator : std::iterator<std::input_iterator_tag, std::string> {
  public:
    iterator(std::vector<std::pair<std::string, uint64_t>>::iterator it)
      :iter(it) {}
    iterator(const iterator& other) :iter(other.iter) {}
    iterator& operator++() {++iter; return *this; }
    iterator operator++(int) {iterator tmp(*this); operator++(); return tmp;}
    bool operator==(const iterator& rhs) const {return iter == rhs.iter;}
    bool operator!=(const iterator& rhs) const {return iter != rhs.iter;}
    reference operator*() {return iter->first;}

  private:
    std::vector<std::pair<std::string, uint64_t>>::iterator iter;
  };

    iterator begin() {
      return iterator(pairs.begin());
    }

    iterator end() {
      return iterator(pairs.end());
    }
private:
  std::vector<std::pair<std::string, uint64_t>> pairs;
};

static void BM_std_function_cb(benchmark::State& state) {
  int size = state.range(0);
  auto strs = ::create_strvec(size);
  for (auto _ : state) {
    ::std_function_cb(strs, [](std::string const& val) {
        benchmark::DoNotOptimize(val.size());
      });
  }
  state.SetComplexityN(size);
}

static void BM_template_function_cb(benchmark::State& state) {
  int size = state.range(0);
  auto strs = ::create_strvec(size);
  for (auto _ : state) {
    ::template_function_cb(strs, [](std::string const& val) {
        benchmark::DoNotOptimize(val.size());
      });
  }
  state.SetComplexityN(size);
}

static void BM_ClassCB(benchmark::State& state) {
  int size = state.range(0);
  auto cls = ClassCB(::create_strvec(size));
  for (auto _ : state) {
    for (auto val : cls) {
      benchmark::DoNotOptimize(val.size());
    }
  }
  state.SetComplexityN(size);
}

static void BM_two_vec(benchmark::State& state) {
  int size = state.range(0);
  auto vec_a = std::vector<int>(size);
  auto vec_b = std::vector<int>(size);
  auto vec_c = std::vector<int>(1);
  for (int i = 0; i < size; i++) {
    vec_a[i] = i;
    vec_b[i] = size - i;
  }
  for (auto _ : state) {
    vec_c[0] = 0;
    for (int i = 0; i < size; i++) {
      vec_c[0] += vec_a[i] + vec_b[i];
    }
    benchmark::ClobberMemory();
  }
  state.SetComplexityN(size);
}

BENCHMARK(BM_std_function_cb)->Range(1<<10, 1<<20)->Complexity(benchmark::oN);
BENCHMARK(BM_template_function_cb)->Range(1<<10, 1<<20)
->Complexity(benchmark::oN);
BENCHMARK(BM_ClassCB)->Range(1<<10, 1<<20)->Complexity(benchmark::oN);
BENCHMARK(BM_two_vec)->Range(1<<10, 1<<20)->Complexity(benchmark::oN);

BENCHMARK_MAIN();
