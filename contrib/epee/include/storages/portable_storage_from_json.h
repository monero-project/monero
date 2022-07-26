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
#include <boost/lexical_cast.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "parserse_base_utils.h"
#include "file_io_utils.h"

#define EPEE_JSON_RECURSION_LIMIT_INTERNAL 100

namespace epee
{
  using namespace misc_utils::parse;
  namespace serialization
  {
    namespace json
    {
#define CHECK_ISSPACE()  if(!epee::misc_utils::parse::isspace(*it)){ ASSERT_MES_AND_THROW("Wrong JSON character at: " << std::string(it, buf_end));}

      /*inline void parse_error()
      {
        ASSERT_MES_AND_THROW("json parse error");
      }*/
      template<class t_storage>
      inline void run_handler(typename t_storage::hsection current_section, std::string::const_iterator& sec_buf_begin, std::string::const_iterator buf_end, t_storage& stg, unsigned int recursion)
      {
        CHECK_AND_ASSERT_THROW_MES(recursion < EPEE_JSON_RECURSION_LIMIT_INTERNAL, "Wrong JSON data: recursion limitation (" << EPEE_JSON_RECURSION_LIMIT_INTERNAL << ") exceeded");

        std::string name;        
        typename t_storage::harray h_array = nullptr;
        enum match_state
        {
          match_state_lookup_for_section_start, 
          match_state_lookup_for_name, 
          match_state_waiting_separator, 
          match_state_wonder_after_separator, 
          match_state_wonder_after_value, 
          match_state_wonder_array, 
          match_state_array_after_value,
          match_state_array_waiting_value, 
          match_state_error
        };

        enum array_mode
        {
          array_mode_undifined = 0,
          array_mode_sections, 
          array_mode_string, 
          array_mode_numbers,
          array_mode_booleans
        };

        match_state state = match_state_lookup_for_section_start;
        array_mode array_md = array_mode_undifined;
        std::string::const_iterator it = sec_buf_begin;
        for(;it != buf_end;it++)
        {
          switch (state)
          {
          case match_state_lookup_for_section_start:
            if(*it == '{')
              state = match_state_lookup_for_name;
            else CHECK_ISSPACE();
            break;
          case match_state_lookup_for_name:
            switch(*it)
            {
            case '"':
              match_string2(it, buf_end, name);
              state = match_state_waiting_separator;
              break;
            case '}':
              //this is it! section ends here.
              //seems that it is empty section
              sec_buf_begin = it;
              return;
            default:
              CHECK_ISSPACE();
            }
            break;
          case match_state_waiting_separator:
            if(*it == ':')
              state = match_state_wonder_after_separator;
            else CHECK_ISSPACE();
            break;
          case match_state_wonder_after_separator:
            if(*it == '"')
            {//just a named string value started
              std::string val;
              match_string2(it, buf_end, val);
              //insert text value 
              stg.set_value(name, std::move(val), current_section);
              state = match_state_wonder_after_value;
            }else if (epee::misc_utils::parse::isdigit(*it) || *it == '-')
            {//just a named number value started
              boost::string_ref val;
              bool is_v_float = false;bool is_signed = false;
              match_number2(it, buf_end, val, is_v_float, is_signed);
              if(!is_v_float)
              {
                if(is_signed)
                {
                  errno = 0;
                  int64_t nval = strtoll(val.data(), NULL, 10);
                  if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                  stg.set_value(name, int64_t(nval), current_section);
                }else
                {
                  errno = 0;
                  uint64_t nval = strtoull(val.data(), NULL, 10);
                  if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                  stg.set_value(name, uint64_t(nval), current_section);
                }
              }else
              {
                errno = 0;
                double nval = strtod(val.data(), NULL);
                if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                stg.set_value(name, double(nval), current_section);
              }
              state = match_state_wonder_after_value;
            }else if(isalpha(*it) )
            {// could be null, true or false
              boost::string_ref word;
              match_word2(it, buf_end, word);
              if(boost::iequals(word, "null"))
              {
                state = match_state_wonder_after_value;
                //just skip this, 
              }else if(boost::iequals(word, "true"))
              {
                stg.set_value(name, true, current_section);              
                state = match_state_wonder_after_value;
              }else if(boost::iequals(word, "false"))
              {
                stg.set_value(name, false, current_section);              
                state = match_state_wonder_after_value;
              }else ASSERT_MES_AND_THROW("Unknown value keyword " << word);
            }else if(*it == '{')
            {
              //sub section here
              typename t_storage::hsection new_sec = stg.open_section(name, current_section, true);
              CHECK_AND_ASSERT_THROW_MES(new_sec, "Failed to insert new section in json: " << std::string(it, buf_end));
              run_handler(new_sec, it, buf_end, stg, recursion + 1);
              state = match_state_wonder_after_value;
            }else if(*it == '[')
            {//array of something
              state = match_state_wonder_array;
            }else CHECK_ISSPACE();
            break;
          case match_state_wonder_after_value:
            if(*it == ',')
              state = match_state_lookup_for_name;
            else if(*it == '}')
            {
              //this is it! section ends here.
              sec_buf_begin = it;
              return;
            }else CHECK_ISSPACE();
            break;
          case match_state_wonder_array:
            if(*it == '[')
            {
              ASSERT_MES_AND_THROW("array of array not suppoerted yet :( sorry"); 
              //mean array of array
            }
            if(*it == '{')
            {
              //mean array of sections
              typename t_storage::hsection new_sec = nullptr;
              h_array = stg.insert_first_section(name, new_sec, current_section);
              CHECK_AND_ASSERT_THROW_MES(h_array&&new_sec, "failed to create new section");
              run_handler(new_sec, it, buf_end, stg, recursion + 1);
              state = match_state_array_after_value;
              array_md = array_mode_sections;
            }else if(*it == '"')
            {
              //mean array of strings
              std::string val;
              match_string2(it, buf_end, val);
              h_array = stg.insert_first_value(name, std::move(val), current_section);
              CHECK_AND_ASSERT_THROW_MES(h_array, " failed to insert values entry");
              state = match_state_array_after_value;
              array_md = array_mode_string;
            }else if (epee::misc_utils::parse::isdigit(*it) || *it == '-')
            {//array of numbers value started
              boost::string_ref val;
              bool is_v_float = false;bool is_signed_val = false;
              match_number2(it, buf_end, val, is_v_float, is_signed_val);
              if(!is_v_float)
              {
                if (is_signed_val)
                {
                  errno = 0;
                  int64_t nval = strtoll(val.data(), NULL, 10);
                  if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                  h_array = stg.insert_first_value(name, int64_t(nval), current_section);
                }else
                {
                  errno = 0;
                  uint64_t nval = strtoull(val.data(), NULL, 10);
                  if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                  h_array = stg.insert_first_value(name, uint64_t(nval), current_section);
                }
                CHECK_AND_ASSERT_THROW_MES(h_array, " failed to insert values section entry");
              }else
              {
                errno = 0;
                double nval = strtod(val.data(), NULL);
                if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                h_array = stg.insert_first_value(name, double(nval), current_section);
                CHECK_AND_ASSERT_THROW_MES(h_array, " failed to insert values section entry");
              }

              state = match_state_array_after_value;
              array_md = array_mode_numbers;
            }else if(*it == ']')//empty array
            {
              array_md = array_mode_undifined;
              state = match_state_wonder_after_value;
            }else if(isalpha(*it) )
            {// array of booleans
              boost::string_ref word;
              match_word2(it, buf_end, word);
              if(boost::iequals(word, "true"))
              {
                h_array = stg.insert_first_value(name, true, current_section);              
                CHECK_AND_ASSERT_THROW_MES(h_array, " failed to insert values section entry");
                state = match_state_array_after_value;
                array_md = array_mode_booleans;
              }else if(boost::iequals(word, "false"))
              {
                h_array = stg.insert_first_value(name, false, current_section);              
                CHECK_AND_ASSERT_THROW_MES(h_array, " failed to insert values section entry");
                state = match_state_array_after_value;
                array_md = array_mode_booleans;

              }else ASSERT_MES_AND_THROW("Unknown value keyword " << word)
            }else CHECK_ISSPACE();
            break;
          case match_state_array_after_value:
            if(*it == ',')
              state = match_state_array_waiting_value;
            else if(*it == ']')
            {
              h_array = nullptr;
              array_md = array_mode_undifined;
              state = match_state_wonder_after_value;
            }else CHECK_ISSPACE();
            break;
          case match_state_array_waiting_value:
            switch(array_md)
            {
            case array_mode_sections:
              if(*it == '{')
              {
                typename t_storage::hsection new_sec = NULL;
                bool res = stg.insert_next_section(h_array, new_sec);
                CHECK_AND_ASSERT_THROW_MES(res&&new_sec, "failed to insert next section");
                run_handler(new_sec, it, buf_end, stg, recursion + 1);
                state = match_state_array_after_value;
              }else CHECK_ISSPACE();
              break;
            case array_mode_string:
              if(*it == '"')
              {
                std::string val;
                match_string2(it, buf_end, val);
                bool res = stg.insert_next_value(h_array, std::move(val));
                CHECK_AND_ASSERT_THROW_MES(res, "failed to insert values");
                state = match_state_array_after_value;
              }else CHECK_ISSPACE();
              break;
            case array_mode_numbers:
              if (epee::misc_utils::parse::isdigit(*it) || *it == '-')
              {//array of numbers value started
                boost::string_ref val;
                bool is_v_float = false;bool is_signed_val = false;
                match_number2(it, buf_end, val, is_v_float, is_signed_val);
                bool insert_res = false;
                if(!is_v_float)
                {
                  if (is_signed_val)
                  {
                    errno = 0;
                    int64_t nval = strtoll(val.data(), NULL, 10);
                    if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                    insert_res = stg.insert_next_value(h_array, int64_t(nval));
                  }else
                  {
                    errno = 0;
                    uint64_t nval = strtoull(val.data(), NULL, 10);
                    if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                    insert_res = stg.insert_next_value(h_array, uint64_t(nval));
                  }
                }else
                {
                  errno = 0;
                  double nval = strtod(val.data(), NULL);
                  if (errno) throw std::runtime_error("Invalid number: " + std::string(val));
                  insert_res = stg.insert_next_value(h_array, double(nval));
                }
                CHECK_AND_ASSERT_THROW_MES(insert_res, "Failed to insert next value");
                state = match_state_array_after_value;
                array_md = array_mode_numbers;
              }else CHECK_ISSPACE();
              break;
            case array_mode_booleans:
              if(isalpha(*it) )
              {// array of booleans
                boost::string_ref word;
                match_word2(it, buf_end, word);
                if(boost::iequals(word, "true"))
                {
                  bool r = stg.insert_next_value(h_array, true);              
                  CHECK_AND_ASSERT_THROW_MES(r, " failed to insert values section entry");
                  state = match_state_array_after_value;
                }else if(boost::iequals(word, "false"))
                {
                  bool r = stg.insert_next_value(h_array, false);
                  CHECK_AND_ASSERT_THROW_MES(r, " failed to insert values section entry");
                  state = match_state_array_after_value;
                }
                else ASSERT_MES_AND_THROW("Unknown value keyword " << word);
              }else CHECK_ISSPACE();
              break;
            case array_mode_undifined:
            default:
              ASSERT_MES_AND_THROW("Bad array state");
            }
            break;
          case match_state_error:
          default:
            ASSERT_MES_AND_THROW("WRONG JSON STATE");
          }
        }
      }
/*
{
    "firstName": "John",
    "lastName": "Smith",
    "age": 25,
    "address": {
        "streetAddress": "21 2nd Street",
        "city": "New York",
        "state": "NY",
        "postalCode": -10021, 
        "have_boobs": true, 
        "have_balls": false 
    },
    "phoneNumber": [
        {
            "type": "home",
            "number": "212 555-1234"
        },
        {
            "type": "fax",
            "number": "646 555-4567"
        }
    ], 
    "phoneNumbers": [
    "812 123-1234",
    "916 123-4567"
    ]
}
*/
      template<class t_storage>
      inline bool load_from_json(const std::string& buff_json, t_storage& stg)
      {
        std::string::const_iterator sec_buf_begin  = buff_json.begin();
        try
        {
          run_handler(nullptr, sec_buf_begin, buff_json.end(), stg, 0);
          return true;
        }
        catch(const std::exception& ex)
        {
          MERROR("Failed to parse json, what: " << ex.what());
          return false;
        }
        catch(...)
        {
          MERROR("Failed to parse json");
          return false;
        }
      }
    }
  }
}
