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

#include <algorithm>
#include <boost/utility/string_ref.hpp>

namespace epee 
{
namespace misc_utils
{
  namespace parse
  {
    // 1: digit
    // 2: .eE (floating point)
    // 4: alpha
    // 8: whitespace
    // 16: allowed in float but doesn't necessarily mean it's a float
    // 32: \ and " (end of verbatim string)
    static const constexpr uint8_t lut[256]={
      0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 0, 0, // 16
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 32
      8, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 16, 0, 16, 18, 0, // 48
      17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 0, 0, 0, 0, 0, 0, // 64
      0, 4, 4, 4, 4, 22, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 80
      4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 32, 0, 0, 0, // 96
      0, 4, 4, 4, 4, 22, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, // 112
      4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, // 128
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    };

    inline bool isspace(char c)
    {
      return lut[(uint8_t)c] & 8;
    }

    inline bool isdigit(char c)
    {
      return lut[(uint8_t)c] & 1;
    }

    inline std::string transform_to_escape_sequence(const std::string& src)
    {
      static const char escaped[] = "\b\f\n\r\t\v\"\\/";
      std::string::const_iterator it = std::find_first_of(src.begin(), src.end(), escaped, escaped + sizeof(escaped));
      if (it == src.end())
        return src;

      std::string res;
      res.reserve(2 * src.size());
      res.assign(src.begin(), it);
      for(; it!=src.end(); ++it)
      {
        switch(*it)
        {
        case '\b':  //Backspace (ascii code 08)
          res+="\\b"; break;
        case '\f':  //Form feed (ascii code 0C)
          res+="\\f"; break;
        case '\n':  //New line
          res+="\\n"; break;
        case '\r':  //Carriage return
          res+="\\r"; break;
        case '\t':  //Tab
          res+="\\t"; break;
        case '\v':  //Vertical tab
          res+="\\v"; break;
        //case '\'':  //Apostrophe or single quote
        //  res+="\\'"; break;
        case '"':  //Double quote
          res+="\\\""; break;
        case '\\':  //Backslash caracter
          res+="\\\\"; break;
        case '/':  //Backslash caracter
          res+="\\/"; break;
        default:
          res.push_back(*it);
        }
      }
      return res;
    }
    /*
      
      \b  Backspace (ascii code 08)
      \f  Form feed (ascii code 0C)
      \n  New line
      \r  Carriage return
      \t  Tab
      \v  Vertical tab
      \'  Apostrophe or single quote
      \"  Double quote
      \\  Backslash character

      */
      inline void match_string2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        bool escape_mode = false;
        std::string::const_iterator it = star_end_string;
        ++it;
        std::string::const_iterator fi = it;
        while (fi != buf_end && ((lut[(uint8_t)*fi] & 32)) == 0)
          ++fi;
        val.assign(it, fi);
        val.reserve(std::distance(star_end_string, buf_end));
        it = fi;
        for(;it != buf_end;it++)
        {
          if(escape_mode/*prev_ch == '\\'*/)
          {
            switch(*it)
            {
            case 'b':  //Backspace (ascii code 08)
              val.push_back(0x08);break;
            case 'f':  //Form feed (ascii code 0C)
              val.push_back(0x0C);break;
            case 'n':  //New line
              val.push_back('\n');break;
            case 'r':  //Carriage return
              val.push_back('\r');break;
            case 't':  //Tab
              val.push_back('\t');break;
            case 'v':  //Vertical tab
              val.push_back('\v');break;
            case '\'':  //Apostrophe or single quote
              val.push_back('\'');break;
            case '"':  //Double quote
              val.push_back('"');break;
            case '\\':  //Backslash character
              val.push_back('\\');break;
            case '/':  //Slash character
              val.push_back('/');break;
            default:
              val.push_back(*it);
              LOG_PRINT_L0("Unknown escape sequence :\"\\" << *it << "\"");
            }
            escape_mode = false;
          }else if(*it == '"')
          {
            star_end_string = it;
            return;
          }else if(*it == '\\')
          {
            escape_mode = true;
          }          
          else
          {
            val.push_back(*it); 
          }
        }
        ASSERT_MES_AND_THROW("Failed to match string in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_string(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        try
        {

          match_string2(star_end_string, buf_end, val);
          return true;
        }
        catch(...)
        {
          return false;
        }
      }
      inline void match_number2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, boost::string_ref& val, bool& is_float_val, bool& is_signed_val)
      {
        val.clear();
        uint8_t float_flag = 0;
        is_signed_val = false;
        size_t chars = 0;
        std::string::const_iterator it = star_end_string;
        if (it != buf_end && *it == '-')
        {
          is_signed_val = true;
          ++chars;
          ++it;
        }
        for(;it != buf_end;it++)
        {
          const uint8_t flags = lut[(uint8_t)*it];
          if (flags & 16)
          {
            float_flag |= flags;
            ++chars;
          }
          else
          {
            val = boost::string_ref(&*star_end_string, chars);
            if(val.size())
            {
              star_end_string = --it;
              is_float_val = !!(float_flag & 2);
              return;
            }
            else 
              ASSERT_MES_AND_THROW("wrong number in json entry: " << std::string(star_end_string, buf_end));
          }
        }
        ASSERT_MES_AND_THROW("wrong number in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_number(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, boost::string_ref& val)
      {
        try
        {
          bool is_v_float = false;bool is_signed_val = false;
          match_number2(star_end_string, buf_end, val, is_v_float, is_signed_val);
          return !is_v_float;
        }
        catch(...)
        {
          return false;
        }
      }
      inline void match_word2(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, boost::string_ref& val)
      {
        val.clear();

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if (!(lut[(uint8_t)*it] & 4))
          {
            val = boost::string_ref(&*star_end_string, std::distance(star_end_string, it));
            if(val.size())
            {
              star_end_string = --it;
              return;
            }else 
              ASSERT_MES_AND_THROW("failed to match word number in json entry: " << std::string(star_end_string, buf_end));
          }
        }
        ASSERT_MES_AND_THROW("failed to match word number in json entry: " << std::string(star_end_string, buf_end));
      }
      inline bool match_word(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, boost::string_ref& val)
      {
        try
        {
          match_word2(star_end_string, buf_end, val);
          return true;
        }
        catch(...)
        {
          return false;
        }
      }
      inline bool match_word_with_extrasymb(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string& val)
      {
        val.clear();

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(!isalnum(*it) && *it != '-' && *it != '_')
          {
            val.assign(star_end_string, it);
            if(val.size())
            {
              star_end_string = --it;
              return true;
            }else 
              return false;
          }
        }
        return false;
      }
      inline bool match_word_til_equal_mark(std::string::const_iterator& star_end_string, std::string::const_iterator buf_end, std::string::const_iterator& word_end)
      {
        word_end = star_end_string;

        for(std::string::const_iterator it = star_end_string;it != buf_end;it++)
        {
          if(isspace(*it))
          {

            continue;
          }else if( *it == '=' )
          {            
            star_end_string = it;
            word_end = it;
            return true;
          }
        }
        return false;
      }
  }
}
}
