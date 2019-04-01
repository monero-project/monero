// Copyright (c) 2018, Ryo Currency Project
//
// Portions of this file are available under BSD-3 license. Please see ORIGINAL-LICENSE for details
// All rights reserved.
//
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <inttypes.h> 

extern "C" {

/** auxiliary hashing functions
 * 
 * @warning Hash functions were optimized to handle only 200 bytes long input. As such they
 * are not useable outside of PoW calculation.
 *
 * @param data 200byte input data
 * @param hashval 32byte hashed data
 * @{
 */
void conceal_blake256_hash(const uint8_t* data, uint8_t* hashval);
void conceal_skein_hash(const uint8_t* data, uint8_t *hashval);
void conceal_groestl_hash(const uint8_t* data, uint8_t* hashval);
void conceal_jh_hash(const uint8_t* data, uint8_t* hashval);

///@}
}