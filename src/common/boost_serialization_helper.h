// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>


namespace tools
{
  template<class t_object>
  bool serialize_obj_to_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();
    std::ofstream data_file;
    data_file.open( file_path , std::ios_base::binary | std::ios_base::out| std::ios::trunc);
    if(data_file.fail())
      return false;

    boost::archive::binary_oarchive a(data_file);
    a << obj;

    return !data_file.fail();
    CATCH_ENTRY_L0("serialize_obj_to_file", false);
  }

  template<class t_object>
  bool unserialize_obj_from_file(t_object& obj, const std::string& file_path)
  {
    TRY_ENTRY();

    std::ifstream data_file;  
    data_file.open( file_path, std::ios_base::binary | std::ios_base::in);
    if(data_file.fail())
      return false;
    boost::archive::binary_iarchive a(data_file);

    a >> obj;
    return !data_file.fail();
    CATCH_ENTRY_L0("unserialize_obj_from_file", false);
  }
}
