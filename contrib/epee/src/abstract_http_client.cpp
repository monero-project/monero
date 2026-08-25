#include <charconv>
#include <math.h>
#include "net/abstract_http_client.h"
#include "net/http_base.h"
#include "net/net_parse_helpers.h"
#include "misc_log_ex.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "net.http"

namespace {
constexpr auto hex_byte_len = 2; // 00 to FF
constexpr char uri_escape_char = '%'; // placed before each byte i.e. %61
// immediately invoked lambda expression that creates a lookup table at compile-time.
// each safe byte for URL is '\0\0', each unsafe byte has the hex representation of the byte.
constexpr auto encoding_table = []() -> std::array<std::array<unsigned char, hex_byte_len>, 256> {
  std::array<std::array<unsigned char, hex_byte_len>, 256> temp{}; // everything is zeroed-out.
  std::string_view unsafe{"\"<>%\\^[]`+$,@:;!#&"};
  for (std::size_t i = 0; i < temp.size(); ++i) {
    if((i <= 32) || (i >= 123) || (unsafe.find(i) != unsafe.npos)) {
      // encode the unsafe characters.
      const auto first {i / 16};
      const auto second {i % 16};
      temp[i][0] = (first < 0xA) ? ('0' + first) : ('A' + (first - 0xA));
      temp[i][1] = (second < 0xA) ? ('0' + second) : ('A' + (second - 0xA));
    }
  }
  return temp;
}(); // note : all this is done at compile-time.
} // namespace

namespace epee
{
namespace net_utils
{
  std::string convert_to_url_format(std::string_view uri)
  {
    std::string result (uri.size() * (hex_byte_len + 1), '\0'); // reserve the max possible size, shrink later.
    auto write_it {result.begin()};
    for (const unsigned char character : uri) {
      const std::array<unsigned char, hex_byte_len>& encoding = encoding_table[character];
      if (encoding.front() == '\0') {
        *(write_it++) = character;
      } else {
        *write_it = uri_escape_char;
        write_it = std::copy(encoding.begin(), encoding.end(), write_it + 1);
      }
    }
    result.resize(std::distance(result.begin(), write_it));
    return result;
  }
  //----------------------------------------------------------------------------------------------------
  std::string convert_from_url_format(std::string_view uri)
  {
    std::string result (uri.size(), '\0'); // reserve the max possible size, shrink later.
    auto write_it {result.begin()};
    for (auto it {uri.cbegin()}, end {uri.cend()}; it < end; ++it) {
      const auto character {*it};
      if (character != uri_escape_char || std::distance(it, end) < (hex_byte_len + 1)) {
        *(write_it++) = character;
        continue;
      }
      unsigned char decoded{};
      const auto hex_begin = it + 1; // skip '%'
      const auto hex_end {hex_begin + hex_byte_len};
      const auto [ptr, ec] = std::from_chars(hex_begin, hex_end, decoded, 16);
      if (ec == std::errc{} && ptr == hex_end) {
        *(write_it++) = decoded;
      } else {
        write_it = std::copy(it, hex_end, write_it); // in case of failure, copy the original string's contents.
      }
      it += hex_byte_len;
    }
    result.resize(std::distance(result.begin(), write_it));
    return result;
  }
  //----------------------------------------------------------------------------------------------------

namespace http
{
  //----------------------------------------------------------------------------------------------------
  bool epee::net_utils::http::abstract_http_client::set_server(const std::string& address, boost::optional<login> user, ssl_options_t ssl_options)
  {
    http::url_content parsed{};
    const bool r = parse_url(address, parsed);
    CHECK_AND_ASSERT_MES(r, false, "failed to parse url: " << address);
    if (parsed.port == 0)
    {
      if (parsed.schema == "http")
        parsed.port = 80;
      else if (parsed.schema == "https")
        parsed.port = 443;
    }
    set_server(std::move(parsed.host), std::to_string(parsed.port), std::move(user), std::move(ssl_options));
    return true;
  }

  bool epee::net_utils::http::abstract_http_client::set_proxy(const std::string& address)
  {
    return false;
  }
}
}
}
