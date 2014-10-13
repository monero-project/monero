// http://en.wikipedia.org/wiki/Setcontext; man 3 makecontext; man 2 getcontext
// http://www.boost.org/doc/libs/1_53_0/libs/context/doc/html/index.html
// g++ -std=c++11 -O1 -Wall -g test_cbcoro.cc -pthread && ./a.out

#include <glim/exception.hpp>
#include <glim/NsecTimer.hpp>

#include "cbcoro.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep
#include <string.h> // strerror
#include <errno.h>
#include <functional>
using std::function;
#include <thread>
#include <memory>
using std::shared_ptr; using std::make_shared;
#include <string>
using std::string; using std::to_string;
#include <iostream>
using std::cout; using std::endl;

/** A typical remote service with callback. */
void esDelete (int frople, std::function<void(int)> cb) {
  std::thread th ([cb,frople]() {
    cout << "esDelete: sleeping for a second" << endl;
    std::this_thread::sleep_for (std::chrono::seconds (1));
    cb (frople);
  }); th.detach();
}

struct RemoveFroples: public glim::CBCoro {
  const char* _argument;
  RemoveFroples (const char* argument): _argument (argument) {
    cout << "RF: constructor" << endl;
  }
  virtual ~RemoveFroples() {puts ("~RemoveFroples");}
  virtual void run() override {
    for (int i = 1; i <= 4; ++i) {
      cout << "RF: Removing frople " << i << "..." << endl;
      int returnedFrople = 0;
      yieldForCallback ([this,i,&returnedFrople]() {
        if (i != 2) {
          // Sometimes we use a callback.
          esDelete (i, [this,&returnedFrople](int frople) {
            cout << "RF,CB: frople " << frople << "." << endl;
            returnedFrople = frople;
            invokeFromCallback();
          });
        } else {
          // Sometimes we don't use a callback.
          returnedFrople = 0;
          invokeFromCallback();
        }
      });
      cout << "RF: Returned from callback; _returnTo is: " << (intptr_t) _returnTo << "; frople " << returnedFrople << endl;
    }
    cout << "RF: finish! _returnTo is: " << (intptr_t) _returnTo << endl;
  };
};

int main() {
  glim::cbCoro ([](glim::CBCoro* cbcoro) {
    cout << "main: run1, thread " << std::this_thread::get_id() << endl;  // Runs on the `main` thread.
    cbcoro->yieldForCallback ([&]() {
      std::thread callbackThread ([&]() {
        std::this_thread::sleep_for (std::chrono::seconds (4));
        cbcoro->invokeFromCallback();
      }); callbackThread.detach();
    });
    cout << "main: run2, thread " << std::this_thread::get_id() << endl;  // Runs on the `callbackThread`.
  });

  (new RemoveFroples ("argument"))->start();
  cout << "main: returned from RemoveFroples" << endl;

  glim::NsecTimer timer; const int ops = RUNNING_ON_VALGRIND ? 999 : 9999;
  for (int i = 0; i < ops; ++i) glim::cbCoro ([](glim::CBCoro* cbcoro) {});
  double speedEmpty = ops / timer.sec();
  timer.restart();
  for (int i = 0; i < ops; ++i) glim::cbCoro ([](glim::CBCoro* cbcoro) {cbcoro->yieldForCallback ([&]() {cbcoro->invokeFromCallback();});});
  double speedImmediate = ops / timer.sec();

  sleep (5);
  cout << "speed: empty: " << speedEmpty << " o/s" << endl;
  cout << "speed: immediate: " << speedImmediate << " o/s" << endl;
  return 0;
}
