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

/*
class HashJoinSimple {
 public:
  HashJoinSimple(KeyValVec r, KeyValVec s) {
    _r = HashKeyValVec(r.size());
    _s = HashKeyValVec(s.size());
    ::radix_hash_bf1<std::string, uint64_t>(r.begin(), r.end(), _r.begin(),
                                            r.size(), 11, 0);
    ::radix_hash_bf1<std::string, uint64_t>(s.begin(), s.end(), _s.begin(),
                                            s.size(), 11, 0);
  }
  class iterator : std::iterator<std::input_iterator_tag,
    std::tuple<std::size_t, std::string, uint64_t>> {
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
    std::vector<std::tuple<std::size_t, std::string, uint64_t>>::iterator r_it;
    std::vector<std::tuple<std::size_t, std::string, uint64_t>>::iterator s_it;
  };

    iterator begin() {
      return iterator(pairs.begin());
    }

    iterator end() {
      return iterator(pairs.end());
    }
 private:
  HashKeyValVec _r;
  HashKeyValVec _s;
};
*/

#endif
