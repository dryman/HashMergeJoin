#include "hashjoin.h"

void single_table_join(KeyValVec r, KeyValVec s, 
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback) {
  std::unordered_map<std::string, uint64_t> s_map;
  s_map.reserve(s.size());
  for (auto s_kv : s) {
    s_map[s_kv.first] = s_kv.second;
  }
  for (auto r_kv : r) {
    callback(r_kv.first, r_kv.second, s_map[r_kv.first]);
  }
}

void radix_hash_table_join(KeyValVec r, KeyValVec s, int table_size,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback) {
  unsigned long long input_size = r.size();
  int input_clz = __builtin_clzll(input_size);
  std::unordered_map<std::string, uint64_t> table;
  table.reserve(table_size);

}

void radix_hash_join(KeyValVec r, KeyValVec s, 
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback) {
}

