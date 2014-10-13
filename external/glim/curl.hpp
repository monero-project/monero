/** \file
 * Very simple header-only wrapper around libcurl.\n
 * See also: https://github.com/venam/Browser\n
 * See also: https://github.com/mologie/curl-asio\n
 * See also: http://thread.gmane.org/gmane.comp.web.curl.library/1322 (this one uses a temporary file). */

#ifndef _GLIM_CURL_INCLUDED
#define _GLIM_CURL_INCLUDED

#include "gstring.hpp"
#include "exception.hpp"
#include <curl/curl.h>
#include <algorithm>
#include <functional>
#include <string.h>
#include <stdint.h>

namespace glim {

inline size_t curlWriteToString (void *buffer, size_t size, size_t nmemb, void *userp) {
  ((std::string*) userp)->append ((const char*) buffer, size * nmemb);
  return size * nmemb;};

inline size_t curlReadFromString (void *ptr, size_t size, size_t nmemb, void *userdata);
inline size_t curlReadFromGString (void *ptr, size_t size, size_t nmemb, void *userdata);
inline size_t curlWriteHeader (void *ptr, size_t size, size_t nmemb, void *curlPtr);
inline int curlDebugCB (CURL* curl, curl_infotype type, char* bytes, size_t size, void* curlPtr);

/**
 Simple HTTP requests using cURL.
 Example: \code
   std::string w3 = glim::Curl() .http ("http://www.w3.org/") .go().str();
 \endcode
 */
class Curl {
 protected:
  Curl (const Curl&): _curl (NULL), _headers (NULL), _sent (0), _needs_cleanup (true) {} // No copying.
 public:
  struct PerformError: public glim::Exception {
    PerformError (const char* message, const char* file, int32_t line):
      glim::Exception (message, file, line) {}
  };
  struct GetinfoError: public glim::Exception {
    CURLcode _code; std::string _error;
    GetinfoError (CURLcode code, const std::string& error, const char* file, int32_t line):
      glim::Exception (error, file, line),
      _code (code), _error (error) {}
  };
 public:
  CURL* _curl;
  struct curl_slist *_headers;
  std::function<void (const char* header, int len)> _headerListener;
  std::function<void (curl_infotype type, char* bytes, size_t size)> _debugListener;
  std::string _sendStr; ///< We're using `std::string` instead of `gstring` in order to support payloads larger than 16 MiB.
  glim::gstring _sendGStr; ///< `gstring::view` and `gstring::ref` allow us to zero-copy.
  uint32_t _sent;
  std::string _got;
  bool _needs_cleanup:1; ///< ~Curl will do `curl_easy_cleanup` if `true`.
  char _errorBuf[CURL_ERROR_SIZE];

  Curl (Curl&&) = default;

  /// @param cleanup can be turned off if the cURL is freed elsewhere.
  Curl (bool cleanup = true): _curl (curl_easy_init()), _headers (NULL), _sent (0), _needs_cleanup (cleanup) {
    curl_easy_setopt (_curl, CURLOPT_NOSIGNAL, 1L); // required per http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
    *_errorBuf = 0;}
  /// Wraps an existing handle (will invoke `curl_easy_cleanup` nevertheless).
  /// @param cleanup can be turned off if the cURL is freed elsewhere.
  Curl (CURL* curl, bool cleanup = true): _curl (curl), _headers (NULL), _sent (0), _needs_cleanup (cleanup) {
    curl_easy_setopt (_curl, CURLOPT_NOSIGNAL, 1L); // required per http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
    *_errorBuf = 0;}
  ~Curl(){
    if (_headers) {curl_slist_free_all (_headers); _headers = NULL;}
    if (_curl) {if (_needs_cleanup) curl_easy_cleanup (_curl); _curl = NULL;}
  }

  /** Stores the content to be sent into an `std::string` inside `Curl`.
   * NB: In order to have an effect this method should be used *before* the `http()` and `smtp()` methods. */
  template<typename STR> Curl& send (STR&& text) {
    _sendStr = std::forward<STR> (text);
    _sendGStr.clear();
    _sent = 0;
    return *this;}

  /// Adds "Content-Type" header into `_headers`.
  Curl& contentType (const char* ct) {
    char ctb[64]; gstring cth (sizeof (ctb), ctb, false, 0);
    cth << "Content-Type: " << ct << "\r\n";
    _headers = curl_slist_append (_headers, cth.c_str());
    return *this;
  }

  /// @param fullHeader is a full HTTP header and a newline, e.g. "User-Agent: Me\r\n".
  Curl& header (const char* fullHeader) {
    _headers = curl_slist_append (_headers, fullHeader);
    return *this;
  }

  /**
   Sets the majority of options for the http request.
   NB: If `send` was used with a non-empty string then `http` will use `CURLOPT_UPLOAD`, setting http method to `PUT` (use the `method()` to override).
   \n
   Example: \code
     glim::Curl curl;
     curl.http (url.c_str()) .go();
     std::cout << curl.status() << std::endl << curl.str() << std::endl;
   \endcode
   */
  Curl& http (const char* url, int timeoutSec = 20) {
    curl_easy_setopt (_curl, CURLOPT_NOSIGNAL, 1L); // required per http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
    curl_easy_setopt (_curl, CURLOPT_URL, url);
    curl_easy_setopt (_curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
    curl_easy_setopt (_curl, CURLOPT_WRITEDATA, &_got);
    curl_easy_setopt (_curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt (_curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
    curl_easy_setopt (_curl, CURLOPT_ERRORBUFFER, _errorBuf);
    if (_sendStr.size() || _sendGStr.size()) {
      curl_easy_setopt (_curl, CURLOPT_UPLOAD, 1L); // http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTUPLOAD
      if (_sendStr.size()) {
        curl_easy_setopt (_curl, CURLOPT_INFILESIZE, (long) _sendStr.size());
        curl_easy_setopt (_curl, CURLOPT_READFUNCTION, curlReadFromString);
      } else {
        curl_easy_setopt (_curl, CURLOPT_INFILESIZE, (long) _sendGStr.size());
        curl_easy_setopt (_curl, CURLOPT_READFUNCTION, curlReadFromGString);}
      curl_easy_setopt (_curl, CURLOPT_READDATA, this);}
    if (_headers)
      curl_easy_setopt (_curl, CURLOPT_HTTPHEADER, _headers); // http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTHTTPHEADER
    return *this;
  }

  /**
   Set options for smtp request.
   Example: \code
     long rc = glim::Curl().send ("Subject: subject\r\n\r\n" "text\r\n") .smtp ("from", "to") .go().status();
     if (rc != 250) std::cerr << "Error sending email: " << rc << std::endl;
   \endcode */
  Curl& smtp (const char* from, const char* to) {
    curl_easy_setopt (_curl, CURLOPT_NOSIGNAL, 1L); // required per http://curl.haxx.se/libcurl/c/libcurl-tutorial.html#Multi-threading
    curl_easy_setopt (_curl, CURLOPT_URL, "smtp://127.0.0.1");
    if (from) curl_easy_setopt (_curl, CURLOPT_MAIL_FROM, from);
    bcc (to);
    if (_headers) curl_easy_setopt (_curl, CURLOPT_MAIL_RCPT, _headers);
    curl_easy_setopt (_curl, CURLOPT_WRITEFUNCTION, curlWriteToString);
    curl_easy_setopt (_curl, CURLOPT_WRITEDATA, &_got);
    if (_sendStr.size()) {
      curl_easy_setopt (_curl, CURLOPT_INFILESIZE, (long) _sendStr.size());
      curl_easy_setopt (_curl, CURLOPT_READFUNCTION, curlReadFromString);
      curl_easy_setopt (_curl, CURLOPT_READDATA, this);
    } else if (_sendGStr.size()) {
      curl_easy_setopt (_curl, CURLOPT_INFILESIZE, (long) _sendGStr.size());
      curl_easy_setopt (_curl, CURLOPT_READFUNCTION, curlReadFromGString);
      curl_easy_setopt (_curl, CURLOPT_READDATA, this);
    }
    curl_easy_setopt (_curl, CURLOPT_UPLOAD, 1L);  // cURL now needs this to actually send the email, cf. "http://curl.haxx.se/mail/lib-2013-12/0152.html".
    return *this;
  }

  /** Add SMTP recipient to the `_headers` (which are then set into `CURLOPT_MAIL_RCPT` by the `Curl::smtp`).
   * NB: Should be used *before* the `Curl::smtp`! */
  Curl& bcc (const char* to) {
    if (to) _headers = curl_slist_append (_headers, to);
    return *this;
  }

  /**
   Uses `CURLOPT_CUSTOMREQUEST` to set the http method.
   Can be used both before and after the `http` method.\n
   Example sending a POST request to ElasticSearch: \code
     glim::Curl curl;
     curl.send (C2GSTRING (R"({"query":{"match_all":{}},"facets":{"tags":{"terms":{"field":"tags","size":1000}}}})"));
     curl.method ("POST") .http ("http://127.0.0.1:9200/froples/frople/_search", 120);
     if (curl.verbose().go().status() != 200) GTHROW ("Error fetching tags: " + std::to_string (curl.status()) + ", " + curl.str());
     cout << curl.gstr() << endl;
   \endcode */
  Curl& method (const char* method) {
    curl_easy_setopt (_curl, CURLOPT_CUSTOMREQUEST, method);
    return *this;
  }

  /** Setup a handler to process the headers cURL gets from the response.
   * "The header callback will be called once for each header and only complete header lines are passed on to the callback".\n
   * See http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTHEADERFUNCTION */
  Curl& headerListener (std::function<void (const char* header, int len)> listener) {
    curl_easy_setopt (_curl, CURLOPT_HEADERFUNCTION, curlWriteHeader);
    curl_easy_setopt (_curl, CURLOPT_WRITEHEADER, this);
    _headerListener = listener;
    return *this;
  }

  /** Setup a handler to get the debug messages generated by cURL.
   * See http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTDEBUGFUNCTION */
  Curl& debugListener (std::function<void (curl_infotype type, char* bytes, size_t size)> listener) {
    curl_easy_setopt (_curl, CURLOPT_DEBUGFUNCTION, curlDebugCB);
    curl_easy_setopt (_curl, CURLOPT_DEBUGDATA, this);
    _debugListener = listener;
    return verbose (true);
  }

  /**
   Setup a handler to get some of the debug messages generated by cURL.
   Listener gets a formatted text: outbound data is prepended with "> " and inbound with "< ".\n
   Usage example: \code
     auto curlDebug = std::make_shared<std::string>();
     curl->debugListenerF ([curlDebug](const char* bytes, size_t size) {curlDebug->append (bytes, size);});
     ...
     if (curl->status() != 200) std::cerr << "cURL status != 200; debug follows: " << *curlDebug << std::endl;
   \endcode
   See http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTDEBUGFUNCTION
   @param listener The receiver of the debug information.
   @param data Whether to pass the data (`CURLINFO_DATA_IN`, `CURLINFO_DATA_OUT`) to the `listener`.
  */
  Curl& debugListenerF (std::function<void (const char* bytes, size_t size)> listener, bool data = false) {
    return debugListener ([listener/* = std::move (listener)*/,data] (curl_infotype type, char* bytes, size_t size) {
      GSTRING_ON_STACK (buf, 256);
      auto prepend = [&](const char* prefix) {
        buf << prefix; for (char *p = bytes, *end = bytes + size; p < end; ++p) {buf << *p; if (*p == '\n' && p + 2 < end) buf << prefix;}};
      if (type == CURLINFO_HEADER_IN || (type == CURLINFO_DATA_IN && data)) prepend ("< ");
      else if (type == CURLINFO_HEADER_OUT || (type == CURLINFO_DATA_OUT && data)) prepend ("> ");
      listener (buf.c_str(), buf.size());
    });
  }

  /// Whether to print debug information to `CURLOPT_STDERR`.
  /// Note that when `debugListener` is used, verbose output will go to the listener and not to `CURLOPT_STDERR`.
  Curl& verbose (bool on = true) {
    curl_easy_setopt (_curl, CURLOPT_VERBOSE, on ? 1L : 0L);
    return *this;
  }

  /// Reset the buffers and perform the cURL request.
  Curl& go() {
    _got.clear();
    *_errorBuf = 0;
    if (curl_easy_perform (_curl)) throw PerformError (_errorBuf, __FILE__, __LINE__);
    return *this;
  }

  /// The contents of the response.
  const std::string& str() const {return _got;}
  /// CString of `str`.
  const char* c_str() const {return _got.c_str();}
  /// Returns a gstring "view" into `str`.
  gstring gstr() const {return gstring (0, (void*) _got.data(), false, _got.size());}

  /// The status of the response (For HTTP it's 200 ok, 404 not found, 500 error, etc).
  long status() const {
    long status; CURLcode err = curl_easy_getinfo (_curl, CURLINFO_RESPONSE_CODE, &status);
    if (err) {
      GSTRING_ON_STACK (message, 128) << "CURL error " << (int) err << ": " << curl_easy_strerror (err);
      throw GetinfoError (err, message.str(), __FILE__, __LINE__);
    }
    return status;}
};

/** Moves the content to be sent into a `glim::gstring` inside `Curl`.
 * NB: In order to have an effect this method should be used *before* the `http()` and `smtp()` methods. */
template<> inline Curl& Curl::send<gstring> (gstring&& text) {
  _sendStr.clear();
  _sendGStr = std::move (text);
  _sent = 0;
  return *this;}

inline size_t curlReadFromString (void *ptr, size_t size, size_t nmemb, void *userdata) {
  Curl* curl = (Curl*) userdata;
  size_t len = std::min (curl->_sendStr.size() - curl->_sent, size * nmemb);
  if (len) memcpy (ptr, curl->_sendStr.data() + curl->_sent, len);
  curl->_sent += len;
  return len;}

inline size_t curlReadFromGString (void *ptr, size_t size, size_t nmemb, void *userdata) {
  Curl* curl = (Curl*) userdata;
  size_t len = std::min (curl->_sendGStr.size() - curl->_sent, size * nmemb);
  if (len) memcpy (ptr, curl->_sendGStr.data() + curl->_sent, len);
  curl->_sent += len;
  return len;}

// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTHEADERFUNCTION
inline size_t curlWriteHeader (void *ptr, size_t size, size_t nmemb, void *curlPtr) {
  Curl* curl = (Curl*) curlPtr;
  std::function<void (const char* header, int len)>& listener = curl->_headerListener;
  int len = size * nmemb;
  if (listener) listener ((const char*) ptr, len);
  return (size_t) len;
}

// http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTDEBUGFUNCTION
inline int curlDebugCB (CURL*, curl_infotype type, char* bytes, size_t size, void* curlPtr) {
  Curl* curl = (Curl*) curlPtr;
  auto& listener = curl->_debugListener;
  if (listener) listener (type, bytes, size);
  return 0;
}

/// Example: std::string w3 = glim::curl2str ("http://www.w3.org/");
inline std::string curl2str (const char* url, int timeoutSec = 20) {
  try {
    return glim::Curl().http (url, timeoutSec) .go().str();
  } catch (const std::exception&) {}
  return std::string();
}

}

#endif
