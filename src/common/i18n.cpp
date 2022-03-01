// Copyright (c) 2014-2022, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <string>
#include <map>
#include "file_io_utils.h"
#include "common/i18n.h"
#include "translation_files.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <algorithm>

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "i18n"

#define MAX_LANGUAGE_SIZE 16

static const unsigned char qm_magic[16] = {0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95, 0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd};

static std::map<std::string,std::string> i18n_entries;

/* Logging isn't initialized yet when this is run */
/* add std::flush, because std::endl doesn't seem to flush, contrary to expected */
// #define i18n_log(x) do { std::cout << __FILE__ << ":" << __LINE__ << ": " << x << std::endl; std::cout << std::flush; } while(0)
#define i18n_log(x) ((void)0)

std::string i18n_get_language()
{
  const char *e;

  e = getenv("LANG");
  i18n_log("LANG=" << e);
  if (!e || !*e) {
    e = getenv("LC_ALL");
    i18n_log("LC_ALL=" << e);
  }
  if (!e || !*e)
    e = "en";

  std::string language = e;
  language = language.substr(0, language.find("."));
  language = language.substr(0, language.find("@"));

  // check valid values
  for (char c: language)
    if (!strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_-.@", c))
      return "en";

  std::transform(language.begin(), language.end(), language.begin(), tolower);
  if (language.size() > MAX_LANGUAGE_SIZE)
  {
    i18n_log("Language from LANG/LC_ALL suspiciously long, defaulting to en");
    return "en";
  }
  return language;
}

static uint32_t be32(const unsigned char *data)
{
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

static std::string utf16(const unsigned char *data, uint32_t len)
{
  std::string s;
  while (len >= 2) {
    uint32_t code = (data[0] << 8) | data[1];
    data += 2;
    len -= 2;
    if (code >= 0xd800 && code <= 0xdbfff && len >= 2) {
      uint32_t next = (data[0] << 8) | data[1];
      if (next >= 0xdc00 && next <= 0xdfff) {
        code = (code << 10) + next - 0x35dfc00;
        data += 2;
        len -= 2;
      }
    }
    if (code <= 0x7f) {
      s += (char)code;
    }
    else if (code <= 0x7ff) {
      s += 0xc0 | (code >> 6);
      s += 0x80 | (code & 0x3f);
    }
    else if (code <= 0xffff) {
      s += 0xe0 | (code >> 12);
      s += 0x80 | ((code >> 6) & 0x3f);
      s += 0x80 | (code & 0x3f);
    }
    else {
      s += 0xf0 | (code >> 18);
      s += 0x80 | ((code >> 12) & 0x3f);
      s += 0x80 | ((code >> 6) & 0x3f);
      s += 0x80 | (code & 0x3f);
    }
  }
  return s;
}

static std::string utf8(const unsigned char *data, uint32_t len)
{
  /* assume well formedness */
  return std::string((const char *)data,len);
}

int i18n_set_language(const char *directory, const char *base, std::string language)
{
  std::string filename, contents;
  const unsigned char *data;
  size_t datalen;
  size_t idx;
  unsigned char chunk_type;
  uint32_t chunk_size;
  uint32_t num_messages = (uint32_t)-1;
  uint32_t messages_idx = (uint32_t)-1;
  uint32_t offsets_idx = (uint32_t)-1;
  std::string translation, source, context;

  i18n_log("i18n_set_language(" << directory << "," << base << ")");
  if (!directory || !base)
    return -1;

  if (language.empty())
    language = i18n_get_language();
  filename = std::string(directory) + "/" + base + "_" + language + ".qm";
  i18n_log("Loading translations for language " << language);

  boost::system::error_code ignored_ec;
  if (boost::filesystem::exists(filename, ignored_ec)) {
    if (!epee::file_io_utils::load_file_to_string(filename, contents)) {
      i18n_log("Failed to load translations file: " << filename);
      return -1;
    }
  } else {
    i18n_log("Translations file not found: " << filename);
    filename = std::string(base) + "_" + language + ".qm";
    if (!find_embedded_file(filename, contents)) {
      i18n_log("Embedded translations file not found: " << filename);
      const char *underscore = strchr(language.c_str(), '_');
      if (underscore) {
        std::string fallback_language = std::string(language, 0, underscore - language.c_str());
        filename = std::string(directory) + "/" + base + "_" + fallback_language + ".qm";
        i18n_log("Loading translations for language " << fallback_language);
        if (boost::filesystem::exists(filename, ignored_ec)) {
          if (!epee::file_io_utils::load_file_to_string(filename, contents)) {
            i18n_log("Failed to load translations file: " << filename);
            return -1;
          }
        } else {
          i18n_log("Translations file not found: " << filename);
          filename = std::string(base) + "_" + fallback_language + ".qm";
          if (!find_embedded_file(filename, contents)) {
            i18n_log("Embedded translations file not found: " << filename);
            return -1;
          }
        }
      } else {
        return -1;
      }
    }
  }

  data = (const unsigned char*)contents.c_str();
  datalen = contents.size();
  idx = 0;
  i18n_log("Translations file size: " << datalen);

  /* Format of the QM file (AFAICT):
   *   16 bytes magic
   *   chunk list: N instances of chunks:
   *     1 byte: chunk type (0x42: offsets, 0x69: messages)
   *     4 bytes: chunk length, big endian
   *     D bytes: "chunk length" bytes of data
   *
   *   0x42 chunk: N instances of subchunks:
   *     1 byte: subchunk type
   *       0x01: end, no data
   *       0x02: unsupported
   *       0x03: translation
   *         4 bytes: string length, big endian
   *         N bytes: string data, UTF-16 (or UCS2-BE ?)
   *       0x04: unsupported
   *       0x05: obsolete, unsupported
   *       0x06: source text
   *       0x07: context
   *       0x08: obsolete, unsupported
   *       other: unsupported
   *     4 bytes: subchunk length, big endian - except for 0x01, which has none
   *     S bytes: "chunk length" bytes of data
   *   0x69 chunk:
   *     string data indexed by the 0x42 chunk data
   */
  if (datalen < sizeof(qm_magic) || memcmp(data, qm_magic, sizeof(qm_magic))) {
    i18n_log("Bad translations file format: " << filename);
    return -1;
  }
  idx += sizeof(qm_magic);

  while (idx < datalen) {
    if (idx + 5 > datalen) {
      i18n_log("Bad translations file format: " << filename);
      return -1;
    }
    chunk_type = data[idx++];
    chunk_size = be32(data+idx);
    idx += 4;

    i18n_log("Found " << chunk_type << " of " << chunk_size << " bytes");
    if (chunk_size >= datalen || idx > datalen - chunk_size) {
      i18n_log("Bad translations file format: " << filename);
      return -1;
    }

    switch (chunk_type) {
      case 0x42:
        i18n_log("Found offsets at " << idx);
        /* two 32 bit integers, and possible padding */
        offsets_idx = idx;
        num_messages = chunk_size / 8;
        break;
      case 0x69:
        i18n_log("Found messages at " << idx);
        messages_idx = idx;
        break;
      default:
        i18n_log("Found unsupported chunk type: " << chunk_type);
        break;
    }

    idx += chunk_size;
  }

  if (offsets_idx == (uint32_t)-1) {
    i18n_log("No offsets chunk found");
    return -1;
  }
  if (messages_idx == (uint32_t)-1) {
    i18n_log("No messages chunk found");
    return -1;
  }

  for (uint32_t m = 0; m < num_messages; ++m) {
    be32(data+offsets_idx+m*8); // unused
    idx = be32(data+offsets_idx+m*8+4);
    idx += messages_idx;

    if (idx > datalen || idx + 1 > datalen) {
      i18n_log("Bad translations file format: " << filename);
      return -1;
    }

    while (1) {
      if (idx + 5 > datalen) {
        i18n_log("Bad translations file format: " << filename);
        return -1;
      }
      chunk_type = data[idx++];
      chunk_size = 0;
      if (chunk_type == 0x01) {
        i18n_entries[context + std::string("",1) + source] = translation;
        context = std::string();
        source = std::string();
        translation = std::string();
        break;
      }

      chunk_size = be32(data+idx);
      idx += 4;
      i18n_log("Found " << chunk_type << " of " << chunk_size << " bytes");
      if (chunk_size >= datalen || idx > datalen - chunk_size) {
        i18n_log("Bad translations file format: " << filename);
        return -1;
      }
      switch (chunk_type) {
        case 0x03: // translation, UTF-16
          translation = utf16(data+idx, chunk_size);
          i18n_log("Found translation: " << translation);
          break;
        case 0x06: // source, UTF-8
          source = utf8(data+idx, chunk_size);
          i18n_log("Found source: " << source);
          break;
        case 0x07: // context, UTF-8
          context = utf8(data+idx, chunk_size);
          i18n_log("Found context: " << context);
          break;
      }
      idx += chunk_size;
    }
  }

  return 0;
}

/* The entries is constant by that time */
const char *i18n_translate(const char *s, const std::string &context)
{
  const std::string key = context + std::string("", 1) + s;
  std::map<std::string,std::string>::const_iterator i = i18n_entries.find(key);
  if (i == i18n_entries.end())
    return s;
  return (*i).second.c_str();
}


