// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.



bool transactions_flow_test(std::string& working_folder, 
                            std::string path_source_wallet, 
                            std::string path_terget_wallet, 
                            std::string& daemon_addr_a, 
                            std::string& daemon_addr_b, 
                            uint64_t amount_to_transfer, size_t mix_in_factor, size_t transactions_count, size_t transactions_per_second);
