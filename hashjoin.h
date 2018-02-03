#ifndef HASH_JOIN_H
#define HASH_JOIN_H 1

#include <vector>
#include <string>
#include <utility>
#include <type_traits>
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

template<typename RIter, typename SIter>
class HashMergeJoin {
  static_assert(std::is_same<
                typename RIter::value_type::first_type,
                typename SIter::value_type::first_type>::value,
                "RIter and SIter key type must be the same");
  static_assert(std::is_same<
                typename RIter::difference_type,
                typename SIter::differende_type>::value,
                "RIter and SIter difference type must be the same");
  typedef typename RIter::value_type::first_type Key;
  typedef typename RIter::value_type::second_type RValue;
  typedef typename SIter::value_type::second_type SValue;
  typedef typename Difference;
 public:
  HashMergeJoin(RIter r_begin, RIter r_end,
                SIter s_begin, SIter s_end) {
    Difference r_size, s_size;
    r_size = std::difference(r_begin, r_end);
    s_size = std::difference(s_begin, s_end);

  }
 private:
  std::vector<std::tuple<std::size_t, Key, RValue>> r_sorted;
  std::vector<std::tuple<std::size_t, Key, SValue>> s_sorted;
};

#endif
