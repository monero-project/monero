// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/serialization/split_free.hpp>
#include <unordered_map>
#include <unordered_set>

namespace boost
{
  namespace serialization
  {
    template <class Archive, class h_key, class hval>
    inline void save(Archive &a, const std::unordered_map<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      size_t s = x.size();
      a << s;
      BOOST_FOREACH(auto& v, x)
      {
        a << v.first;
        a << v.second;
      }
    }

    template <class Archive, class h_key, class hval>
    inline void load(Archive &a, std::unordered_map<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      x.clear();
      size_t s = 0;
      a >> s;
      for(size_t i = 0; i != s; i++)
      {
        h_key k;
        hval v;
        a >> k;
        a >> v;
        x.insert(std::pair<h_key, hval>(k, v));
      }
    }


    template <class Archive, class h_key, class hval>
    inline void save(Archive &a, const std::unordered_multimap<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      size_t s = x.size();
      a << s;
      BOOST_FOREACH(auto& v, x)
      {
        a << v.first;
        a << v.second;
      }
    }

    template <class Archive, class h_key, class hval>
    inline void load(Archive &a, std::unordered_multimap<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      x.clear();
      size_t s = 0;
      a >> s;
      for(size_t i = 0; i != s; i++)
      {
        h_key k;
        hval v;
        a >> k;
        a >> v;
        x.emplace(k, v);
      }
    }


    template <class Archive, class hval>
    inline void save(Archive &a, const std::unordered_set<hval> &x, const boost::serialization::version_type ver)
    {
      size_t s = x.size();
      a << s;
      BOOST_FOREACH(auto& v, x)
      {
        a << v;
      }
    }

    template <class Archive, class hval>
    inline void load(Archive &a, std::unordered_set<hval> &x, const boost::serialization::version_type ver)
    {
      x.clear();
      size_t s = 0;
      a >> s;
      for(size_t i = 0; i != s; i++)
      {
        hval v;
        a >> v;
        x.insert(v);
      }
    }


    template <class Archive, class h_key, class hval>
    inline void serialize(Archive &a, std::unordered_map<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      split_free(a, x, ver);
    }

    template <class Archive, class h_key, class hval>
    inline void serialize(Archive &a, std::unordered_multimap<h_key, hval> &x, const boost::serialization::version_type ver)
    {
      split_free(a, x, ver);
    }

    template <class Archive, class hval>
    inline void serialize(Archive &a, std::unordered_set<hval> &x, const boost::serialization::version_type ver)
    {
      split_free(a, x, ver);
    }
  }
}
