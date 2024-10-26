// Copyright (c) 2022-2024, The Monero Project
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

// Variant wrapper class.

#pragma once

//local headers

//third party headers
#include <boost/blank.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/distance.hpp>
#include <boost/mpl/find.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/none_t.hpp>
#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

//standard headers
#include <stdexcept>
#include <type_traits>
#include <utility>

//forward declarations


namespace tools
{

[[noreturn]] inline void variant_static_visitor_blank_err()
{ throw std::runtime_error("variant: tried to visit an empty variant."); }
[[noreturn]] inline void variant_unwrap_err()
{ throw std::runtime_error("variant: tried to access value of incorrect type."); }

////
// variant: convenience wrapper around boost::variant with a cleaner interface
// - the variant is 'optional' - an empty variant will evaluate to 'false' and an initialized variant will be 'true'
///
template <typename ResultT>
struct variant_static_visitor : public boost::static_visitor<ResultT>
{
    /// provide visitation for empty variants
    /// - add this to your visitor with: using variant_static_visitor::operator();
    [[noreturn]] ResultT operator()(const boost::blank)       { variant_static_visitor_blank_err(); }
    [[noreturn]] ResultT operator()(const boost::blank) const { variant_static_visitor_blank_err(); }
};

template <typename... Types>
class variant final
{
    using VType = boost::variant<boost::blank, Types...>;

public:
//constructors
    /// default constructor
    variant() = default;
    variant(boost::none_t) : variant{} {}  //act like boost::optional

    /// construct from variant type (use enable_if to avoid issues with copy/move constructor)
    template <typename T,
        typename std::enable_if<
                !std::is_same<
                        std::remove_cv_t<std::remove_reference_t<T>>,
                        variant<Types...>
                    >::value,
                bool
            >::type = true>
    variant(T &&value) : m_value{std::forward<T>(value)} {}

//overloaded operators
    /// boolean operator: true if the variant isn't empty/uninitialized
    explicit operator bool() const noexcept { return !this->is_empty(); }

//member functions
    /// check if empty/uninitialized
    bool is_empty() const noexcept { return m_value.which() == 0; }

    /// check the variant type
    template <typename T>
    bool is_type() const noexcept { return this->index() == this->type_index_of<T>(); }

    /// try to get a handle to the embedded value (return nullptr on failure)
    template <typename T>
          T* try_unwrap()       noexcept { return boost::strict_get<      T>(&m_value); }
    template <typename T>
    const T* try_unwrap() const noexcept { return boost::strict_get<const T>(&m_value); }

    /// get a handle to the embedded value
    template <typename T>
    T& unwrap()
    {
        T *value_ptr{this->try_unwrap<T>()};
        if (!value_ptr) variant_unwrap_err();
        return *value_ptr;
    }
    template <typename T>
    const T& unwrap() const
    {
        const T *value_ptr{this->try_unwrap<T>()};
        if (!value_ptr) variant_unwrap_err();
        return *value_ptr;
    }

    /// get the type index of the currently stored type
    int index() const noexcept { return m_value.which(); }

    /// get the type index of a requested type (compile error for invalid types) (boost::mp11 is boost 1.66.0)
    template <typename T>
    static constexpr int type_index_of() noexcept
    {
        using types = boost::mpl::vector<boost::blank, Types...>;
        using elem  = typename boost::mpl::find<types, T>::type;
        using begin = typename boost::mpl::begin<types>::type;
        return boost::mpl::distance<begin, elem>::value;
    }

    /// check if two variants have the same type
    static bool same_type(const variant<Types...> &v1, const variant<Types...> &v2) noexcept
    { return v1.index() == v2.index(); }

    /// apply a visitor to the variant
    template <typename VisitorT>
    typename VisitorT::result_type visit(VisitorT &&visitor)
    {
        return boost::apply_visitor(std::forward<VisitorT>(visitor), m_value);
    }
    template <typename VisitorT>
    typename VisitorT::result_type visit(VisitorT &&visitor) const
    {
        return boost::apply_visitor(std::forward<VisitorT>(visitor), m_value);
    }

private:
//member variables
    /// variant of all value types
    VType m_value;
};

} //namespace tools
