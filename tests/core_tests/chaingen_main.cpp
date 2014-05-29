// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chaingen.h"
#include "chaingen_tests_list.h"
#include "common/command_line.h"
#include "transaction_tests.h"

namespace po = boost::program_options;

namespace
{
  const command_line::arg_descriptor<std::string> arg_test_data_path              = {"test_data_path", "", ""};
  const command_line::arg_descriptor<bool>        arg_generate_test_data          = {"generate_test_data", ""};
  const command_line::arg_descriptor<bool>        arg_play_test_data              = {"play_test_data", ""};
  const command_line::arg_descriptor<bool>        arg_generate_and_play_test_data = {"generate_and_play_test_data", ""};
  const command_line::arg_descriptor<bool>        arg_test_transactions           = {"test_transactions", ""};
}

int main(int argc, char* argv[])
{
  TRY_ENTRY();
  epee::string_tools::set_module_name_and_folder(argv[0]);

  //set up logging options
  epee::log_space::get_set_log_detalisation_level(true, LOG_LEVEL_3);
  epee::log_space::log_singletone::add_logger(LOGGER_CONSOLE, NULL, NULL, LOG_LEVEL_2);
  
  epee::log_space::log_singletone::add_logger(LOGGER_FILE, 
    epee::log_space::log_singletone::get_default_log_file().c_str(), 
    epee::log_space::log_singletone::get_default_log_folder().c_str());

  po::options_description desc_options("Allowed options");
  command_line::add_arg(desc_options, command_line::arg_help);
  command_line::add_arg(desc_options, arg_test_data_path);
  command_line::add_arg(desc_options, arg_generate_test_data);
  command_line::add_arg(desc_options, arg_play_test_data);
  command_line::add_arg(desc_options, arg_generate_and_play_test_data);
  command_line::add_arg(desc_options, arg_test_transactions);

  po::variables_map vm;
  bool r = command_line::handle_error_helper(desc_options, [&]()
  {
    po::store(po::parse_command_line(argc, argv, desc_options), vm);
    po::notify(vm);
    return true;
  });
  if (!r)
    return 1;

  if (command_line::get_arg(vm, command_line::arg_help))
  {
    std::cout << desc_options << std::endl;
    return 0;
  }

  size_t tests_count = 0;
  std::vector<std::string> failed_tests;
  std::string tests_folder = command_line::get_arg(vm, arg_test_data_path);
  if (command_line::get_arg(vm, arg_generate_test_data))
  {
    GENERATE("chain001.dat", gen_simple_chain_001);
  }
  else if (command_line::get_arg(vm, arg_play_test_data))
  {
    PLAY("chain001.dat", gen_simple_chain_001);
  }
  else if (command_line::get_arg(vm, arg_generate_and_play_test_data))
  {
    GENERATE_AND_PLAY(gen_simple_chain_001);
    GENERATE_AND_PLAY(gen_simple_chain_split_1);
    GENERATE_AND_PLAY(one_block);
    GENERATE_AND_PLAY(gen_chain_switch_1);
    GENERATE_AND_PLAY(gen_ring_signature_1);
    GENERATE_AND_PLAY(gen_ring_signature_2);
    //GENERATE_AND_PLAY(gen_ring_signature_big); // Takes up to XXX hours (if CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW == 10)

    // Block verification tests
    GENERATE_AND_PLAY(gen_block_big_major_version);
    GENERATE_AND_PLAY(gen_block_big_minor_version);
    GENERATE_AND_PLAY(gen_block_ts_not_checked);
    GENERATE_AND_PLAY(gen_block_ts_in_past);
    GENERATE_AND_PLAY(gen_block_ts_in_future);
    GENERATE_AND_PLAY(gen_block_invalid_prev_id);
    GENERATE_AND_PLAY(gen_block_invalid_nonce);
    GENERATE_AND_PLAY(gen_block_no_miner_tx);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_low);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_high);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_timestamp_in_past);
    GENERATE_AND_PLAY(gen_block_unlock_time_is_timestamp_in_future);
    GENERATE_AND_PLAY(gen_block_height_is_low);
    GENERATE_AND_PLAY(gen_block_height_is_high);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_2_tx_gen_in);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_2_in);
    GENERATE_AND_PLAY(gen_block_miner_tx_with_txin_to_key);
    GENERATE_AND_PLAY(gen_block_miner_tx_out_is_small);
    GENERATE_AND_PLAY(gen_block_miner_tx_out_is_big);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_no_out);
    GENERATE_AND_PLAY(gen_block_miner_tx_has_out_to_alice);
    GENERATE_AND_PLAY(gen_block_has_invalid_tx);
    GENERATE_AND_PLAY(gen_block_is_too_big);
    GENERATE_AND_PLAY(gen_block_invalid_binary_format); // Takes up to 3 hours, if CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW == 500, up to 30 minutes, if CRYPTONOTE_MINED_MONEY_UNLOCK_WINDOW == 10

    // Transaction verification tests
    GENERATE_AND_PLAY(gen_tx_big_version);
    GENERATE_AND_PLAY(gen_tx_unlock_time);
    GENERATE_AND_PLAY(gen_tx_input_is_not_txin_to_key);
    GENERATE_AND_PLAY(gen_tx_no_inputs_no_outputs);
    GENERATE_AND_PLAY(gen_tx_no_inputs_has_outputs);
    GENERATE_AND_PLAY(gen_tx_has_inputs_no_outputs);
    GENERATE_AND_PLAY(gen_tx_invalid_input_amount);
    GENERATE_AND_PLAY(gen_tx_input_wo_key_offsets);
    GENERATE_AND_PLAY(gen_tx_sender_key_offest_not_exist);
    GENERATE_AND_PLAY(gen_tx_key_offest_points_to_foreign_key);
    GENERATE_AND_PLAY(gen_tx_mixed_key_offest_not_exist);
    GENERATE_AND_PLAY(gen_tx_key_image_not_derive_from_tx_key);
    GENERATE_AND_PLAY(gen_tx_key_image_is_invalid);
    GENERATE_AND_PLAY(gen_tx_check_input_unlock_time);
    GENERATE_AND_PLAY(gen_tx_txout_to_key_has_invalid_key);
    GENERATE_AND_PLAY(gen_tx_output_with_zero_amount);
    GENERATE_AND_PLAY(gen_tx_output_is_not_txout_to_key);
    GENERATE_AND_PLAY(gen_tx_signatures_are_invalid);

    // Double spend
    GENERATE_AND_PLAY(gen_double_spend_in_tx<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_tx<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_the_same_block<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_the_same_block<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_blocks<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_blocks<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_different_chains);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_the_same_block<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_the_same_block<true>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_different_blocks<false>);
    GENERATE_AND_PLAY(gen_double_spend_in_alt_chain_in_different_blocks<true>);

    GENERATE_AND_PLAY(gen_uint_overflow_1);
    GENERATE_AND_PLAY(gen_uint_overflow_2);

    GENERATE_AND_PLAY(gen_block_reward);

    std::cout << (failed_tests.empty() ? concolor::green : concolor::magenta);
    std::cout << "\nREPORT:\n";
    std::cout << "  Test run: " << tests_count << '\n';
    std::cout << "  Failures: " << failed_tests.size() << '\n';
    if (!failed_tests.empty())
    {
      std::cout << "FAILED TESTS:\n";
      BOOST_FOREACH(auto test_name, failed_tests)
      {
        std::cout << "  " << test_name << '\n';
      }
    }
    std::cout << concolor::normal << std::endl;
  }
  else if (command_line::get_arg(vm, arg_test_transactions))
  {
    CALL_TEST("TRANSACTIONS TESTS", test_transactions);
  }
  else
  {
    std::cout << concolor::magenta << "Wrong arguments" << concolor::normal << std::endl;
    std::cout << desc_options << std::endl;
    return 2;
  }

  return failed_tests.empty() ? 0 : 1;

  CATCH_ENTRY_L0("main", 1);
}
