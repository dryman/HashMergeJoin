#ifndef HASH_JOIN_H
#define HASH_JOIN_H 1

#include <vector>
#include <string>
#include <utility>
#include <functional>
#include "radix_hash.h"

typedef std::vector<std::pair<std::string, uint64_t>> KeyValVec;
typedef std::vector<std::tuple<std::size_t, std::string, uint64_t>>
  HashKeyValVec;

void single_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string const& key, uint64_t r_val, uint64_t s_val)>
    callback);

template<typename F>
void radix_hash_join_templated(KeyValVec r, KeyValVec s, F const& callback) {
  auto r_sorted = HashKeyValVec(r.size());
  auto s_sorted = HashKeyValVec(s.size());

  ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), r_sorted.begin(),
                                          r.size(), 11, 0);
  ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(), s_sorted.begin(),
                                          s.size(), 11, 0);

  for (auto r_it = r_sorted.begin(),
         s_it = s_sorted.begin();
       r_it != r_sorted.end() || s_it != s_sorted.end();) {
    callback(std::get<1>(*r_it), std::get<2>(*r_it), std::get<2>(*s_it));
    r_it++;
    s_it++;
  }
}

#endif
