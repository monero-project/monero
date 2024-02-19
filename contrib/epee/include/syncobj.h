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




#ifndef __WINH_OBJ_H__
#define __WINH_OBJ_H__

#include <boost/chrono/duration.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/detail/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
#include <cstdint>
#include "misc_log_ex.h"
#include "misc_language.h"

namespace epee
{

  namespace debug
  {
    inline unsigned int &g_test_dbg_lock_sleep()
    {
      static unsigned int value = 0;
      return value;
    }
  }
  
  struct simple_event
  {
    simple_event() : m_rised(false)
    {
    }

    void raise()
    {
      boost::unique_lock<boost::mutex> lock(m_mx);
      m_rised = true;
      m_cond_var.notify_one();
    }

    void wait()
    {
      boost::unique_lock<boost::mutex> lock(m_mx);
      while (!m_rised) 
        m_cond_var.wait(lock);
      m_rised = false;
    }

  private:
    boost::mutex m_mx;
    boost::condition_variable m_cond_var;
    bool m_rised;
  };

  class critical_region;

  class critical_section
  {
    boost::recursive_mutex m_section;

  public:
    //to make copy fake!
    critical_section(const critical_section& section)
    {
    }

    critical_section()
    {
    }

    ~critical_section()
    {
    }

    void lock()
    {
      m_section.lock();
      //EnterCriticalSection( &m_section );
    }

    void unlock()
    {
      m_section.unlock();
    }

    bool tryLock()
    {
      return m_section.try_lock();
    }

    // to make copy fake
    critical_section& operator=(const critical_section& section)
    {
      return *this;
    }
  };


  template<class t_lock>
  class critical_region_t
  {
    t_lock&	m_locker;
    bool m_unlocked;

    critical_region_t(const critical_region_t&) {}

  public:
    critical_region_t(t_lock& cs): m_locker(cs), m_unlocked(false)
    {
      m_locker.lock();
    }

    ~critical_region_t()
    {
      unlock();
    }

    void unlock()
    {
      if (!m_unlocked)
      {
        m_locker.unlock();
        m_unlocked = true;
      }
    }
  };


  struct reader_writer_lock {

    reader_writer_lock() noexcept : 
    write_mode(false),
    read_mode(0) {};

    bool start_read() noexcept {
      if (have_write()) {
        return false;
      }
      lock_reader();
      return true;
    }

    void end_read() noexcept {
      unlock_reader();
      condition.notify_all();
    }

    bool start_write() noexcept {
      if (have_write()) {
        return false;
      }
      lock_and_change(boost::this_thread::get_id());
      return true;
    }

    void end_write() noexcept {
      unlock_writer();
      condition.notify_all();
    }

    bool have_write() noexcept {
      boost::mutex::scoped_lock lock(internal_mutex);
      if (write_mode
          && writer_id == boost::this_thread::get_id()) {
        return true;
      }
      return false;
    }

    ~reader_writer_lock() = default;
    reader_writer_lock(reader_writer_lock&) = delete;
    reader_writer_lock operator=(reader_writer_lock&) = delete;

  private:

    void unlock_writer() noexcept {
      CHECK_AND_ASSERT_MES2(write_mode && !read_mode, "Ending write transaction by " << boost::this_thread::get_id() << " while the lock is not in write mode");
      boost::mutex::scoped_lock lock(internal_mutex);      
      rw_mutex.unlock();
      write_mode = false;
      writer_id = boost::thread::id();
    }

    void unlock_reader() noexcept {
      CHECK_AND_ASSERT_MES2(read_mode && !write_mode, "Ending read transaction by " << boost::this_thread::get_id() << " while the lock is not in read mode");
      boost::mutex::scoped_lock lock(internal_mutex);      
      rw_mutex.unlock_shared();
      read_mode--;
      if (rw_mutex.try_lock()) {
        CHECK_AND_ASSERT_MES2(!read_mode, "Reader is not zero but goes to read_mode = 0 by " << boost::this_thread::get_id());
        write_mode = false;
        rw_mutex.unlock();
      }
      return; 
    }

    void lock_reader() noexcept {
      CHECK_AND_ASSERT_MES2(read_mode < UINT32_MAX, "Maximum number of readers reached.");
      do {  
        boost::mutex::scoped_lock lock(internal_mutex);      
        if(!write_mode && rw_mutex.try_lock_shared()) {
          write_mode = false;
          read_mode++;
          return; 
        }
        condition.wait(lock);
      } while(true);
    }

    void lock_and_change(boost::thread::id new_owner) noexcept {
      do {  
        boost::mutex::scoped_lock lock(internal_mutex);      
        if (!read_mode && rw_mutex.try_lock()) {
          writer_id = new_owner;
          write_mode = true;
          read_mode = 0;
          return; 
        }
        condition.wait(lock);
      } while(true);
    }

    boost::mutex internal_mutex; // keep the data in RWLock consistent.
    boost::shared_mutex rw_mutex;
    bool write_mode = false;
    uint32_t read_mode = 0;
    boost::thread::id writer_id;
    boost::condition_variable condition;

  };

#define RWLOCK(lock)                                                         \
  bool rw_release_required##lock = lock.start_write();                       \
  epee::misc_utils::auto_scope_leave_caller scope_exit_handler##lock =       \
      epee::misc_utils::create_scope_leave_handler([&]() {                   \
        if (rw_release_required##lock)                                       \
          lock.end_write();                                                  \
      });                                                                    


#define RLOCK(lock)                                                          \
    bool r_release_required##lock = lock.start_read();                       \
    epee::misc_utils::auto_scope_leave_caller scope_exit_handler##lock =     \
        epee::misc_utils::create_scope_leave_handler([&]() {                 \
          if (r_release_required##lock)                                      \
            lock.end_read();                                                 \
        });


#define  CRITICAL_REGION_LOCAL(x) {} epee::critical_region_t<decltype(x)>   critical_region_var(x)
#define  CRITICAL_REGION_BEGIN(x) { boost::this_thread::sleep_for(boost::chrono::milliseconds(epee::debug::g_test_dbg_lock_sleep())); epee::critical_region_t<decltype(x)>   critical_region_var(x)
#define  CRITICAL_REGION_LOCAL1(x) {boost::this_thread::sleep_for(boost::chrono::milliseconds(epee::debug::g_test_dbg_lock_sleep()));} epee::critical_region_t<decltype(x)>   critical_region_var1(x)
#define  CRITICAL_REGION_BEGIN1(x) {  boost::this_thread::sleep_for(boost::chrono::milliseconds(epee::debug::g_test_dbg_lock_sleep())); epee::critical_region_t<decltype(x)>   critical_region_var1(x)

#define  CRITICAL_REGION_END() }

}

#endif
