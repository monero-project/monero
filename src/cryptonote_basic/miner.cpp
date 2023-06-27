// Copyright (c) 2014-2022, The Monero Project
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

#include <sstream>
#include <numeric>
#include <boost/algorithm/string.hpp>
#include "misc_language.h"
#include "syncobj.h"
#include "cryptonote_basic_impl.h"
#include "cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "file_io_utils.h"
#include "common/command_line.h"
#include "common/util.h"
#include "string_coding.h"
#include "string_tools.h"
#include "storages/portable_storage_template_helper.h"
#include "boost/logic/tribool.hpp"
#include <boost/filesystem.hpp>

#ifdef __APPLE__
  #include <sys/times.h>
  #include <IOKit/IOKitLib.h>
  #include <IOKit/ps/IOPSKeys.h>
  #include <IOKit/ps/IOPowerSources.h>
  #include <mach/mach_host.h>
  #include <AvailabilityMacros.h>
  #include <TargetConditionals.h>
#elif defined(__linux__)
  #include <unistd.h>
  #include <sys/resource.h>
  #include <sys/times.h>
  #include <time.h>
#elif defined(__FreeBSD__)
  #include <devstat.h>
  #include <errno.h>
  #include <fcntl.h>
#if defined(__amd64__) || defined(__i386__) || defined(__x86_64__)
  #include <machine/apm_bios.h>
#endif
  #include <stdio.h>
  #include <sys/resource.h>
  #include <sys/sysctl.h>
  #include <sys/times.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "miner"

#define AUTODETECT_WINDOW 10 // seconds
#define AUTODETECT_GAIN_THRESHOLD 1.02f  // 2%

using namespace epee;

#include "miner.h"
#include "crypto/hash.h"


extern "C" void slow_hash_allocate_state();
extern "C" void slow_hash_free_state();
namespace cryptonote
{

  namespace
  {
    const command_line::arg_descriptor<std::string> arg_extra_messages =  {"extra-messages-file", "Specify file for extra messages to include into coinbase transactions", "", true};
    const command_line::arg_descriptor<std::string> arg_start_mining =    {"start-mining", "Specify wallet address to mining for", "", true};
    const command_line::arg_descriptor<uint32_t>      arg_mining_threads =  {"mining-threads", "Specify mining threads count", 0, true};
    const command_line::arg_descriptor<bool>        arg_bg_mining_enable =  {"bg-mining-enable", "enable background mining", true, true};
    const command_line::arg_descriptor<bool>        arg_bg_mining_ignore_battery =  {"bg-mining-ignore-battery", "if true, assumes plugged in when unable to query system power status", false, true};    
    const command_line::arg_descriptor<uint64_t>    arg_bg_mining_min_idle_interval_seconds =  {"bg-mining-min-idle-interval", "Specify min lookback interval in seconds for determining idle state", miner::BACKGROUND_MINING_DEFAULT_MIN_IDLE_INTERVAL_IN_SECONDS, true};
    const command_line::arg_descriptor<uint16_t>     arg_bg_mining_idle_threshold_percentage =  {"bg-mining-idle-threshold", "Specify minimum avg idle percentage over lookback interval", miner::BACKGROUND_MINING_DEFAULT_IDLE_THRESHOLD_PERCENTAGE, true};
    const command_line::arg_descriptor<uint16_t>     arg_bg_mining_miner_target_percentage =  {"bg-mining-miner-target", "Specify maximum percentage cpu use by miner(s)", miner::BACKGROUND_MINING_DEFAULT_MINING_TARGET_PERCENTAGE, true};
  }


  miner::miner(i_miner_handler* phandler, const get_block_hash_t &gbh):m_stop(1),
    m_template{},
    m_template_no(0),
    m_diffic(0),
    m_thread_index(0),
    m_phandler(phandler),
    m_gbh(gbh),
    m_height(0),
    m_threads_active(0),
    m_pausers_count(0),
    m_threads_total(0),
    m_starter_nonce(0),
    m_last_hr_merge_time(0),
    m_hashes(0),
    m_total_hashes(0),
    m_do_print_hashrate(false),
    m_do_mining(false),
    m_current_hash_rate(0),
    m_is_background_mining_enabled(false),
    m_min_idle_seconds(BACKGROUND_MINING_DEFAULT_MIN_IDLE_INTERVAL_IN_SECONDS),
    m_idle_threshold(BACKGROUND_MINING_DEFAULT_IDLE_THRESHOLD_PERCENTAGE),
    m_mining_target(BACKGROUND_MINING_DEFAULT_MINING_TARGET_PERCENTAGE),
    m_miner_extra_sleep(BACKGROUND_MINING_DEFAULT_MINER_EXTRA_SLEEP_MILLIS),
    m_block_reward(0)
  {
    m_attrs.set_stack_size(THREAD_STACK_SIZE);
  }
  //-----------------------------------------------------------------------------------------------------
  miner::~miner()
  {
    try { stop(); }
    catch (...) { /* ignore */ }
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_block_template(const block& bl, const difficulty_type& di, uint64_t height, uint64_t block_reward)
  {
    CRITICAL_REGION_LOCAL(m_template_lock);
    m_template = bl;
    m_diffic = di;
    m_height = height;
    m_block_reward = block_reward;
    ++m_template_no;
    m_starter_nonce = crypto::rand<uint32_t>();
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_block_chain_update()
  {
    if(!is_mining())
      return true;

    return request_block_template();
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::request_block_template()
  {
    block bl;
    difficulty_type di = AUTO_VAL_INIT(di);
    uint64_t height = AUTO_VAL_INIT(height);
    uint64_t expected_reward; //only used for RPC calls - could possibly be useful here too?

    cryptonote::blobdata extra_nonce;
    if(m_extra_messages.size() && m_config.current_extra_message_index < m_extra_messages.size())
    {
      extra_nonce = m_extra_messages[m_config.current_extra_message_index];
    }

    uint64_t seed_height;
    crypto::hash seed_hash;
    if(!m_phandler->get_block_template(bl, m_mine_address, di, height, expected_reward, extra_nonce, seed_height, seed_hash))
    {
      LOG_ERROR("Failed to get_block_template(), stopping mining");
      return false;
    }
    set_block_template(bl, di, height, expected_reward);
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::on_idle()
  {
    m_update_block_template_interval.do_call([&](){
      if(is_mining())request_block_template();
      return true;
    });

    m_update_merge_hr_interval.do_call([&](){
      merge_hr();
      return true;
    });

    m_autodetect_interval.do_call([&](){
      update_autodetection();
      return true;
    });

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::do_print_hashrate(bool do_hr)
  {
    m_do_print_hashrate = do_hr;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::merge_hr()
  {
    if(m_last_hr_merge_time && is_mining())
    {
      m_current_hash_rate = m_hashes * 1000 / ((misc_utils::get_tick_count() - m_last_hr_merge_time + 1));
      CRITICAL_REGION_LOCAL(m_last_hash_rates_lock);
      m_last_hash_rates.push_back(m_current_hash_rate);
      if(m_last_hash_rates.size() > 19)
        m_last_hash_rates.pop_front();
      if(m_do_print_hashrate)
      {
        uint64_t total_hr = std::accumulate(m_last_hash_rates.begin(), m_last_hash_rates.end(), 0);
        float hr = static_cast<float>(total_hr)/static_cast<float>(m_last_hash_rates.size());
        const auto flags = std::cout.flags();
        const auto precision = std::cout.precision();
        std::cout << "hashrate: " << std::setprecision(4) << std::fixed << hr << std::setiosflags(flags) << std::setprecision(precision) << ENDL;
      }
    }
    m_last_hr_merge_time = misc_utils::get_tick_count();
    m_hashes = 0;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::update_autodetection()
  {
    if (m_threads_autodetect.empty())
      return;

    uint64_t now = epee::misc_utils::get_ns_count();
    uint64_t dt = now - m_threads_autodetect.back().first;
    if (dt < AUTODETECT_WINDOW * 1000000000ull)
      return;

    // work out how many more hashes we got
    m_threads_autodetect.back().first = dt;
    uint64_t dh = m_total_hashes - m_threads_autodetect.back().second;
    m_threads_autodetect.back().second = dh;
    float hs = dh / (dt / (float)1000000000);
    MGINFO("Mining autodetection: " << m_threads_autodetect.size() << " threads: " << hs << " H/s");

    // when we don't increase by at least 2%, stop, otherwise check next
    // if N and N+1 have mostly the same hash rate, we want to "lighter" one
    bool found = false;
    if (m_threads_autodetect.size() > 1)
    {
      int previdx = m_threads_autodetect.size() - 2;
      float previous_hs = m_threads_autodetect[previdx].second / (m_threads_autodetect[previdx].first / (float)1000000000);
      if (previous_hs > 0 && hs / previous_hs < AUTODETECT_GAIN_THRESHOLD)
      {
        m_threads_total = m_threads_autodetect.size() - 1;
        m_threads_autodetect.clear();
        MGINFO("Optimal number of threads seems to be " << m_threads_total);
        found = true;
      }
    }

    if (!found)
    {
      // setup one more thread
      m_threads_autodetect.push_back({now, m_total_hashes});
      m_threads_total = m_threads_autodetect.size();
    }

    // restart all threads
    {
      CRITICAL_REGION_LOCAL(m_threads_lock);
      m_stop = true;
      while (m_threads_active > 0)
        misc_utils::sleep_no_w(100);
      m_threads.clear();
    }
    m_stop = false;
    m_thread_index = 0;
    for(size_t i = 0; i != m_threads_total; i++)
      m_threads.push_back(boost::thread(m_attrs, boost::bind(&miner::worker_thread, this)));
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_extra_messages);
    command_line::add_arg(desc, arg_start_mining);
    command_line::add_arg(desc, arg_mining_threads);
    command_line::add_arg(desc, arg_bg_mining_enable);
    command_line::add_arg(desc, arg_bg_mining_ignore_battery);    
    command_line::add_arg(desc, arg_bg_mining_min_idle_interval_seconds);
    command_line::add_arg(desc, arg_bg_mining_idle_threshold_percentage);
    command_line::add_arg(desc, arg_bg_mining_miner_target_percentage);
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::init(const boost::program_options::variables_map& vm, network_type nettype)
  {
    if(command_line::has_arg(vm, arg_extra_messages))
    {
      std::string buff;
      bool r = file_io_utils::load_file_to_string(command_line::get_arg(vm, arg_extra_messages), buff);
      CHECK_AND_ASSERT_MES(r, false, "Failed to load file with extra messages: " << command_line::get_arg(vm, arg_extra_messages));
      std::vector<std::string> extra_vec;
      boost::split(extra_vec, buff, boost::is_any_of("\n"), boost::token_compress_on );
      m_extra_messages.resize(extra_vec.size());
      for(size_t i = 0; i != extra_vec.size(); i++)
      {
        string_tools::trim(extra_vec[i]);
        if(!extra_vec[i].size())
          continue;
        std::string buff = string_encoding::base64_decode(extra_vec[i]);
        if(buff != "0")
          m_extra_messages[i] = buff;
      }
      m_config_folder_path = boost::filesystem::path(command_line::get_arg(vm, arg_extra_messages)).parent_path().string();
      m_config = AUTO_VAL_INIT(m_config);
      const std::string filename = m_config_folder_path + "/" + MINER_CONFIG_FILE_NAME;
      CHECK_AND_ASSERT_MES(epee::serialization::load_t_from_json_file(m_config, filename), false, "Failed to load data from " << filename);
      MINFO("Loaded " << m_extra_messages.size() << " extra messages, current index " << m_config.current_extra_message_index);
    }

    if(command_line::has_arg(vm, arg_start_mining))
    {
      address_parse_info info;
      if(!cryptonote::get_account_address_from_str(info, nettype, command_line::get_arg(vm, arg_start_mining)) || info.is_subaddress)
      {
        LOG_ERROR("Target account address " << command_line::get_arg(vm, arg_start_mining) << " has wrong format, starting daemon canceled");
        return false;
      }
      m_mine_address = info.address;
      m_threads_total = 1;
      m_do_mining = true;
      if(command_line::has_arg(vm, arg_mining_threads))
      {
        m_threads_total = command_line::get_arg(vm, arg_mining_threads);
      }
    }

    // Background mining parameters
    // Let init set all parameters even if background mining is not enabled, they can start later with params set
    if(command_line::has_arg(vm, arg_bg_mining_enable))
      set_is_background_mining_enabled( command_line::get_arg(vm, arg_bg_mining_enable) );
    if(command_line::has_arg(vm, arg_bg_mining_ignore_battery))
      set_ignore_battery( command_line::get_arg(vm, arg_bg_mining_ignore_battery) );      
    if(command_line::has_arg(vm, arg_bg_mining_min_idle_interval_seconds))
      set_min_idle_seconds( command_line::get_arg(vm, arg_bg_mining_min_idle_interval_seconds) );
    if(command_line::has_arg(vm, arg_bg_mining_idle_threshold_percentage))
      set_idle_threshold( command_line::get_arg(vm, arg_bg_mining_idle_threshold_percentage) );
    if(command_line::has_arg(vm, arg_bg_mining_miner_target_percentage))
      set_mining_target( command_line::get_arg(vm, arg_bg_mining_miner_target_percentage) );

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::is_mining() const
  {
    return !m_stop;
  }
  //-----------------------------------------------------------------------------------------------------
  const account_public_address& miner::get_mining_address() const
  {
    return m_mine_address;
  }
  //-----------------------------------------------------------------------------------------------------
  uint32_t miner::get_threads_count() const {
    return m_threads_total;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::start(const account_public_address& adr, size_t threads_count, bool do_background, bool ignore_battery)
  {
    m_block_reward = 0;
    m_mine_address = adr;
    m_threads_total = static_cast<uint32_t>(threads_count);
    if (threads_count == 0)
    {
      m_threads_autodetect.clear();
      m_threads_autodetect.push_back({epee::misc_utils::get_ns_count(), m_total_hashes});
      m_threads_total = 1;
    }
    m_starter_nonce = crypto::rand<uint32_t>();
    CRITICAL_REGION_LOCAL(m_threads_lock);
    if(is_mining())
    {
      LOG_ERROR("Starting miner but it's already started");
      return false;
    }

    if(!m_threads.empty())
    {
      LOG_ERROR("Unable to start miner because there are active mining threads");
      return false;
    }

    request_block_template();//lets update block template

    m_stop = false;
    m_thread_index = 0;
    set_is_background_mining_enabled(do_background);
    set_ignore_battery(ignore_battery);
    
    for(size_t i = 0; i != m_threads_total; i++)
    {
      m_threads.push_back(boost::thread(m_attrs, boost::bind(&miner::worker_thread, this)));
    }

    if (threads_count == 0)
      MINFO("Mining has started, autodetecting optimal number of threads, good luck!" );
    else
      MINFO("Mining has started with " << threads_count << " threads, good luck!" );

    if( get_is_background_mining_enabled() )
    {
      m_background_mining_thread = boost::thread(m_attrs, boost::bind(&miner::background_worker_thread, this));
      LOG_PRINT_L0("Background mining controller thread started" );
    }

    if(get_ignore_battery())
    {
      MINFO("Ignoring battery");
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  uint64_t miner::get_speed() const
  {
    if(is_mining()) {
      return m_current_hash_rate;
    }
    else {
      return 0;
    }
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::send_stop_signal()
  {
    m_stop = true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::stop()
  {
    MTRACE("Miner has received stop signal");

    CRITICAL_REGION_LOCAL(m_threads_lock);
    bool mining = !m_threads.empty();
    if (!mining)
    {
      MTRACE("Not mining - nothing to stop" );
      return true;
    }

    send_stop_signal();

    // In case background mining was active and the miner threads are waiting
    // on the background miner to signal start. 
    while (m_threads_active > 0)
    {
      m_is_background_mining_started_cond.notify_all();
      misc_utils::sleep_no_w(100);
    }

    // The background mining thread could be sleeping for a long time, so we
    // interrupt it just in case
    m_background_mining_thread.interrupt();
    m_background_mining_thread.join();
    m_is_background_mining_enabled = false;

    MINFO("Mining has been stopped, " << m_threads.size() << " finished" );
    m_threads.clear();
    m_threads_autodetect.clear();
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::find_nonce_for_given_block(const get_block_hash_t &gbh, block& bl, const difficulty_type& diffic, uint64_t height, const crypto::hash *seed_hash)
  {
    for(; bl.nonce != std::numeric_limits<uint32_t>::max(); bl.nonce++)
    {
      crypto::hash h;
      gbh(bl, height, seed_hash, diffic <= 100 ? 0 : tools::get_max_concurrency(), h);

      if(check_hash(h, diffic))
      {
        bl.invalidate_hashes();
        return true;
      }
    }
    bl.invalidate_hashes();
    return false;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::on_synchronized()
  {
    if(m_do_mining)
    {
      start(m_mine_address, m_threads_total, get_is_background_mining_enabled(), get_ignore_battery());
    }
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::pause()
  {
    CRITICAL_REGION_LOCAL(m_miners_count_lock);
    MDEBUG("miner::pause: " << m_pausers_count << " -> " << (m_pausers_count + 1));
    ++m_pausers_count;
    if(m_pausers_count == 1 && is_mining())
      MDEBUG("MINING PAUSED");
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::resume()
  {
    CRITICAL_REGION_LOCAL(m_miners_count_lock);
    MDEBUG("miner::resume: " << m_pausers_count << " -> " << (m_pausers_count - 1));
    --m_pausers_count;
    if(m_pausers_count < 0)
    {
      m_pausers_count = 0;
      MERROR("Unexpected miner::resume() called");
    }
    if(!m_pausers_count && is_mining())
      MDEBUG("MINING RESUMED");
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::worker_thread()
  {
    const uint32_t th_local_index = m_thread_index++; // atomically increment, getting value before increment
    bool rx_set = false;

    MLOG_SET_THREAD_NAME(std::string("[miner ") + std::to_string(th_local_index) + "]");
    MGINFO("Miner thread was started ["<< th_local_index << "]");
    uint32_t nonce = m_starter_nonce + th_local_index;
    uint64_t height = 0;
    difficulty_type local_diff = 0;
    uint32_t local_template_ver = 0;
    block b;
    slow_hash_allocate_state();
    ++m_threads_active;
    while(!m_stop)
    {
      if(m_pausers_count)//anti split workaround
      {
        misc_utils::sleep_no_w(100);
        continue;
      }
      else if( m_is_background_mining_enabled )
      {
        misc_utils::sleep_no_w(m_miner_extra_sleep);
        while( !m_is_background_mining_started )
        {
          MGINFO("background mining is enabled, but not started, waiting until start triggers");
          boost::unique_lock<boost::mutex> started_lock( m_is_background_mining_started_mutex );        
          m_is_background_mining_started_cond.wait( started_lock );
          if( m_stop ) break;
        }
        
        if( m_stop ) continue;         
      }

      if(local_template_ver != m_template_no)
      {
        CRITICAL_REGION_BEGIN(m_template_lock);
        b = m_template;
        local_diff = m_diffic;
        height = m_height;
        CRITICAL_REGION_END();
        local_template_ver = m_template_no;
        nonce = m_starter_nonce + th_local_index;
      }

      if(!local_template_ver)//no any set_block_template call
      {
        LOG_PRINT_L2("Block template not set yet");
        epee::misc_utils::sleep_no_w(1000);
        continue;
      }

      b.nonce = nonce;
      crypto::hash h;

      if ((b.major_version >= RX_BLOCK_VERSION) && !rx_set)
      {
        crypto::rx_set_miner_thread(th_local_index, tools::get_max_concurrency());
        rx_set = true;
      }

      m_gbh(b, height, NULL, tools::get_max_concurrency(), h);

      if(check_hash(h, local_diff))
      {
        //we lucky!
        ++m_config.current_extra_message_index;
        MGINFO_GREEN("Found block " << get_block_hash(b) << " at height " << height << " for difficulty: " << local_diff);
        cryptonote::block_verification_context bvc;
        if(!m_phandler->handle_block_found(b, bvc) || !bvc.m_added_to_main_chain)
        {
          --m_config.current_extra_message_index;
        }else
        {
          //success update, lets update config
          if (!m_config_folder_path.empty())
            epee::serialization::store_t_to_json_file(m_config, m_config_folder_path + "/" + MINER_CONFIG_FILE_NAME);
        }
      }
      nonce+=m_threads_total;
      ++m_hashes;
      ++m_total_hashes;
    }
    slow_hash_free_state();
    MGINFO("Miner thread stopped ["<< th_local_index << "]");
    --m_threads_active;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::get_is_background_mining_enabled() const
  {
    return m_is_background_mining_enabled;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::get_ignore_battery() const
  {
    return m_ignore_battery;
  }  
  //-----------------------------------------------------------------------------------------------------
  /**
  * This has differing behaviour depending on if mining has been started/etc.
  * Note: add documentation
  */
  bool miner::set_is_background_mining_enabled(bool is_background_mining_enabled)
  {
    m_is_background_mining_enabled = is_background_mining_enabled;
    // Extra logic will be required if we make this function public in the future
    // and allow toggling smart mining without start/stop
    //m_is_background_mining_enabled_cond.notify_one();
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  void miner::set_ignore_battery(bool ignore_battery)
  {
    m_ignore_battery = ignore_battery;
  }
  //-----------------------------------------------------------------------------------------------------
  uint64_t miner::get_min_idle_seconds() const
  {
    return m_min_idle_seconds;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_min_idle_seconds(uint64_t min_idle_seconds)
  {
    if(min_idle_seconds > BACKGROUND_MINING_MAX_MIN_IDLE_INTERVAL_IN_SECONDS) return false;
    if(min_idle_seconds < BACKGROUND_MINING_MIN_MIN_IDLE_INTERVAL_IN_SECONDS) return false;
    m_min_idle_seconds = min_idle_seconds;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  uint8_t miner::get_idle_threshold() const
  {
    return m_idle_threshold;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_idle_threshold(uint8_t idle_threshold)
  {
    if(idle_threshold > BACKGROUND_MINING_MAX_IDLE_THRESHOLD_PERCENTAGE) return false;
    if(idle_threshold < BACKGROUND_MINING_MIN_IDLE_THRESHOLD_PERCENTAGE) return false;
    m_idle_threshold = idle_threshold;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  uint8_t miner::get_mining_target() const
  {
    return m_mining_target;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::set_mining_target(uint8_t mining_target)
  {
    if(mining_target > BACKGROUND_MINING_MAX_MINING_TARGET_PERCENTAGE) return false;
    if(mining_target < BACKGROUND_MINING_MIN_MINING_TARGET_PERCENTAGE) return false;
    m_mining_target = mining_target;
    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::background_worker_thread()
  {
    uint64_t prev_total_time, current_total_time;
    uint64_t prev_idle_time, current_idle_time;
    uint64_t previous_process_time = 0, current_process_time = 0;
    m_is_background_mining_started = false;

    if(!get_system_times(prev_total_time, prev_idle_time))
    {
      LOG_ERROR("get_system_times call failed, background mining will NOT work!");
      return false;
    }
    
    while(!m_stop)
    {
        
      try
      {
        // Commenting out the below since we're going with privatizing the bg mining enabled
        // function, but I'll leave the code/comments here for anyone that wants to modify the
        // patch in the future
        // -------------------------------------------------------------------------------------
        // All of this might be overkill if we just enforced some simple requirements
        // about changing this variable before/after the miner starts, but I envision
        // in the future a checkbox that you can tick on/off for background mining after
        // you've clicked "start mining". There's still an issue here where if background
        // mining is disabled when start is called, this thread is never created, and so
        // enabling after does nothing, something I have to fix in the future. However,
        // this should take care of the case where mining is started with bg-enabled, 
        // and then the user decides to un-check background mining, and just do
        // regular full-speed mining. I might just be over-doing it and thinking up 
        // non-existant use-cases, so if the consensus is to simplify, we can remove all this fluff.
        /*
        while( !m_is_background_mining_enabled )
        {
          MGINFO("background mining is disabled, waiting until enabled!");
          boost::unique_lock<boost::mutex> enabled_lock( m_is_background_mining_enabled_mutex );        
          m_is_background_mining_enabled_cond.wait( enabled_lock );
        } 
        */       
        
        // If we're already mining, then sleep for the miner monitor interval.
        // If we're NOT mining, then sleep for the idle monitor interval
        uint64_t sleep_for_seconds = BACKGROUND_MINING_MINER_MONITOR_INVERVAL_IN_SECONDS;
        if( !m_is_background_mining_started ) sleep_for_seconds = get_min_idle_seconds();
        boost::this_thread::sleep_for(boost::chrono::seconds(sleep_for_seconds));
      }
      catch(const boost::thread_interrupted&)
      {
        MDEBUG("background miner thread interrupted ");
        continue; // if interrupted because stop called, loop should end ..
      }

      bool on_ac_power = m_ignore_battery;
      if(!m_ignore_battery)
      {
        boost::tribool battery_powered(on_battery_power());
        if(!indeterminate( battery_powered ))
        {
          on_ac_power = !(bool)battery_powered;
        }
      }

      if( m_is_background_mining_started )
      {
        // figure out if we need to stop, and monitor mining usage
        
        // If we get here, then previous values are initialized.
        // Let's get some current data for comparison.

        if(!get_system_times(current_total_time, current_idle_time))
        {
          MERROR("get_system_times call failed");
          continue;
        }

        if(!get_process_time(current_process_time))
        {
          MERROR("get_process_time call failed!");
          continue;
        }

        uint64_t total_diff = (current_total_time - prev_total_time);
        uint64_t idle_diff = (current_idle_time - prev_idle_time);
        uint64_t process_diff = (current_process_time - previous_process_time);
        uint8_t idle_percentage = get_percent_of_total(idle_diff, total_diff);
        uint8_t process_percentage = get_percent_of_total(process_diff, total_diff);

        MDEBUG("idle percentage is " << unsigned(idle_percentage) << "\%, miner percentage is " << unsigned(process_percentage) << "\%, ac power : " << on_ac_power);
        if( idle_percentage + process_percentage < get_idle_threshold() || !on_ac_power )
        {
          MINFO("cpu is " << unsigned(idle_percentage) << "% idle, idle threshold is " << unsigned(get_idle_threshold()) << "\%, ac power : " << on_ac_power << ", background mining stopping, thanks for your contribution!");
          m_is_background_mining_started = false;

          // reset process times
          previous_process_time = 0;
          current_process_time = 0;
        }
        else
        {
          previous_process_time = current_process_time;

          // adjust the miner extra sleep variable
          int64_t miner_extra_sleep_change = (-1 * (get_mining_target() - process_percentage) );
          int64_t new_miner_extra_sleep = m_miner_extra_sleep + miner_extra_sleep_change;
          // if you start the miner with few threads on a multicore system, this could
          // fall below zero because all the time functions aggregate across all processors.
          // I'm just hard limiting to 5 millis min sleep here, other options?
          m_miner_extra_sleep = std::max( new_miner_extra_sleep , (int64_t)5 );
          MDEBUG("m_miner_extra_sleep " << m_miner_extra_sleep);
        }
        
        prev_total_time = current_total_time;
        prev_idle_time = current_idle_time;
      }
      else if( on_ac_power )
      {
        // figure out if we need to start

        if(!get_system_times(current_total_time, current_idle_time))
        {
          MERROR("get_system_times call failed");
          continue;
        }

        uint64_t total_diff = (current_total_time - prev_total_time);
        uint64_t idle_diff = (current_idle_time - prev_idle_time);
        uint8_t idle_percentage = get_percent_of_total(idle_diff, total_diff);

        MDEBUG("idle percentage is " << unsigned(idle_percentage));
        if( idle_percentage >= get_idle_threshold() && on_ac_power )
        {
          MINFO("cpu is " << unsigned(idle_percentage) << "% idle, idle threshold is " << unsigned(get_idle_threshold()) << "\%, ac power : " << on_ac_power << ", background mining started, good luck!");
          m_is_background_mining_started = true;
          m_is_background_mining_started_cond.notify_all();

          // Wait for a little mining to happen ..
          boost::this_thread::sleep_for(boost::chrono::seconds( 1 ));

          // Starting data ...
          if(!get_process_time(previous_process_time))
          {
            m_is_background_mining_started = false;
            MERROR("get_process_time call failed!");
          }
        }

        prev_total_time = current_total_time;
        prev_idle_time = current_idle_time;
      }
    }

    return true;
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::get_system_times(uint64_t& total_time, uint64_t& idle_time)
  {
    #ifdef _WIN32

    	FILETIME idleTime;
    	FILETIME kernelTime;
    	FILETIME userTime;
    	if ( GetSystemTimes( &idleTime, &kernelTime, &userTime ) != -1 )
    	{
        total_time =
          ( (((uint64_t)(kernelTime.dwHighDateTime)) << 32) | ((uint64_t)kernelTime.dwLowDateTime) )
          + ( (((uint64_t)(userTime.dwHighDateTime)) << 32) | ((uint64_t)userTime.dwLowDateTime) );

        idle_time = ( (((uint64_t)(idleTime.dwHighDateTime)) << 32) | ((uint64_t)idleTime.dwLowDateTime) );

        return true;
    	}

    #elif defined(__linux__)

      const std::string STAT_FILE_PATH = "/proc/stat";

      if( !epee::file_io_utils::is_file_exist(STAT_FILE_PATH) )
      {
        LOG_ERROR("'" << STAT_FILE_PATH << "' file does not exist");
        return false;
      }

      std::ifstream stat_file_stream(STAT_FILE_PATH);
      if( stat_file_stream.fail() )
      {
        LOG_ERROR("failed to open '" << STAT_FILE_PATH << "'");
        return false;
      }

      std::string line;
      std::getline(stat_file_stream, line);
      std::istringstream stat_file_iss(line);
      stat_file_iss.ignore(65536, ' '); // skip cpu label ...
      uint64_t utime, ntime, stime, itime;
      if( !(stat_file_iss >> utime && stat_file_iss >> ntime && stat_file_iss >> stime && stat_file_iss >> itime) )
      {
        LOG_ERROR("failed to read '" << STAT_FILE_PATH << "'");
        return false;
      }

      idle_time = itime;
      total_time = utime + ntime + stime + itime;

      return true;

    #elif defined(__APPLE__)

      mach_msg_type_number_t count;
      kern_return_t status;
      host_cpu_load_info_data_t stats;      
      count = HOST_CPU_LOAD_INFO_COUNT;
      status = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&stats, &count);
      if(status != KERN_SUCCESS)
      {
        return false;
      }

      idle_time = stats.cpu_ticks[CPU_STATE_IDLE];
      total_time = idle_time + stats.cpu_ticks[CPU_STATE_USER] + stats.cpu_ticks[CPU_STATE_SYSTEM];
      
      return true;

    #elif defined(__FreeBSD__)

      struct statinfo s;
      size_t n = sizeof(s.cp_time);
      if( sysctlbyname("kern.cp_time", s.cp_time, &n, NULL, 0) == -1 )
      {
        LOG_ERROR("sysctlbyname(\"kern.cp_time\"): " << strerror(errno));
        return false;
      }
      if( n != sizeof(s.cp_time) )
      {
        LOG_ERROR("sysctlbyname(\"kern.cp_time\") output is unexpectedly "
          << n << " bytes instead of the expected " << sizeof(s.cp_time)
          << " bytes.");
        return false;
      }

      idle_time = s.cp_time[CP_IDLE];
      total_time =
        s.cp_time[CP_USER] +
        s.cp_time[CP_NICE] +
        s.cp_time[CP_SYS] +
        s.cp_time[CP_INTR] +
        s.cp_time[CP_IDLE];

      return true;

    #endif

    return false; // unsupported system
  }
  //-----------------------------------------------------------------------------------------------------
  bool miner::get_process_time(uint64_t& total_time)
  {
    #ifdef _WIN32

      FILETIME createTime;
      FILETIME exitTime;
      FILETIME kernelTime;
      FILETIME userTime;
      if ( GetProcessTimes( GetCurrentProcess(), &createTime, &exitTime, &kernelTime, &userTime ) != -1 )
      {
        total_time =
          ( (((uint64_t)(kernelTime.dwHighDateTime)) << 32) | ((uint64_t)kernelTime.dwLowDateTime) )
          + ( (((uint64_t)(userTime.dwHighDateTime)) << 32) | ((uint64_t)userTime.dwLowDateTime) );

        return true;
      }

    #elif (defined(__linux__) && defined(_SC_CLK_TCK)) || defined(__APPLE__) || defined(__FreeBSD__)

      struct tms tms;
      if ( times(&tms) != (clock_t)-1 )
      {
    		total_time = tms.tms_utime + tms.tms_stime;
        return true;
      }

    #endif

    return false; // unsupported system
  }
  //-----------------------------------------------------------------------------------------------------  
  uint8_t miner::get_percent_of_total(uint64_t other, uint64_t total)
  {
    return (uint8_t)( ceil( (other * 1.f / total * 1.f) * 100) );    
  }
  //-----------------------------------------------------------------------------------------------------    
  boost::logic::tribool miner::on_battery_power()
  {
    #ifdef _WIN32

      SYSTEM_POWER_STATUS power_status;
    	if ( GetSystemPowerStatus( &power_status ) != 0 )
    	{
        return boost::logic::tribool(power_status.ACLineStatus != 1);
    	}

    #elif defined(__APPLE__) 
      
      #if TARGET_OS_MAC && (!defined(MAC_OS_X_VERSION_MIN_REQUIRED) || MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7)
        return boost::logic::tribool(IOPSGetTimeRemainingEstimate() != kIOPSTimeRemainingUnlimited);
      #else
        // iOS or OSX <10.7
        return boost::logic::tribool(boost::logic::indeterminate);
      #endif

    #elif defined(__linux__)

      // Use the power_supply class http://lxr.linux.no/#linux+v4.10.1/Documentation/power/power_supply_class.txt
      std::string power_supply_class_path = "/sys/class/power_supply";

      boost::tribool on_battery = boost::logic::tribool(boost::logic::indeterminate);
      if (boost::filesystem::is_directory(power_supply_class_path))
      {
        const boost::filesystem::directory_iterator end_itr;
        for (boost::filesystem::directory_iterator iter(power_supply_class_path); iter != end_itr; ++iter)
        {
          const boost::filesystem::path& power_supply_path = iter->path();
          if (boost::filesystem::is_directory(power_supply_path))
          {
            boost::filesystem::path power_supply_type_path = power_supply_path / "type";
            if (boost::filesystem::is_regular_file(power_supply_type_path))
            {
              std::ifstream power_supply_type_stream(power_supply_type_path.string());
              if (power_supply_type_stream.fail())
              {
                LOG_PRINT_L0("Unable to read from " << power_supply_type_path << " to check power supply type");
                continue;
              }

              std::string power_supply_type;
              std::getline(power_supply_type_stream, power_supply_type);

              // If there is an AC adapter that's present and online we can break early
              if (boost::starts_with(power_supply_type, "Mains"))
              {
                boost::filesystem::path power_supply_online_path = power_supply_path / "online";
                if (boost::filesystem::is_regular_file(power_supply_online_path))
                {
                  std::ifstream power_supply_online_stream(power_supply_online_path.string());
                  if (power_supply_online_stream.fail())
                  {
                    LOG_PRINT_L0("Unable to read from " << power_supply_online_path << " to check ac power supply status");
                    continue;
                  }

                  if (power_supply_online_stream.get() == '1')
                  {
                    return boost::logic::tribool(false);
                  }
                }
              }
              else if (boost::starts_with(power_supply_type, "Battery") && boost::logic::indeterminate(on_battery))
              {
                boost::filesystem::path power_supply_status_path = power_supply_path / "status";
                if (boost::filesystem::is_regular_file(power_supply_status_path))
                {
                  std::ifstream power_supply_status_stream(power_supply_status_path.string());
                  if (power_supply_status_stream.fail())
                  {
                    LOG_PRINT_L0("Unable to read from " << power_supply_status_path << " to check battery power supply status");
                    continue;
                  }

                  // Possible status are Charging, Full, Discharging, Not Charging, and Unknown
                  // We are only need to handle negative states right now
                  std::string power_supply_status;
                  std::getline(power_supply_status_stream, power_supply_status);
                  if (boost::starts_with(power_supply_status, "Charging") || boost::starts_with(power_supply_status, "Full"))
                  {
                    on_battery = boost::logic::tribool(false);
                  }

                  if (boost::starts_with(power_supply_status, "Discharging"))
                  {
                    on_battery = boost::logic::tribool(true);
                  }
                }
              }
            }
          }
        }
      }

      if (boost::logic::indeterminate(on_battery))
      {
        static bool error_shown = false;
        if (!error_shown)
        {
          LOG_ERROR("couldn't query power status from " << power_supply_class_path);
          error_shown = true;
        }
      }
      return on_battery;

    #elif defined(__FreeBSD__)
      int ac;
      size_t n = sizeof(ac);
      if( sysctlbyname("hw.acpi.acline", &ac, &n, NULL, 0) == -1 )
      {
        if( errno != ENOENT )
        {
          LOG_ERROR("Cannot query battery status: "
            << "sysctlbyname(\"hw.acpi.acline\"): " << strerror(errno));
          return boost::logic::tribool(boost::logic::indeterminate);
        }

        // If sysctl fails with ENOENT, then try querying /dev/apm.

        static const char* dev_apm = "/dev/apm";
        const int fd = open(dev_apm, O_RDONLY);
        if( fd == -1 ) {
          LOG_ERROR("Cannot query battery status: "
            << "open(): " << dev_apm << ": " << strerror(errno));
          return boost::logic::tribool(boost::logic::indeterminate);
        }

#if defined(__amd64__) || defined(__i386__) || defined(__x86_64__)
        apm_info info;
        if( ioctl(fd, APMIO_GETINFO, &info) == -1 ) {
          close(fd);
          LOG_ERROR("Cannot query battery status: "
            << "ioctl(" << dev_apm << ", APMIO_GETINFO): " << strerror(errno));
          return boost::logic::tribool(boost::logic::indeterminate);
        }

        close(fd);

        // See apm(8).
        switch( info.ai_acline )
        {
        case 0: // off-line
        case 2: // backup power
          return boost::logic::tribool(true);
        case 1: // on-line
          return boost::logic::tribool(false);
        }
        switch( info.ai_batt_stat )
        {
        case 0: // high
        case 1: // low
        case 2: // critical
          return boost::logic::tribool(true);
        case 3: // charging
          return boost::logic::tribool(false);
        }

        LOG_ERROR("Cannot query battery status: "
          << "sysctl hw.acpi.acline is not available and /dev/apm returns "
          << "unexpected ac-line status (" << info.ai_acline << ") and "
          << "battery status (" << info.ai_batt_stat << ").");
        return boost::logic::tribool(boost::logic::indeterminate);
      }
      if( n != sizeof(ac) )
      {
        LOG_ERROR("sysctlbyname(\"hw.acpi.acline\") output is unexpectedly "
          << n << " bytes instead of the expected " << sizeof(ac) << " bytes.");
        return boost::logic::tribool(boost::logic::indeterminate);
#endif
      }
      return boost::logic::tribool(ac == 0);
    #endif
    
    LOG_ERROR("couldn't query power status");
    return boost::logic::tribool(boost::logic::indeterminate);
  }
}
