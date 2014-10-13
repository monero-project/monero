// Simple header-only wrapper around libevent's evhttp client.
// See also: https://github.com/cpp-netlib/cpp-netlib/issues/160

#ifndef _GLIM_HGET_INCLUDED
#define _GLIM_HGET_INCLUDED

#include <event2/event.h>
#include <event2/dns.h>
#include <evhttp.h> // http://stackoverflow.com/a/5237994; http://archives.seul.org/libevent/users/Sep-2010/msg00050.html

#include <memory>
#include <functional>
#include <stdexcept>
#include <iostream>
#include <vector>

#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "exception.hpp"
#include "gstring.hpp"

namespace glim {

/// HTTP results
struct hgot {
  int32_t status = 0;
  /// Uses errno codes.
  int32_t error = 0;
  struct evbuffer* body = 0;
  struct evhttp_request* req = 0;
  size_t bodyLength() const {return body ? evbuffer_get_length (body) : 0;}
  /// Warning: the string is NOT zero-terminated.
  const char* bodyData() {return body ? (const char*) evbuffer_pullup (body, -1) : "";}
  /// Returns a zero-terminated string. Warning: modifies the `body` every time in order to add the terminator.
  const char* cbody() {if (!body) return ""; evbuffer_add (body, "", 1); return (const char*) evbuffer_pullup (body, -1);}
  /// A gstring *view* into the `body`.
  glim::gstring gbody() {
    if (!body) return glim::gstring();
    return glim::gstring (glim::gstring::ReferenceConstructor(), (const char*) evbuffer_pullup (body, -1), evbuffer_get_length (body));}
};

/// Used internally to pass both connection and handler into callback.
struct hgetContext {
  struct evhttp_connection* conn;
  std::function<void(hgot&)> handler;
  hgetContext (struct evhttp_connection* conn, std::function<void(hgot&)> handler): conn (conn), handler (handler) {}
};

/// Invoked when evhttp finishes a request.
inline void hgetCB (struct evhttp_request* req, void* ctx_){
  hgetContext* ctx = (hgetContext*) ctx_;

  hgot gt;
  if (req == NULL) gt.error = ETIMEDOUT;
  else if (req->response_code == 0) gt.error = ECONNREFUSED;
  else {
    gt.status = req->response_code;
    gt.body = req->input_buffer;
    gt.req = req;
  }

  try {
    ctx->handler (gt);
  } catch (const std::runtime_error& ex) { // Shouldn't normally happen:
    std::cerr << "glim::hget, handler exception: " << ex.what() << std::endl;
  }

  evhttp_connection_free ((struct evhttp_connection*) ctx->conn);
  //freed by libevent//if (req != NULL) evhttp_request_free (req);
  delete ctx;
}

/**
  C++ wrapper around libevent's http client.
  Example: \code
  hget (evbase, dnsbase) .setRequestBuilder ([](struct evhttp_request* req){
    evbuffer_add (req->output_buffer, "foo", 3);
    evhttp_add_header (req->output_headers, "Content-Length", "3");
  }) .go ("http://127.0.0.1:8080/test", [](hgot& got){
    if (got.error) log_warn ("127.0.0.1:8080 " << strerror (got.error));
    else if (got.status != 200) log_warn ("127.0.0.1:8080 != 200");
    else log_info ("got " << evbuffer_get_length (got.body) << " bytes from /test: " << evbuffer_pullup (got.body, -1));
  }); \endcode
 */
class hget {
 public:
  std::shared_ptr<struct event_base> _evbase;
  std::shared_ptr<struct evdns_base> _dnsbase;
  std::function<void(struct evhttp_request*)> _requestBuilder;
  enum evhttp_cmd_type _method;
 public:
  typedef std::shared_ptr<struct evhttp_uri> uri_t;
  /// The third parameter is the request number, starting from 1.
  typedef std::function<float(hgot&,uri_t,int32_t)> until_handler_t;
 public:
  hget (std::shared_ptr<struct event_base> evbase, std::shared_ptr<struct evdns_base> dnsbase):
    _evbase (evbase), _dnsbase (dnsbase), _method (EVHTTP_REQ_GET) {}

  /// Modifies the request before its execution.
  hget& setRequestBuilder (std::function<void(struct evhttp_request*)> rb) {
    _requestBuilder = rb;
    return *this;
  }

  /** Uses a simple request builder to send the `str`.
   * `str` is a `char` string class with methods `data` and `size`. */
  template<typename STR> hget& payload (STR str, const char* contentType = nullptr, enum evhttp_cmd_type method = EVHTTP_REQ_POST) {
    _method = method;
    return setRequestBuilder ([str,contentType](struct evhttp_request* req) {
      if (contentType) evhttp_add_header (req->output_headers, "Content-Type", contentType);
      char buf[64];
      *glim::itoa (buf, (int) str.size()) = 0;
      evhttp_add_header (req->output_headers, "Content-Length", buf);
      evbuffer_add (req->output_buffer, (const void*) str.data(), (size_t) str.size());
    });
  }

  struct evhttp_request* go (uri_t uri, int32_t timeoutSec, std::function<void(hgot&)> handler) {
    int port = evhttp_uri_get_port (uri.get());
    if (port == -1) port = 80;
    struct evhttp_connection* conn = evhttp_connection_base_new (_evbase.get(), _dnsbase.get(),
      evhttp_uri_get_host (uri.get()), port);
    evhttp_connection_set_timeout (conn, timeoutSec);
    struct evhttp_request *req = evhttp_request_new (hgetCB, new hgetContext(conn, handler));
    int ret = evhttp_add_header (req->output_headers, "Host", evhttp_uri_get_host (uri.get()));
    if (ret) throw std::runtime_error ("hget: evhttp_add_header(Host) != 0");
    if (_requestBuilder) _requestBuilder (req);
    const char* get = evhttp_uri_get_path (uri.get());
    const char* qs = evhttp_uri_get_query (uri.get());
    if (qs == NULL) {
      ret = evhttp_make_request (conn, req, _method, get);
    } else {
      size_t getLen = strlen (get);
      size_t qsLen = strlen (qs);
      char buf[getLen + 1 + qsLen + 1];
      char* caret = stpcpy (buf, get);
      *caret++ = '?';
      caret = stpcpy (caret, qs);
      assert (caret - buf < sizeof (buf));
      ret = evhttp_make_request (conn, req, _method, buf);
    }
    if (ret) throw std::runtime_error ("hget: evhttp_make_request != 0");
    return req;
  }
  struct evhttp_request* go (const char* url, int32_t timeoutSec, std::function<void(hgot&)> handler) {
    return go (std::shared_ptr<struct evhttp_uri> (evhttp_uri_parse (url), evhttp_uri_free), timeoutSec, handler);
  }

  void goUntil (std::vector<uri_t> urls, until_handler_t handler, int32_t timeoutSec = 20);
  /**
     Parse urls and call `goUntil`.
     Example (trying ten times to reach the servers): \code
       std::string path ("/path");
       hget.goUntilS (boost::assign::list_of ("http://server1" + path) ("http://server2" + path),
        [](hgot& got, hget::uri_t uri, int32_t num)->float {
         std::cout << "server: " << evhttp_uri_get_host (uri.get()) << "; request number: " << num << std::endl;
         if (got.status != 200 && num < 10) return 1.f; // Retry in a second.
         return -1.f; // No need to retry the request.
       });
     \endcode
     @param urls is a for-compatible container of strings (where string has methods `data` and `size`).
   */
  template<typename URLS> void goUntilS (URLS&& urls, until_handler_t handler, int32_t timeoutSec = 20) {
    std::vector<uri_t> parsedUrls;
    for (auto&& url: urls) {
      // Copying to stack might be cheaper than malloc in c_str.
      int len = url.size(); char buf[len + 1]; memcpy (buf, url.data(), len); buf[len] = 0;
      struct evhttp_uri* uri = evhttp_uri_parse (buf);
      if (!uri) GTHROW (std::string ("!evhttp_uri_parse: ") + buf);
      parsedUrls.push_back (uri_t (uri, evhttp_uri_free));
    }
    goUntil (parsedUrls, handler, timeoutSec);
  }
  /**
     Parse urls and call `goUntil`.
     Example (trying ten times to reach the servers): \code
       hget.goUntilC (boost::assign::list_of ("http://server1/") ("http://server2/"),
        [](hgot& got, hget::uri_t uri, int32_t num)->float {
         std::cout << "server: " << evhttp_uri_get_host (uri.get()) << "; request number: " << num << std::endl;
         if (got.status != 200 && num < 10) return 1.f; // Retry in a second.
         return -1.f; // No need to retry the request.
       });
     \endcode
     Or with `std::array` instead of `boost::assign::list_of`: \code
       std::array<const char*, 2> urls {{"http://server1/", "http://server2/"}};
       hget.goUntilC (urls, [](hgot& got, hget::uri_t uri, int32_t num)->float {
         return got.status != 200 && num < 10 ? 0.f : -1.f;});
     \endcode
     @param urls is a for-compatible container of C strings (const char*).
   */
  template<typename URLS> void goUntilC (URLS&& urls, until_handler_t handler, int32_t timeoutSec = 20) {
    std::vector<uri_t> parsedUrls;
    for (auto url: urls) {
      struct evhttp_uri* uri = evhttp_uri_parse (url);
      if (!uri) GTHROW (std::string ("Can't parse url: ") + url);
      parsedUrls.push_back (uri_t (uri, evhttp_uri_free));
    }
    goUntil (parsedUrls, handler, timeoutSec);
  }
};

inline void hgetUntilRetryCB (evutil_socket_t, short, void* utilHandlerPtr); // event_callback_fn

/** `hget::goUntil` implementation.
 * This function object is passed to `hget::go` as a handler and calls `hget::go` again if necessary. */
struct HgetUntilHandler {
  hget _hget;
  hget::until_handler_t _handler;
  std::vector<hget::uri_t> _urls;
  int32_t _timeoutSec;
  int32_t _requestNum;
  uint8_t _nextUrl; ///< A round-robin pointer to the next url in `_urls`.
  HgetUntilHandler (hget& hg, hget::until_handler_t handler, std::vector<hget::uri_t> urls, int32_t timeoutSec):
    _hget (hg), _handler (handler), _urls (urls), _timeoutSec (timeoutSec), _requestNum (0), _nextUrl (0) {}
  void operator() (hgot& got) {
    uint8_t urlNum = _nextUrl ? _nextUrl - 1 : _urls.size() - 1;
    float retryAfterSec = _handler (got, _urls[urlNum], _requestNum);
    if (retryAfterSec == 0.f) retry();
    else if (retryAfterSec > 0.f) {
      struct timeval wait;
      wait.tv_sec = (int) retryAfterSec;
      retryAfterSec -= wait.tv_sec;
      wait.tv_usec = (int) (retryAfterSec * 1000000.f);
      int rc = event_base_once (_hget._evbase.get(), -1, EV_TIMEOUT, hgetUntilRetryCB, new HgetUntilHandler (*this), &wait);
      if (rc) throw std::runtime_error ("HgetUntilHandler: event_base_once != 0");
    }
  }
  void start() {retry();}
  void retry() {
    uint8_t nextUrl = _nextUrl++;
    if (_nextUrl >= _urls.size()) _nextUrl = 0;
    ++_requestNum;
    _hget.go (_urls[nextUrl], _timeoutSec, *this);
  }
};

/// Used in `hget::goUntil` to wait in `evtimer_new` before repeating the request.
inline void hgetUntilRetryCB (evutil_socket_t, short, void* utilHandlerPtr) { // event_callback_fn
  std::unique_ptr<HgetUntilHandler> untilHandler ((HgetUntilHandler*) utilHandlerPtr);
  untilHandler->retry();
}

/**
 * Allows to retry the request using multiple URLs in a round-robin fashion.
 * The `handler` returns the number of seconds to wait before retrying the request or -1 if no retry is necessary.
 */
inline void hget::goUntil (std::vector<uri_t> urls, until_handler_t handler, int32_t timeoutSec) {
  HgetUntilHandler (*this, handler, urls, timeoutSec) .start();
}

}

#endif // _GLIM_HGET_INCLUDED
