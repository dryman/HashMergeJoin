#include <unordered_map>
#include <assert.h>
#include "hashjoin.h"
#include "radix_hash.h"

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

void radix_hash_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback) {

  auto r_sorted = KeyValVec(r.size());
  std::unordered_map<std::string, uint64_t> s_map;
  s_map.reserve(s.size());
  for (auto s_kv : s) {
    s_map[s_kv.first] = s_kv.second;
  }
  ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          r.size(), 11, 0);

  for (auto r_it : r_sorted) {
      callback(r_it.first, r_it.second, s_map[r_it.first]);
  }
}

void radix_hash_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback) {

  auto r_sorted = KeyValVec(r.size());
  auto s_sorted = KeyValVec(s.size());

  ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          r.size(), 11, 0);
  ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(), s_sorted.begin(),
                                          s.size(), 11, 0);

  for (auto r_it = r_sorted.begin(),
         s_it = s_sorted.begin();
       r_it != r_sorted.end() || s_it != s_sorted.end();) {
    //if (r_it->first == s_it->first) {
      callback(r_it->first, r_it->second, s_it->second);
      //}
    r_it++;
    s_it++;
  }
}
