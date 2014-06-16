// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "common/command_line.h"
#include "misc_log_ex.h"
#include "simpleminer.h"
#include "target_helper.h"
#include "net/http_server_handlers_map2.h"
#include "simpleminer_protocol_defs.h"
#include "storages/http_abstract_invoke.h"
#include "string_tools.h"
#include "cryptonote_core/account.h"
#include "cryptonote_core/cryptonote_format_utils.h"

using namespace epee;
namespace po = boost::program_options;

int main(int argc, char** argv)
{
  string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  log_space::get_set_log_detalisation_level(true, LOG_LEVEL_4);
  log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL);
  log_space::log_singletone::add_logger(LOGGER_FILE,
    log_space::log_singletone::get_default_log_file().c_str(),
    log_space::log_singletone::get_default_log_folder().c_str());

  po::options_description desc("Allowed options");
  command_line::add_arg(desc, command_line::arg_help);
  mining::simpleminer::init_options(desc);

  //uint32_t t = mining::get_target_for_difficulty(700000);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (command_line::get_arg(vm, command_line::arg_help))
    {
      std::cout << desc << std::endl;
      return false;
    }

    return true;
  });
  if (!r)
    return 1;

  mining::simpleminer miner;
  r = miner.init(vm);
  r = r && miner.run(); // Never returns...

  return 0;
}



namespace mining
{
  const command_line::arg_descriptor<std::string, true> arg_pool_addr = {"pool-addr", ""};
  const command_line::arg_descriptor<std::string, true> arg_login = {"login", ""};
  const command_line::arg_descriptor<std::string, true> arg_pass = {"pass", ""};

  //-----------------------------------------------------------------------------------------------------
  void simpleminer::init_options(boost::program_options::options_description& desc)
  {
    command_line::add_arg(desc, arg_pool_addr);
    command_line::add_arg(desc, arg_login);
    command_line::add_arg(desc, arg_pass);
  }
  bool simpleminer::init(const boost::program_options::variables_map& vm)
  {
    std::string pool_addr = command_line::get_arg(vm, arg_pool_addr);
    //parse ip and address
    std::string::size_type p = pool_addr.find(':');
    CHECK_AND_ASSERT_MES(p != std::string::npos && (p + 1 != pool_addr.size()), false, "Wrong srv address syntax");
    m_pool_ip = pool_addr.substr(0, p);
    m_pool_port = pool_addr.substr(p + 1, pool_addr.size());
    m_login = command_line::get_arg(vm, arg_login);
    m_pass = command_line::get_arg(vm, arg_pass);
    return true;
  }

  bool simpleminer::text_job_details_to_native_job_details(const job_details& job, simpleminer::job_details_native& native_details)
  {
    bool r = epee::string_tools::parse_hexstr_to_binbuff(job.blob, native_details.blob);
    CHECK_AND_ASSERT_MES(r, false, "wrong buffer sent from pool server");
    r = epee::string_tools::parse_tpod_from_hex_string(job.target, native_details.target);
    CHECK_AND_ASSERT_MES(r, false, "wrong buffer sent from pool server");
    native_details.job_id = job.job_id;
    return true;
  }

  bool simpleminer::run()
  {
    std::string pool_session_id;
    simpleminer::job_details_native job = AUTO_VAL_INIT(job);
    uint64_t last_job_ticks = 0;

    while(true)
    {
      //-----------------
      last_job_ticks = epee::misc_utils::get_tick_count();
      if(!m_http_client.is_connected())
      {
        LOG_PRINT_L0("Connecting " << m_pool_ip << ":" << m_pool_port << "....");
        if(!m_http_client.connect(m_pool_ip, m_pool_port, 20000))
        {
          LOG_PRINT_L0("Failed to connect " << m_pool_ip << ":" << m_pool_port << ", sleep....");
          epee::misc_utils::sleep_no_w(1000);
          continue;
        }
        //DO AUTH
        LOG_PRINT_L0("Connected " << m_pool_ip << ":" << m_pool_port << " OK");
        COMMAND_RPC_LOGIN::request req = AUTO_VAL_INIT(req);
        req.login = m_login;
        req.pass = m_pass;
        req.agent = "simpleminer/0.1";
        COMMAND_RPC_LOGIN::response resp = AUTO_VAL_INIT(resp);
        if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_LOGIN>("/", req, resp, m_http_client))
        {
          LOG_PRINT_L0("Failed to invoke login " << m_pool_ip << ":" << m_pool_port << ", disconnect and sleep....");
          m_http_client.disconnect();
          epee::misc_utils::sleep_no_w(1000);
          continue;
        }
        if(resp.status != "OK" || resp.id.empty())
        {
          LOG_PRINT_L0("Failed to login " << m_pool_ip << ":" << m_pool_port << ", disconnect and sleep....");
          m_http_client.disconnect();
          epee::misc_utils::sleep_no_w(1000);
          continue;
        }
        pool_session_id = resp.id;
        //78
        if (resp.job.blob.empty() && resp.job.target.empty() && resp.job.job_id.empty())
        {
            LOG_PRINT_L0("Job didn't change");
            continue;
        }
        else if(!text_job_details_to_native_job_details(resp.job, job))
        {
          LOG_PRINT_L0("Failed to text_job_details_to_native_job_details(), disconnect and sleep....");
          m_http_client.disconnect();
          epee::misc_utils::sleep_no_w(1000);
          continue;
        }
        last_job_ticks = epee::misc_utils::get_tick_count();

      }
      while(epee::misc_utils::get_tick_count() - last_job_ticks < 20000)
      {
        //uint32_t c = (*((uint32_t*)&job.blob.data()[39]));
        ++(*((uint32_t*)&job.blob.data()[39]));
        crypto::hash h = cryptonote::null_hash;
        crypto::cn_slow_hash(job.blob.data(), job.blob.size(), h);
        if(  ((uint32_t*)&h)[7] < job.target )
        {
          //found!
          
          COMMAND_RPC_SUBMITSHARE::request submit_request = AUTO_VAL_INIT(submit_request);
          COMMAND_RPC_SUBMITSHARE::response submit_response = AUTO_VAL_INIT(submit_response);
          submit_request.id     = pool_session_id;
          submit_request.job_id = job.job_id;
          submit_request.nonce  = epee::string_tools::pod_to_hex((*((uint32_t*)&job.blob.data()[39])));
          submit_request.result = epee::string_tools::pod_to_hex(h);
          LOG_PRINT_L0("Share found: nonce=" << submit_request.nonce << " for job=" << job.job_id << ", submitting...");
          if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_SUBMITSHARE>("/", submit_request, submit_response, m_http_client))
          {
            LOG_PRINT_L0("Failed to submit share! disconnect and sleep....");
            m_http_client.disconnect();
            epee::misc_utils::sleep_no_w(1000);
            break;
          }
          if(submit_response.status != "OK")
          {
            LOG_PRINT_L0("Failed to submit share! (submitted share rejected) disconnect and sleep....");
            m_http_client.disconnect();
            epee::misc_utils::sleep_no_w(1000);
            break;
          }
          LOG_PRINT_GREEN("Share submitted successfully!", LOG_LEVEL_0);
          break;
        }
      }
      //get next job
      COMMAND_RPC_GETJOB::request getjob_request = AUTO_VAL_INIT(getjob_request);
      COMMAND_RPC_GETJOB::response getjob_response = AUTO_VAL_INIT(getjob_response);
      getjob_request.id = pool_session_id;
      LOG_PRINT_L0("Getting next job...");
      if(!epee::net_utils::invoke_http_json_rpc<mining::COMMAND_RPC_GETJOB>("/", getjob_request, getjob_response, m_http_client))
      {
        LOG_PRINT_L0("Can't get new job! Disconnect and sleep....");
        m_http_client.disconnect();
        epee::misc_utils::sleep_no_w(1000);
        continue;
      }
      if (getjob_response.blob.empty() && getjob_response.target.empty() && getjob_response.job_id.empty())
      {
        LOG_PRINT_L0("Job didn't change");
        continue;
      }
      else if(!text_job_details_to_native_job_details(getjob_response, job))
      {
        LOG_PRINT_L0("Failed to text_job_details_to_native_job_details(), disconnect and sleep....");
        m_http_client.disconnect();
        epee::misc_utils::sleep_no_w(1000);
        continue;
      }
      last_job_ticks = epee::misc_utils::get_tick_count();
    }

    return true;

  }
}
