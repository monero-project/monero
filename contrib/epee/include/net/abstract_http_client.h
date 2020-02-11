#pragma once

#include <boost/optional/optional.hpp>
#include "http_base.h"
#include "http_auth.h"
#include "net/net_ssl.h"
#include "net_parse_helpers.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace epee
{
  namespace net_utils
  {
    //---------------------------------------------------------------------------
    static inline const char* get_hex_vals()
    {
      static const char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
      return hexVals;
    }

    static inline const char* get_unsave_chars()
    {
      //static char unsave_chars[] = "\"<>%\\^[]`+$,@:;/!#?=&";
      static const char unsave_chars[] = "\"<>%\\^[]`+$,@:;!#&";
      return unsave_chars;
    }

    static inline bool is_unsafe(unsigned char compare_char)
    {
      if(compare_char <= 32 || compare_char >= 123)
        return true;

      const char* punsave = get_unsave_chars();

      for(int ichar_pos = 0; 0!=punsave[ichar_pos] ;ichar_pos++)
        if(compare_char == punsave[ichar_pos])
          return true;

      return false;
    }

    static inline
      std::string dec_to_hex(char num, int radix)
    {
      int temp=0;
      std::string csTmp;
      int num_char;

      num_char = (int) num;
      if (num_char < 0)
        num_char = 256 + num_char;

      while (num_char >= radix)
      {
        temp = num_char % radix;
        num_char = (int)floor((float)num_char / (float)radix);
        csTmp = get_hex_vals()[temp];
      }

      csTmp += get_hex_vals()[num_char];

      if(csTmp.size() < 2)
      {
        csTmp += '0';
      }

      std::reverse(csTmp.begin(), csTmp.end());
      //_mbsrev((unsigned char*)csTmp.data());

      return csTmp;
    }
    static inline int get_index(const char *s, char c) { const char *ptr = (const char*)memchr(s, c, 16); return ptr ? ptr-s : -1; }
    static inline
      std::string hex_to_dec_2bytes(const char *s)
    {
      const char *hex = get_hex_vals();
      int i0 = get_index(hex, toupper(s[0]));
      int i1 = get_index(hex, toupper(s[1]));
      if (i0 < 0 || i1 < 0)
        return std::string("%") + std::string(1, s[0]) + std::string(1, s[1]);
      return std::string(1, i0 * 16 | i1);
    }

    static inline std::string convert(char val)
    {
      std::string csRet;
      csRet += "%";
      csRet += dec_to_hex(val, 16);
      return  csRet;
    }
    static inline std::string conver_to_url_format(const std::string& uri)
    {

      std::string result;

      for(size_t i = 0; i!= uri.size(); i++)
      {
        if(is_unsafe(uri[i]))
          result += convert(uri[i]);
        else
          result += uri[i];

      }

      return result;
    }
    static inline std::string convert_from_url_format(const std::string& uri)
    {

      std::string result;

      for(size_t i = 0; i!= uri.size(); i++)
      {
        if(uri[i] == '%' && i + 2 < uri.size())
        {
          result += hex_to_dec_2bytes(uri.c_str() + i + 1);
          i += 2;
        }
        else
          result += uri[i];

      }

      return result;
    }

    static inline std::string convert_to_url_format_force_all(const std::string& uri)
    {

      std::string result;

      for(size_t i = 0; i!= uri.size(); i++)
      {
          result += convert(uri[i]);
      }

      return result;
    }

    namespace http
    {
      /**
       * Abstract HTTP client interface.
       */
      class abstract_http_client
      {
      public:
        abstract_http_client() {}
        virtual ~abstract_http_client() {}
        virtual bool set_server(const std::string& address, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect)
        {
          http::url_content parsed{};
          const bool r = parse_url(address, parsed);
          CHECK_AND_ASSERT_MES(r, false, "failed to parse url: " << address);
          set_server(std::move(parsed.host), std::to_string(parsed.port), std::move(user), std::move(ssl_options));
          return true;
        }
        virtual void set_server(std::string host, std::string port, boost::optional<login> user, ssl_options_t ssl_options = ssl_support_t::e_ssl_support_autodetect) = 0;
        virtual void set_auto_connect(bool auto_connect) = 0;
        template<typename F>
        void set_connector(F connector) {}  // TODO woodser: set_connector cannot be virtual because of template, remove from abstract?
        virtual bool connect(std::chrono::milliseconds timeout) = 0;
        virtual bool disconnect() = 0;
        virtual bool is_connected(bool *ssl = NULL) = 0;
        virtual bool invoke(const boost::string_ref uri, const boost::string_ref method, const std::string& body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
        virtual bool invoke_get(const boost::string_ref uri, std::chrono::milliseconds timeout, const std::string& body = std::string(), const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
        virtual bool invoke_post(const boost::string_ref uri, const std::string& body, std::chrono::milliseconds timeout, const http_response_info** ppresponse_info = NULL, const fields_list& additional_params = fields_list()) = 0;
        virtual uint64_t get_bytes_sent() const = 0;
        virtual uint64_t get_bytes_received() const = 0;
      };
    }
  }
}
