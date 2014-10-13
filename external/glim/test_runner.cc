#include "runner.hpp"
#include "curl.hpp"
#include <future>
#include <thread>
#include <iostream>
#include <assert.h>

static void testRunner() {
  using glim::RunnerV2;
  auto runner = RunnerV2::instance();

  struct Dugout: public glim::CurlmInformationListener {
    glim::Curl _curl;
    std::promise<std::string> _promise;
    void schedule (RunnerV2* runner) {
      _curl.http ("http://glim.ru/", 2);
      curl_easy_setopt (_curl._curl, CURLOPT_PRIVATE, this);  // Tells `addToCURLM` to call this listener later.
      runner->addToCURLM (_curl._curl);
    }
    virtual FreeOptions information (CURLMsg* msg, CURLM* curlm) override {
      _promise.set_value (_curl.str());
      return static_cast<FreeOptions> (REMOVE_CURL_FROM_CURLM | DELETE_LISTENER);
    }
    virtual ~Dugout() {}
  };
  { Dugout* dugout = new Dugout();
    auto future = dugout->_promise.get_future();
    dugout->schedule (runner.get());
    if (future.get().find ("<html") == std::string::npos) GTHROW ("!html"); }

  auto curl = std::make_shared<glim::Curl>(); curl->http ("http://glim.ru/", 2);
  auto promise = std::make_shared<std::promise<std::string>>(); auto future = promise->get_future();
  runner->addToCURLM (curl->_curl, [curl,promise](CURLMsg*, CURLM*) {promise->set_value (curl->str());});
  if (future.get().find ("<html") == std::string::npos) GTHROW ("!html");
}

static void oldTest() {
  std::shared_ptr<struct event_base> evbase (event_base_new(), event_base_free);
  glim::Runner runner (evbase, [](const char* error) {std::cerr << error << std::endl;});

  auto scheduledJobFired = std::make_shared<bool> (false);
  runner.schedule (0.f, [=](glim::Runner::JobInfo&)->bool {*scheduledJobFired = true; return false;});

  auto curl = std::make_shared<glim::Curl> (false); curl->http ("http://glim.ru/env.cgi?pause=50", 5);
  auto curlDebug = std::make_shared<std::string>(); curl->debugListenerF ([curlDebug](const char* bytes, size_t size) {curlDebug->append (bytes, size);});
  volatile bool ran = false;
  runner.multi (curl->_curl, [curl,&ran,evbase,curlDebug](CURLMsg* msg) {
    std::cout << " status: " << curl->status();
    if (curl->status() == 200) std::cout << " ip: " << curl->gstr().view (0, std::max (curl->gstr().find ("\n"), 0));
    if (curlDebug->find ("GET /env.cgi") == std::string::npos) std::cerr << " No headers in debug? " << *curlDebug << std::endl;
    ran = true;
    event_base_loopbreak (evbase.get());
  });
  //struct timeval tv {1, 0}; event_base_loopexit (evbase.get(), &tv); // Exit the loop in a sec.
  event_base_dispatch (evbase.get());
  if (!ran) GTHROW ("!ran");
  if (!*scheduledJobFired) GTHROW ("!scheduledJobFired");

  std::cout << " pass." << std::endl;
  //waiting: "was introduced in Libevent 2.1.1-alpha"//libevent_global_shutdown();
}

int main () {
  std::cout << "Testing runner.hpp ..." << std::flush; try {

    testRunner();
    oldTest();

  } catch (const std::exception& ex) {
    std::cerr << " exception: " << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
