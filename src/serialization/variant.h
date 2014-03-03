// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <boost/variant/variant.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/pop_front.hpp>
#include "serialization.h"

template <class Archive, class T>
struct variant_serialization_traits
{
};

template <class Archive, class Variant, class TBegin, class TEnd>
struct variant_reader
{
  typedef typename Archive::variant_tag_type variant_tag_type;
  typedef typename boost::mpl::next<TBegin>::type TNext;
  typedef typename boost::mpl::deref<TBegin>::type current_type;

  static inline bool read(Archive &ar, Variant &v, variant_tag_type t)
  {
    if (variant_serialization_traits<Archive, current_type>::get_tag() == t) {
      current_type x;
      if(!::do_serialize(ar, x))
      {
        ar.stream().setstate(std::ios::failbit);
        return false;
      }
      v = x;
    } else {
      return variant_reader<Archive, Variant, TNext, TEnd>::read(ar, v, t);
    }
    return true;
  }
};

template <class Archive, class Variant, class TBegin>
struct variant_reader<Archive, Variant, TBegin, TBegin>
{
  typedef typename Archive::variant_tag_type variant_tag_type;

  static inline bool read(Archive &ar, Variant &v, variant_tag_type t)
  {
    ar.stream().setstate(std::ios::failbit);
    return false;
  }

};


template <template <bool> class Archive, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct serializer<Archive<false>, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
  typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> variant_type;
  typedef typename Archive<false>::variant_tag_type variant_tag_type;
  typedef typename variant_type::types types;

  static bool serialize(Archive<false> &ar, variant_type &v) {
    variant_tag_type t;
    ar.begin_variant();
    ar.read_variant_tag(t);
    if(!variant_reader<Archive<false>, variant_type, typename boost::mpl::begin<types>::type, typename boost::mpl::end<types>::type>::read(ar, v, t))
    {
      ar.stream().setstate(std::ios::failbit);
      return false;
    }
    ar.end_variant();
    return true;
  }
};

template <template <bool> class Archive, BOOST_VARIANT_ENUM_PARAMS(typename T)>
struct serializer<Archive<true>, boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)>>
{
  typedef boost::variant<BOOST_VARIANT_ENUM_PARAMS(T)> variant_type;
  //typedef typename Archive<true>::variant_tag_type variant_tag_type;

  struct visitor : public boost::static_visitor<bool>
  {
    Archive<true> &ar;

    visitor(Archive<true> &a) : ar(a) { }

    template <class T>
    bool operator ()(T &rv) const
    {
      ar.begin_variant();
      ar.write_variant_tag(variant_serialization_traits<Archive<true>, T>::get_tag());
      if(!::do_serialize(ar, rv))
      {
        ar.stream().setstate(std::ios::failbit);
        return false;
      }
      ar.end_variant();
      return true;
    }
  };

  static bool serialize(Archive<true> &ar, variant_type &v) {
    return boost::apply_visitor(visitor(ar), v);
  }
};
