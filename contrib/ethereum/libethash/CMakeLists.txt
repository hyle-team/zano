# ethash: C/C++ implementation of Ethash, the Ethereum Proof of Work algorithm.
# Copyright 2018 Pawel Bylica.
# Licensed under the Apache License, Version 2.0. See the LICENSE file.

# set(include_dir ${PROJECT_SOURCE_DIR}/include)


add_library(
    ethash
    bit_manipulation.h
    builtins.h
    endianness.hpp
    ethash/ethash.h
    ethash/ethash.hpp
    ethash-internal.hpp
    ethash.cpp
    ethash/hash_types.h
    managed.cpp
    ethash/keccak.h
    ethash/keccak.hpp
    keccak.c
    keccakf800.c
    keccakf1600.c
    kiss99.hpp
    primes.h
    primes.c
    ethash/progpow.hpp
    progpow.cpp
)




