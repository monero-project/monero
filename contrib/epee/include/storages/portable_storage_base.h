// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 



#pragma once 

#include <cstdint>

#define PORTABLE_STORAGE_SIGNATUREA 0x01011101
#define PORTABLE_STORAGE_SIGNATUREB 0x01020101 // bender's nightmare 
#define PORTABLE_STORAGE_FORMAT_VER 1

#define PORTABLE_RAW_SIZE_MARK_MASK   0x03 
#define PORTABLE_RAW_SIZE_MARK_BYTE   0
#define PORTABLE_RAW_SIZE_MARK_WORD   1
#define PORTABLE_RAW_SIZE_MARK_DWORD  2
#define PORTABLE_RAW_SIZE_MARK_INT64  3

#ifndef MAX_STRING_LEN_POSSIBLE       
#define MAX_STRING_LEN_POSSIBLE       2000000000 //do not let string be so big
#endif

//data types 
#define SERIALIZE_TYPE_INT64                1
#define SERIALIZE_TYPE_INT32                2
#define SERIALIZE_TYPE_INT16                3
#define SERIALIZE_TYPE_INT8                 4
#define SERIALIZE_TYPE_UINT64               5
#define SERIALIZE_TYPE_UINT32               6
#define SERIALIZE_TYPE_UINT16               7
#define SERIALIZE_TYPE_UINT8                8
#define SERIALIZE_TYPE_DOUBLE               9
#define SERIALIZE_TYPE_STRING               10
#define SERIALIZE_TYPE_BOOL                 11
#define SERIALIZE_TYPE_OBJECT               12
#define SERIALIZE_TYPE_ARRAY                13

#define SERIALIZE_FLAG_ARRAY              0x80


namespace epee
{
  namespace serialization
  {
#pragma pack(push)
#pragma pack(1)
    struct storage_block_header
    {
      uint32_t m_signature_a;
      uint32_t m_signature_b;
      uint8_t  m_ver;
    };
#pragma pack(pop)
  }
}
