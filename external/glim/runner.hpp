#ifndef _GLIM_RUNNER_INCLUDED
#define _GLIM_RUNNER_INCLUDED

#include <algorithm>  // min
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include <curl/curl.h>
#include <event2/event.h> // cf. hiperfifo.cpp at http://article.gmane.org/gmane.comp.web.curl.library/37752

#include <boost/intrusive_ptr.hpp>
#include <boost/lockfree/queue.hpp>  // http://www.boost.org/doc/libs/1_53_0/doc/html/boost/lockfree/queue.html
#include <boost/log/trivial.hpp>

#include <time.h>
#include <stdlib.h>  // rand
#include <sys/eventfd.h>

#include "gstring.hpp"
#include "exception.hpp"

namespace glim {

/// Listens to messages returned by `curl_multi_info_read`.
/// NB: When CURL is queued with `addToCURLM` the CURL's `CURLOPT_PRIVATE` must point to the instance of `CurlmInformationListener`.
struct CurlmInformationListener {
  enum FreeOptions {REMOVE_CURL_FROM_CURLM = 1, CURL_CLEANUP = 2, DELETE_LISTENER = 4, REMOVE_CLEAN_DELETE = 1|2|4};
  virtual FreeOptions information (CURLMsg*, CURLM*) = 0;
  virtual ~CurlmInformationListener() {}
};

/// Listener deferring to a lambda.
struct FunCurlmLisneter: public glim::CurlmInformationListener {
  std::function <void(CURLMsg*, CURLM*)> _fun;
  FreeOptions _freeOptions;
  FunCurlmLisneter (std::function <void(CURLMsg*, CURLM*)>&& fun, FreeOptions freeOptions): _fun (std::move (fun)), _freeOptions (freeOptions) {}
  virtual FreeOptions information (CURLMsg* msg, CURLM* curlm) override {
    if (__builtin_expect ((bool) _fun, 1))
      try {_fun (msg, curlm);} catch (const std::exception& ex) {BOOST_LOG_TRIVIAL (error) << "FunCurlmLisneter] " << ex.what();}
    return _freeOptions;
  }
};

/// Running cURL jobs in a single thread.
/// NB: The RunnerV2 *must* be allocated with `boost::intrusive_ptr` (typically you'd use `RunnerV2::instance()`).
class RunnerV2 {
  std::atomic_int_fast32_t _references {0};  // For intrusive_ptr.
  CURLM* _multi = nullptr;  ///< Initialized in `run`. Should not be used outside of it.
  int _eventFd = 0;  ///< Used to give the `curl_multi_wait` some work when there's no cURL descriptors and to wake it from `withCURLM`.
  boost::lockfree::queue<CURL*, boost::lockfree::capacity<64>> _queue;  ///< `CURL` handles waiting to be added to `CURL_MULTI`.
  std::thread _thread;
  std::atomic_bool _running {false};  /// True if the `_thread` is running.

  using FreeOptions = CurlmInformationListener::FreeOptions;

  friend inline void intrusive_ptr_add_ref (RunnerV2*);
  friend inline void intrusive_ptr_release (RunnerV2*);

  void run() noexcept {
    try {
      if (__builtin_expect (_references <= 0, 0)) GTHROW ("RunnerV2] Must be allocated with boost::intrusive_ptr!");
      _running = true;  // NB: _running only becomes true if we're in the intrusive_ptr. ^^
      pthread_setname_np (pthread_self(), "Runner");
      _multi = curl_multi_init(); if (__builtin_expect (_multi == nullptr, 0)) GTHROW ("!curl_multi_init");
      _eventFd = eventfd (0, EFD_CLOEXEC | EFD_NONBLOCK);  // Used to pause `curl_multi_wait` when there's no other jobs.
      if (__builtin_expect (_eventFd == -1, 0)) GTHROW (std::string ("eventfd: ") + ::strerror (errno));
      while (__builtin_expect (_references > 0, 0)) {
        // Reset the CURL_EVENT_FD value to 0, so that the `curl_multi_wait` can sleep.
        if (__builtin_expect (_eventFd > 0, 1)) {eventfd_t count = 0; eventfd_read (_eventFd, &count);}

        // Add the queued CURL handles to our CURLM.
        CURL* easy = nullptr; while (_queue.pop (easy)) curl_multi_add_handle (_multi, easy);

        // Run the cURL.
        int runningHandles = 0;
        CURLMcode rc = curl_multi_perform (_multi, &runningHandles);  // http://curl.haxx.se/libcurl/c/curl_multi_perform.html
        if (__builtin_expect (rc != CURLM_OK, 0)) BOOST_LOG_TRIVIAL (error) << "Runner] curl_multi_perform: " << curl_multi_strerror (rc);

        // Process the finished handles.
        for (;;) {
          int messagesLeft = 0; CURLMsg* msg = curl_multi_info_read (_multi, &messagesLeft); if (msg) try {
            CURL* curl = msg->easy_handle; CurlmInformationListener* listener = 0;
            if (__builtin_expect (curl_easy_getinfo (curl, CURLINFO_PRIVATE, &listener) == CURLE_OK, 1)) {
              using FOP = CurlmInformationListener::FreeOptions;
              FOP fop = listener->information (msg, _multi);
              if (fop & FOP::REMOVE_CURL_FROM_CURLM) curl_multi_remove_handle (_multi, curl);
              if (fop & FOP::CURL_CLEANUP) curl_easy_cleanup (curl);
              if (fop & FOP::DELETE_LISTENER) delete listener;
            } else {
              curl_multi_remove_handle (_multi, curl);
              curl_easy_cleanup (curl);
            }
          } catch (const std::exception& ex) {BOOST_LOG_TRIVIAL (error) << "Runner] " << ex.what();}
          if (messagesLeft == 0) break;
        }

        // Wait on the cURL file descriptors.
        int descriptors = 0;
        curl_waitfd waitfd = {_eventFd, CURL_WAIT_POLLIN, 0};
        eventfd_t eValue = 0; eventfd_read (_eventFd, &eValue);  // Reset the curlEventFd value to zero.
        rc = curl_multi_wait (_multi, &waitfd, 1, 100, &descriptors);  // http://curl.haxx.se/libcurl/c/curl_multi_wait.html
        if (__builtin_expect (rc != CURLM_OK, 0)) BOOST_LOG_TRIVIAL (error) << "Runner] curl_multi_wait: " << curl_multi_strerror (rc);
      }
    } catch (const std::exception& ex) {BOOST_LOG_TRIVIAL (error) << "Runner] " << ex.what();}
    // Delayed destruction: when we're in intrusive_ptr (_running == true) but no longer referenced.
    if (_running && _references == 0) delete this;  // http://www.parashift.com/c++-faq-lite/delete-this.html
    else _running = false;
  }
public:
  RunnerV2() {
    // Start a thread using CURLM in a thread-safe way (that is, from this single thread only).
    // NB: Handles *can* be passed between threads: http://article.gmane.org/gmane.comp.web.curl.library/33188
    _thread = std::thread (&RunnerV2::run, this);
  }
  ~RunnerV2() {
    _thread.detach();
  }

  /// A singletone instance of the Runner used in order for different programes to reuse the same cURL thread.
  static boost::intrusive_ptr<RunnerV2>& instance() {
    static boost::intrusive_ptr<RunnerV2> INSTANCE (new RunnerV2());
    return INSTANCE;
  }

  /// Schedule a CURL handler to be executed in the cURL thread.
  /// NB: If the handle have a `CURLOPT_PRIVATE` option then it MUST point to an instance of `CurlmInformationListener`.
  void addToCURLM (CURL* easyHandle) {
    if (__builtin_expect (!_queue.push (easyHandle), 0)) GTHROW ("Can't push CURL* into the queue.");
    if (__builtin_expect (_eventFd > 0, 1)) eventfd_write (_eventFd, 1);  // Will wake the `curl_multi_wait` up, in order to run the `curl_multi_add_handle`.
  }

  /// Schedule a CURL handler to be executed in the cURL thread.
  /// NB: `CURLOPT_PRIVATE` is overwritten with a pointer to `FunCurlmLisneter`.
  void addToCURLM (CURL* easyHandle, std::function <void(CURLMsg*, CURLM*)>&& listener,
      FreeOptions freeOptions = static_cast<FreeOptions> (FreeOptions::REMOVE_CURL_FROM_CURLM | FreeOptions::DELETE_LISTENER)) {
    FunCurlmLisneter* funListener = new FunCurlmLisneter (std::move (listener), freeOptions);  // Will be deleted by the Runner.
    curl_easy_setopt (easyHandle, CURLOPT_PRIVATE, funListener);  // Tells `addToCURLM` to call this listener later.
    addToCURLM (easyHandle);
  }
};

inline void intrusive_ptr_add_ref (RunnerV2* runner) {++ runner->_references;}
inline void intrusive_ptr_release (RunnerV2* runner) {if (-- runner->_references == 0 && !runner->_running) delete runner;}

/// Run CURLM requests and completion handlers, as well as other periodic jobs.
class Runner {
  G_DEFINE_EXCEPTION (RunnerEx);
  /// Free CURL during stack unwinding.
  struct FreeCurl {
    Runner* runner; CURL* curl;
    FreeCurl (Runner* runner, CURL* curl): runner (runner), curl (curl) {}
    ~FreeCurl() {
      runner->_handlers.erase (curl);
      curl_multi_remove_handle (runner->_curlm, curl);
      curl_easy_cleanup (curl);
    }
  };
 public:
  struct JobInfo;
  /// The job must return `true` if Runner is to continue invoking it.
  typedef std::function<bool(JobInfo& jobInfo)> job_t;
  struct JobInfo {
    job_t job;
    float pauseSec = 1.0f;
    struct timespec ran = {0, 0};
  };
protected:
  typedef std::function<void(CURLMsg*)> handler_t;
  typedef std::function<void(const char* error)> errlog_t;
  std::shared_ptr<struct event_base> _evbase;
  errlog_t _errlog;
  std::recursive_mutex _mutex;
  typedef std::unique_ptr<struct event, void(*)(struct event*)> event_t;
  std::unordered_map<CURL*, std::pair<handler_t, event_t>> _handlers;
  /// Functions to run periodically.
  typedef std::unordered_map<gstring, JobInfo> jobs_map_t;
  jobs_map_t _jobs;
  CURLM* _curlm = nullptr;
  struct event* _timer = nullptr;

  /// Schedule a function to be run on the event loop. Useful to run all cURL methods on the single event loop thread.
  template<typename F>
  void doInEv (F fun, struct timeval after = {0, 0}) {
    struct Dugout {F fun; struct event* timer; Dugout (F&& fun): fun (std::move (fun)), timer (nullptr) {}} *dugout = new Dugout (std::move (fun));
    event_callback_fn cb = [](evutil_socket_t, short, void* dugout_)->void {
      Dugout* dugout = static_cast<Dugout*> (dugout_);
      event_free (dugout->timer); dugout->timer = nullptr;
      F fun = std::move (dugout->fun); delete dugout;
      fun();
    };
    dugout->timer = evtimer_new (_evbase.get(), cb, dugout);
    evtimer_add (dugout->timer, &after);
  }

  bool shouldRun (jobs_map_t::value_type& entry, const struct timespec& ct) {
    JobInfo& jobInfo = entry.second;
    if (jobInfo.pauseSec <= 0.f) return true; // Run always.
    if (jobInfo.ran.tv_sec == 0) {jobInfo.ran = ct; return true;}
    float delta = (float)(ct.tv_sec - jobInfo.ran.tv_sec);
    delta += (float)(ct.tv_nsec - jobInfo.ran.tv_nsec) / 1000000000.0f;
    if (delta >= jobInfo.pauseSec) {jobInfo.ran = ct; return true;}
    return false;
  }

  /// Used for debugging.
  static uint64_t ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds> (std::chrono::system_clock::now().time_since_epoch()) .count();
  }
  /// Tells CURL to check its sockets.
  void callCurlWithTimeout() {
    //std::cout << __LINE__ << ',' << ms() << ": callCurlWithTimeout" << std::endl;
    int running_handles = 0;
    CURLMcode rc = curl_multi_socket_action (_curlm, CURL_SOCKET_TIMEOUT, 0, &running_handles);
    if (rc != CURLM_OK) {GSTRING_ON_STACK (err, 256) << "glim::Runner: curl_multi_socket_action: " << curl_multi_strerror (rc); _errlog (err.c_str());}
  }

  /// Should only be run when the _mutex is locked.
  void checkForFinishedCurlJobs() {
    //std::cout << __LINE__ << ',' << ms() << ": checkForFinishedCurlJobs" << std::endl;
    nextMessage:
      int msgs_in_queue = 0;
      CURLMsg* msg = curl_multi_info_read (_curlm, &msgs_in_queue);
      if (msg) try {
        auto curl = msg->easy_handle;
        FreeCurl freeCurl (this, curl);
        auto it = _handlers.find (curl);
        if (it != _handlers.end()) it->second.first (msg);
        if (msgs_in_queue > 0) goto nextMessage;
      } catch (const std::exception& ex) {
        char eBuf[512]; gstring err (sizeof(eBuf), eBuf, false, 0);
        err << "glim::Runner: handler: " << ex.what();
        _errlog (err.c_str());
      }
  }
  /// Will reset the timer unless there is a shorter timer already set.
  void restartTimer (uint32_t nextInMicro = 100000) {  // 100ms = 100000µs
    struct timeval tv;
    if (event_pending (_timer, EV_TIMEOUT, &tv) && !tv.tv_sec && tv.tv_usec < nextInMicro) return; // Already have a shorter timeout.
    tv = {0, nextInMicro};
    evtimer_add (_timer, &tv);
  }
  static void evTimerCB (evutil_socket_t, short, void* runner_) {
    //std::cout << __LINE__ << ',' << ms() << ": evTimerCB" << std::endl;
    Runner* runner = (Runner*) runner_;
    runner->callCurlWithTimeout();
    runner->run();
  }
  /// event_callback_fn: There is an activity on a socket we are monitoring for CURL.
  static void evSocketCB (evutil_socket_t sock, short events, void* runner_) {
    //std::cout << __LINE__ << ',' << ms() << ": evSocketCB; sock: " << sock << "; events: " << events << std::endl;
    Runner* runner = (Runner*) runner_;
    int ev_bitmask = (events & EV_READ ? CURL_CSELECT_IN : 0) | (events & EV_WRITE ? CURL_CSELECT_OUT : 0);
    int running_handles = 0;
    CURLMcode rc = curl_multi_socket_action (runner->_curlm, sock, ev_bitmask, &running_handles);
    if (rc != CURLM_OK) {GSTRING_ON_STACK (err, 256) << "glim::Runner: curl_multi_socket_action: " << curl_multi_strerror (rc); runner->_errlog (err.c_str());}
  }
  static void deleteEvent (struct event* ev) {
    //std::cout << __LINE__ << ',' << ms() << ": deleteEvent: " << ev << std::endl;
    event_del (ev); event_free (ev);
  };
  /// curl_socket_callback: CURL asks us to monitor the socket.
  static int curlSocketCB (CURL* easy, curl_socket_t sock, int what, void* runner_, void* socketp) {
    //std::cout << __LINE__ << ',' << ms() << ": curlSocketCB; sock: " << sock << "; what: " << what;
    //std::cout << " (" << (what == 0 ? "none" : what == 1 ? "in" : what == 2 ? "out" : what == 3 ? "inout" : what == 4 ? "remove" : "?") << ")" << std::endl;
    Runner* runner = (Runner*) runner_;
    std::lock_guard<std::recursive_mutex> lock (runner->_mutex);
    if (what & CURL_POLL_REMOVE) {
      auto it = runner->_handlers.find (easy); if (it != runner->_handlers.end()) it->second.second.reset();
      // We can't run `checkForFinishedCurlJobs` from there or bad things would happen
      // (`curl_multi_remove_handle` will be called while we are still in the `curl_multi_socket_action`),
      // but we can schedule the check via the libevent timer.
      runner->restartTimer (0);
    } else {
      auto it = runner->_handlers.find (easy); if (it != runner->_handlers.end() && !it->second.second) {
        event_callback_fn cb = evSocketCB;
        struct event* ev = event_new (runner->_evbase.get(), sock, EV_READ | EV_WRITE | EV_ET | EV_PERSIST, cb, runner);
        event_add (ev, nullptr);
        //std::cout << __LINE__ << ',' << ms() << ": new event: " << ev << std::endl;
        it->second.second = event_t (ev, deleteEvent);
      }
    }
    return 0;
  }
  /// curl_multi_timer_callback: Schedule a CURL timer event or if `timeout_ms` is 0 then run immediately.
  static int curlTimerCB (CURLM* multi, long timeout_ms, void* runner_) {
    //std::cout << __LINE__ << ',' << ms() << ": curlTimerCB; timeout_ms: " << timeout_ms << std::endl;
    if (timeout_ms == -1) return 0; // CURL tells us it doesn't need no timer.
    Runner* runner = (Runner*) runner_;
    if (timeout_ms == 0) { // CURL tells us it wants to run NOW.
      runner->callCurlWithTimeout();
      return 0;
    }
    // CURL asks us to run it `timeout_ms` from now.
    runner->restartTimer (std::min ((uint32_t) timeout_ms, (uint32_t) 100) * 1000); // We wait no more than 100ms.
    return 0;
  }
public:
  Runner (std::shared_ptr<struct event_base> evbase, errlog_t errlog): _evbase (evbase), _errlog (errlog) {
    doInEv ([this]() {
      std::lock_guard<std::recursive_mutex> lock (_mutex);
      _curlm = curl_multi_init(); if (!_curlm) GNTHROW (RunnerEx, "!curl_multi_init");
      auto check = [this](CURLMcode rc) {if (rc != CURLM_OK) {curl_multi_cleanup (_curlm); GNTHROW (RunnerEx, "curl_multi_setopt: " + std::to_string (rc));}};
      check (curl_multi_setopt (_curlm, CURLMOPT_SOCKETDATA, this));
      curl_socket_callback socketCB = curlSocketCB; check (curl_multi_setopt (_curlm, CURLMOPT_SOCKETFUNCTION, socketCB));
      check (curl_multi_setopt (_curlm, CURLMOPT_TIMERDATA, this));
      curl_multi_timer_callback curlTimerCB_ = curlTimerCB; check (curl_multi_setopt (_curlm, CURLMOPT_TIMERFUNCTION, curlTimerCB_));
      event_callback_fn evTimerCB_ = evTimerCB; _timer = evtimer_new (_evbase.get(), evTimerCB_, this);
      restartTimer();
    });
  }
  ~Runner() {
    //std::cout << __LINE__ << ',' << ms() << ": ~Runner" << std::endl;
    std::lock_guard<std::recursive_mutex> lock (_mutex);
    if (_timer) {evtimer_del (_timer); event_free (_timer); _timer = nullptr;}
    doInEv ([curlm = _curlm, handlers = std::move (_handlers)]() {
      for (auto it = handlers.begin(), end = handlers.end(); it != end; ++it) {
        curl_multi_remove_handle (curlm, it->first);
        curl_easy_cleanup (it->first);
      }
      if (curlm) {curl_multi_cleanup (curlm);}
    });
    _curlm = nullptr;
  }

  /** Turns HTTP Pipelining on (or off).
   * See http://curl.haxx.se/libcurl/c/curl_multi_setopt.html#CURLMOPTPIPELINING */
  Runner& pipeline (long enabled = 1) {
    CURLMcode rc = curl_multi_setopt (_curlm, CURLMOPT_PIPELINING, enabled);
    if (rc != CURLM_OK) GNTHROW (RunnerEx, "curl_multi_setopt: " + std::to_string (rc));
    return *this;
  }

  /// Wait for the operation to complete, then call the `handler`, then free the `curl`.
  void multi (CURL* curl, handler_t handler) {
    { std::lock_guard<std::recursive_mutex> lock (_mutex);
      _handlers.insert (std::make_pair (curl, std::make_pair (std::move (handler), event_t (nullptr, nullptr)))); }
    doInEv ([this,curl]() {
      curl_multi_add_handle (_curlm, curl);
    });
  }
  /// Register a new job to be run on the thread loop.
  JobInfo& job (const gstring& name) {
    std::lock_guard<std::recursive_mutex> lock (_mutex);
    return _jobs[name];
  }
  /// Register a new job to be run on the thread loop.
  void schedule (const gstring& name, float pauseSec, job_t job) {
    struct timespec ct; if (pauseSec > 0.f) clock_gettime (CLOCK_MONOTONIC, &ct);
    std::lock_guard<std::recursive_mutex> lock (_mutex);
    JobInfo& jobInfo = _jobs[name];
    jobInfo.job = job;
    jobInfo.pauseSec = pauseSec;
    if (pauseSec > 0.f) jobInfo.ran = ct; // If we need a pause then we also need to know when the job was scheduled.
  }
  /// Register a new job to be run on the thread loop.
  void schedule (float pauseSec, job_t job) {
    // Find a unique job name.
    anotherName:
      GSTRING_ON_STACK (name, 64) << "job" << rand();
      if (_jobs.find (name) != _jobs.end()) goto anotherName;
    schedule (name, pauseSec, std::move (job));
  }
  void removeJob (const gstring& name) {
    std::lock_guard<std::recursive_mutex> lock (_mutex);
    _jobs.erase (name);
  }
  /// Invoked automatically from a libevent timer; can also be invoked manually.
  void run() {
    _mutex.lock();
    checkForFinishedCurlJobs();
    // Run non-CURL jobs. Copy jobs into a local array in order not to run them with the `_mutex` locked.
    struct timespec ct; clock_gettime (CLOCK_MONOTONIC, &ct);
    JobInfo jobs[_jobs.size()]; gstring jobNames[_jobs.size()]; int jn = -1; {
      for (auto it = _jobs.begin(), end = _jobs.end(); it != end; ++it) if (shouldRun (*it, ct)) {
        ++jn; jobNames[jn] = it->first; jobs[jn] = it->second;
    } }
    _mutex.unlock();

    for (; jn >= 0; --jn) try {
      if (!jobs[jn].job (jobs[jn])) removeJob (jobNames[jn]);
    } catch (const std::exception& ex) {
      char eBuf[512]; gstring err (sizeof(eBuf), eBuf, false, 0);
      err << "glim::Runner: error in job " << jobNames[jn] << ": " << ex.what();
      _errlog (err.c_str());
    }
    restartTimer();
  }

  /// Expose CURLM. Useful for curl_multi_setopt (http://curl.haxx.se/libcurl/c/curl_multi_setopt.html).
  CURLM* curlm() const {return _curlm;}
};

} // namespace glim

#endif // _GLIM_RUNNER_INCLUDED
