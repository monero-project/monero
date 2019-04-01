// Copyright (c) 2019 The Circle Foundation
// Copyright (c) 2019 fireice-uk
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

enum cryptonight_algo : size_t
{
	CRYPTONIGHT_CONCEAL
};

constexpr size_t CRYPTONIGHT_MEMORY = 2 * 1024 * 1024;
constexpr uint32_t CRYPTONIGHT_MASK = 0x1FFFF0;
constexpr uint32_t CRYPTONIGHT_ITER = 0x80000;
constexpr uint32_t CRYPTONIGHT_FAST_V8_ITER = 0x40000;
constexpr uint32_t CRYPTONIGHT_CONCEAL_ITER = 0x40000;

template<cryptonight_algo ALGO>
inline constexpr size_t cn_select_memory() { return 0; }

template<>
inline constexpr size_t cn_select_memory<CRYPTONIGHT_CONCEAL>() { return CRYPTONIGHT_MEMORY; }


inline size_t cn_select_memory(cryptonight_algo algo)
{
	switch(algo)
	{
	case CRYPTONIGHT_CONCEAL:
		return CRYPTONIGHT_MEMORY;
	default:
		return 0;
	}
}

template<cryptonight_algo ALGO>
inline constexpr uint32_t cn_select_mask() { return 0; }

template<>
inline constexpr uint32_t cn_select_mask<CRYPTONIGHT_CONCEAL>() { return CRYPTONIGHT_MASK; }


inline size_t cn_select_mask(cryptonight_algo algo)
{
	switch(algo)
	{
	case CRYPTONIGHT_CONCEAL:
		return CRYPTONIGHT_MASK;
	default:
		return 0;
	}
}

template<cryptonight_algo ALGO>
inline constexpr uint32_t cn_select_iter() { return 0; }


template<>
inline constexpr uint32_t cn_select_iter<CRYPTONIGHT_CONCEAL>() { return CRYPTONIGHT_CONCEAL_ITER; }

inline size_t cn_select_iter(cryptonight_algo algo)
{
	switch(algo)
	{
	case CRYPTONIGHT_CONCEAL:
		return CRYPTONIGHT_CONCEAL_ITER;
	default:
		return 0;
	}
}