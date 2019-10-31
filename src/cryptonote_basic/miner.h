// Copyright (c) 2014-2019, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#pragma once 

#include <boost/program_options.hpp>
#include <boost/logic/tribool_fwd.hpp>
#include <atomic>
#include "cryptonote_basic.h"
#include "verification_context.h"
#include "difficulty.h"
#include "math_helper.h"
#ifdef _WIN32
#include <windows.h>
#endif

namespace cryptonote
{

  struct i_miner_handler
  {
    virtual bool handle_block_found(block& b, block_verification_context &bvc) = 0;
    virtual bool get_block_template(block& b, const account_public_address& adr, difficulty_type& diffic, uint64_t& height, uint64_t& expected_reward, const blobdata& ex_nonce) = 0;
  protected:
    ~i_miner_handler(){};
  };

  typedef std::function<bool(const cryptonote::block&, uint64_t, unsigned int, crypto::hash&)> get_block_hash_t;

  /************************************************************************/
  /*                                                                      */
  /************************************************************************/
  class miner
  {
  public: 
    miner(i_miner_handler* phandler, const get_block_hash_t& gbh);
    ~miner();
    bool init(const boost::program_options::variables_map& vm, network_type nettype);
    static void init_options(boost::program_options::options_description& desc);
    bool set_block_template(const block& bl, const difficulty_type& diffic, uint64_t height, uint64_t block_reward);
    bool on_block_chain_update();
    bool start(const account_public_address& adr, size_t threads_count, bool do_background = false, bool ignore_battery = false);
    uint64_t get_speed() const;
    uint32_t get_threads_count() const;
    void send_stop_signal();
    bool stop();
    bool is_mining() const;
    const account_public_address& get_mining_address() const;
    bool on_idle();
    void on_synchronized();
    //synchronous analog (for fast calls)
    static bool find_nonce_for_given_block(const get_block_hash_t &gbh, block& bl, const difficulty_type& diffic, uint64_t height);
    void pause();
    void resume();
    void do_print_hashrate(bool do_hr);
    bool get_is_background_mining_enabled() const;
    bool get_ignore_battery() const;
    uint64_t get_min_idle_seconds() const;
    bool set_min_idle_seconds(uint64_t min_idle_seconds);
    uint8_t get_idle_threshold() const;
    bool set_idle_threshold(uint8_t idle_threshold);
    uint8_t get_mining_target() const;
    bool set_mining_target(uint8_t mining_target);
    uint64_t get_block_reward() const { return m_block_reward; }

    static constexpr uint8_t  BACKGROUND_MINING_DEFAULT_IDLE_THRESHOLD_PERCENTAGE       = 90;
    static constexpr uint8_t  BACKGROUND_MINING_MIN_IDLE_THRESHOLD_PERCENTAGE           = 50;
    static constexpr uint8_t  BACKGROUND_MINING_MAX_IDLE_THRESHOLD_PERCENTAGE           = 99;
    static constexpr uint16_t BACKGROUND_MINING_DEFAULT_MIN_IDLE_INTERVAL_IN_SECONDS    = 10;
    static constexpr uint16_t BACKGROUND_MINING_MIN_MIN_IDLE_INTERVAL_IN_SECONDS        = 10;
    static constexpr uint16_t BACKGROUND_MINING_MAX_MIN_IDLE_INTERVAL_IN_SECONDS        = 3600;
    static constexpr uint8_t  BACKGROUND_MINING_DEFAULT_MINING_TARGET_PERCENTAGE        = 40;
    static constexpr uint8_t  BACKGROUND_MINING_MIN_MINING_TARGET_PERCENTAGE            = 5;
    static constexpr uint8_t  BACKGROUND_MINING_MAX_MINING_TARGET_PERCENTAGE            = 50;
    static constexpr uint8_t  BACKGROUND_MINING_MINER_MONITOR_INVERVAL_IN_SECONDS       = 10;
    static constexpr uint64_t BACKGROUND_MINING_DEFAULT_MINER_EXTRA_SLEEP_MILLIS        = 400; // ramp up 
    static constexpr uint64_t BACKGROUND_MINING_MIN_MINER_EXTRA_SLEEP_MILLIS            = 5;

  private:
    bool worker_thread();
    bool request_block_template();
    void  merge_hr();
    void  update_autodetection();
    
    struct miner_config
    {
      uint64_t current_extra_message_index;

      BEGIN_KV_SERIALIZE_MAP()
        KV_SERIALIZE(current_extra_message_index)
      END_KV_SERIALIZE_MAP()
    };


    volatile uint32_t m_stop;
    epee::critical_section m_template_lock;
    block m_template;
    std::atomic<uint32_t> m_template_no;
    std::atomic<uint32_t> m_starter_nonce;
    difficulty_type m_diffic;
    uint64_t m_height;
    volatile uint32_t m_thread_index; 
    volatile uint32_t m_threads_total;
    std::atomic<uint32_t> m_threads_active;
    std::atomic<int32_t> m_pausers_count;
    epee::critical_section m_miners_count_lock;

    std::list<boost::thread> m_threads;
    epee::critical_section m_threads_lock;
    i_miner_handler* m_phandler;
    get_block_hash_t m_gbh;
    account_public_address m_mine_address;
    epee::math_helper::once_a_time_seconds<5> m_update_block_template_interval;
    epee::math_helper::once_a_time_seconds<2> m_update_merge_hr_interval;
    epee::math_helper::once_a_time_seconds<1> m_autodetect_interval;
    std::vector<blobdata> m_extra_messages;
    miner_config m_config;
    std::string m_config_folder_path;    
    std::atomic<uint64_t> m_last_hr_merge_time;
    std::atomic<uint64_t> m_hashes;
    std::atomic<uint64_t> m_total_hashes;
    std::atomic<uint64_t> m_current_hash_rate;
    epee::critical_section m_last_hash_rates_lock;
    std::list<uint64_t> m_last_hash_rates;
    bool m_do_print_hashrate;
    bool m_do_mining;
    std::vector<std::pair<uint64_t, uint64_t>> m_threads_autodetect;
    boost::thread::attributes m_attrs;

    // background mining stuffs ..

    bool set_is_background_mining_enabled(bool is_background_mining_enabled);
    void set_ignore_battery(bool ignore_battery);
    bool background_worker_thread();
    std::atomic<bool> m_is_background_mining_enabled;
    bool m_ignore_battery;
    boost::mutex m_is_background_mining_enabled_mutex;
    boost::condition_variable m_is_background_mining_enabled_cond;
    std::atomic<bool> m_is_background_mining_started;
    boost::mutex m_is_background_mining_started_mutex;
    boost::condition_variable m_is_background_mining_started_cond;    
    boost::thread m_background_mining_thread;
    uint64_t m_min_idle_seconds;
    uint8_t m_idle_threshold;
    uint8_t m_mining_target;
    std::atomic<uint64_t> m_miner_extra_sleep;
    static bool get_system_times(uint64_t& total_time, uint64_t& idle_time);
    static bool get_process_time(uint64_t& total_time);
    static uint8_t get_percent_of_total(uint64_t some_time, uint64_t total_time);
    static boost::logic::tribool on_battery_power();
    std::atomic<uint64_t> m_block_reward;
  };
}
