#ifndef HASH_JOIN_H
#define HASH_JOIN_H 1

#include <vector>
#include <string>
#include <utility>
#include <functional>

typedef std::vector<std::pair<std::string, uint64_t>> KeyValVec;

void single_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_table_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback);

void radix_hash_join(KeyValVec r, KeyValVec s,
    std::function<void(std::string key, uint64_t r_val, uint64_t s_val)>
    callback);

#endif
