// Copyright (c) 2023-2024, The Monero Project
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

#include "common/variant.h"

#include <boost/mpl/deref.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/variant/recursive_variant.hpp>

#include "gtest/gtest.h"

#include <sstream>
#include <type_traits>
#include <vector>

using tools::variant;
using tools::variant_static_visitor;

namespace
{
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T>
using strip_all_t = std::remove_reference_t<std::remove_cv_t<T>>;
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename T, typename U>
using strip_same = std::is_same<strip_all_t<T>, strip_all_t<U>>;
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    typename PositiveType,
    typename TestType,
    typename... VariantTypes,
    class VecTypes         = boost::mpl::vector<VariantTypes...>,
    class VecBegin         = typename boost::mpl::begin<VecTypes>::type,
    class VecIndexT        = typename boost::mpl::find<VecTypes, TestType>::type,
    size_t TYPE_INDEX      = boost::mpl::distance<VecBegin, VecIndexT>::value,
    bool LAST_VARIANT_TYPE = TYPE_INDEX == sizeof...(VariantTypes) - 1
>
static std::enable_if_t<LAST_VARIANT_TYPE>
test_is_type_match(const variant<VariantTypes...>& v)
{
    constexpr bool expected = strip_same<PositiveType, TestType>();
    const bool actual = v.template is_type<TestType>();
    EXPECT_EQ(expected, actual);

    EXPECT_FALSE(v.template is_type<boost::blank>());
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    typename PositiveType,
    typename TestType,
    typename... VariantTypes,
    class VecTypes         = boost::mpl::vector<VariantTypes...>,
    class VecBegin         = typename boost::mpl::begin<VecTypes>::type,
    class VecIndexT        = typename boost::mpl::find<VecTypes, TestType>::type,
    size_t TYPE_INDEX      = boost::mpl::distance<VecBegin, VecIndexT>::value,
    bool LAST_VARIANT_TYPE = TYPE_INDEX == sizeof...(VariantTypes) - 1
>
static std::enable_if_t<!LAST_VARIANT_TYPE>
test_is_type_match(const variant<VariantTypes...>& v)
{
    constexpr bool expected = strip_same<PositiveType, TestType>();
    const bool actual = v.template is_type<TestType>();
    EXPECT_EQ(expected, actual);

    using NextTypeIt   = typename boost::mpl::advance<VecIndexT, boost::mpl::int_<1>>::type;
    using NextTestType = typename boost::mpl::deref<NextTypeIt>::type;
    test_is_type_match<PositiveType, NextTestType>(v);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    typename VariantType0,
    typename... VariantTypesRest,
    typename AssignType
>
static void test_is_type_ref
(
    variant<VariantType0, VariantTypesRest...>& v,
    AssignType&& val
)
{
    v = val;
    test_is_type_match<AssignType, VariantType0>(v);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    typename VariantType0,
    typename... VariantTypesRest,
    typename AssignType0,
    typename... AssignTypesRest
>
static void test_is_type_ref
(
    variant<VariantType0, VariantTypesRest...>& v,
    AssignType0&& val_0,
    AssignTypesRest&&... val_rest
)
{
    v = val_0;
    test_is_type_match<AssignType0, VariantType0>(v);
    test_is_type_ref(v, val_rest...);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename... VariantTypes>
static void test_is_type_full(VariantTypes&&... test_vals)
{
    variant<VariantTypes...> v;
    test_is_type_ref(v, test_vals...);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    size_t IJ = 0,
    typename... VariantTypes,
    bool END = IJ == sizeof...(VariantTypes) * sizeof...(VariantTypes)
>
static std::enable_if_t<END>
test_same_type_ref
(
    variant<VariantTypes...>& v1,
    variant<VariantTypes...>& v2,
    const std::tuple<VariantTypes...>& tup_i,
    const std::tuple<VariantTypes...>& tup_j
)
{ /* trivial end case */ }
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template
<
    size_t IJ = 0,
    typename... VariantTypes,
    bool END = IJ == sizeof...(VariantTypes) * sizeof...(VariantTypes)
>
static std::enable_if_t<!END>
test_same_type_ref
(
    variant<VariantTypes...>& v1,
    variant<VariantTypes...>& v2,
    const std::tuple<VariantTypes...>& tup_i,
    const std::tuple<VariantTypes...>& tup_j
)
{
    constexpr size_t I = IJ / sizeof...(VariantTypes);
    constexpr size_t J = IJ % sizeof...(VariantTypes);
    constexpr bool expected = I == J;

    v1 = std::get<I>(tup_i);
    v2 = std::get<J>(tup_j);
    const bool actual = variant<VariantTypes...>::same_type(v1, v2);

    EXPECT_EQ(expected, actual);

    test_same_type_ref<IJ + 1>(v1, v2, tup_i, tup_j);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
template <typename... VariantTypes>
static void test_same_type_full
(
    const std::tuple<VariantTypes...>& vals_i,
    const std::tuple<VariantTypes...>& vals_j
)
{
    using Variant = variant<VariantTypes...>;
    Variant v_i;
    Variant v_j;
    test_same_type_ref(v_i, v_j, vals_i, vals_j);
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
struct test_stringify_visitor: public variant_static_visitor<std::string>
{
    template <typename T>
    static std::string stringify(const T& t)
    {
        std::stringstream ss;
        ss << typeid(T).name();
        ss << "::";
        ss << t;
        return ss.str();
    }

    template <class Variant, typename T>
    static void test_visitation(const Variant& v, const T& t)
    {
        EXPECT_EQ(test_stringify_visitor::stringify(t), v.visit(test_stringify_visitor()));
    }

    // Make sure boost::blank errors
    using variant_static_visitor::operator();

    // Visitation implementation
    template <typename T>
    std::string operator()(const T& t) const
    {
        return test_stringify_visitor::stringify(t);
    }
};
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
} // anonymous namespace

//-------------------------------------------------------------------------------------------------------------------
TEST(variant, operatorbool)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_FALSE(v);
    v = (int16_t) 2023;
    EXPECT_TRUE(v);
    v = (int16_t) 0;
    EXPECT_TRUE(v);
    v = boost::blank{};
    EXPECT_FALSE(v);
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, is_empty)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_TRUE(v.is_empty());
    v = (int16_t) 2023;
    EXPECT_FALSE(v.is_empty());
    v = (int16_t) 0;
    EXPECT_FALSE(v.is_empty());
    v = boost::blank{};
    EXPECT_TRUE(v.is_empty());

    variant<> v2;
    EXPECT_TRUE(v2.is_empty());
    v2 = boost::blank{};
    EXPECT_TRUE(v2.is_empty());
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, is_type)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_TRUE(v.is_type<boost::blank>());
    v = (int16_t) 2023;
    EXPECT_TRUE(v.is_type<int16_t>());

    test_is_type_full((uint32_t) 2023, (char) '\n', std::string("HOWDY"));
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, try_unwrap)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_FALSE(v.try_unwrap<int8_t>());
    v = (int16_t) 5252;
    ASSERT_TRUE(v.try_unwrap<int16_t>());
    EXPECT_EQ(5252, *v.try_unwrap<int16_t>());
    EXPECT_FALSE(v.try_unwrap<uint16_t>());
    EXPECT_FALSE(v.try_unwrap<std::string>());
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, unwrap)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_THROW(v.unwrap<int8_t>(), std::runtime_error);
    v = (int16_t) 5252;
    EXPECT_EQ(5252, v.unwrap<int16_t>());
    EXPECT_THROW(v.unwrap<uint16_t>(), std::runtime_error);
    EXPECT_THROW(v.unwrap<std::string>(), std::runtime_error);
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, mutation)
{
    variant<uint8_t> v;
    v = (uint8_t) 5;
    EXPECT_EQ(5, v.unwrap<uint8_t>());
    uint8_t &intref{v.unwrap<uint8_t>()};
    intref = 10;
    EXPECT_EQ(10, v.unwrap<uint8_t>());
    EXPECT_TRUE(v.try_unwrap<uint8_t>());
    uint8_t *intptr{v.try_unwrap<uint8_t>()};
    *intptr = 15;
    EXPECT_EQ(15, v.unwrap<uint8_t>());

    const variant<uint8_t> &v_ref{v};
    EXPECT_EQ(15, v_ref.unwrap<uint8_t>());
    EXPECT_TRUE(v_ref.try_unwrap<uint8_t>());
    EXPECT_EQ(15, *(v_ref.try_unwrap<uint8_t>()));
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, index)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_EQ(0, v.index());
    v = (int8_t) 7;
    EXPECT_EQ(1, v.index());
    v = (uint8_t) 7;
    EXPECT_EQ(2, v.index());
    v = (int16_t) 7;
    EXPECT_EQ(3, v.index());
    v = (uint16_t) 7;
    EXPECT_EQ(4, v.index());
    v = "verifiable variant vying for vengence versus visa";
    EXPECT_EQ(5, v.index());
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, type_index_of)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_EQ(0, decltype(v)::type_index_of<boost::blank>());
    EXPECT_EQ(1, decltype(v)::type_index_of<int8_t>());
    EXPECT_EQ(2, decltype(v)::type_index_of<uint8_t>());
    EXPECT_EQ(3, decltype(v)::type_index_of<int16_t>());
    EXPECT_EQ(4, decltype(v)::type_index_of<uint16_t>());
    EXPECT_EQ(5, decltype(v)::type_index_of<std::string>());
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, constexpr_type_index_of)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    constexpr int TINDEX0 = decltype(v)::type_index_of<boost::blank>();
    EXPECT_EQ(0, TINDEX0);
    constexpr int TINDEX5 = decltype(v)::type_index_of<std::string>();
    EXPECT_EQ(5, TINDEX5);
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, same_type)
{
    const std::tuple<int, std::string, char> vals_i(77840, "Hullubaloo", '\0');
    const std::tuple<int, std::string, char> vals_j(1876, "Canneck", '\t');
    test_same_type_full(vals_i, vals_j);
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, visit)
{
    variant<int8_t, uint8_t, int16_t, uint16_t, std::string> v;
    EXPECT_THROW(v.visit(test_stringify_visitor()), std::runtime_error);

    v = "Rev";
    test_stringify_visitor::test_visitation(v, std::string("Rev"));

    v = (int16_t) 2001;
    test_stringify_visitor::test_visitation(v, (int16_t) 2001);
    EXPECT_NE(test_stringify_visitor::stringify((uint16_t) 2001), v.visit(test_stringify_visitor()));
}
//-------------------------------------------------------------------------------------------------------------------
TEST(variant, ad_hoc_recursion)
{
    struct left_t;
    struct right_t;

    using twisty = variant<boost::recursive_wrapper<left_t>, boost::recursive_wrapper<right_t>>;

    struct left_t
    {
        twisty l;
    };

    struct right_t
    {
        twisty r;
    };

    auto right = [](twisty&& t = {}) -> twisty
    {
        right_t r;
        r.r = t;
        return r;
    };

    auto left = [](twisty&& t = {}) -> twisty
    {
        left_t l;
        l.l = t;
        return l;
    };

    struct twisty_counter: variant_static_visitor<std::pair<int, int>>
    {
        std::pair<int, int> operator()(boost::blank) const
        {
            return {0, 0};
        }

        std::pair<int, int> operator()(const left_t& l) const
        {
            auto count = l.l.visit(twisty_counter());
            count.first += 1;
            return count;
        }

        std::pair<int, int> operator()(const right_t& r) const
        {
            auto count = r.r.visit(twisty_counter());
            count.second += 1;
            return count;
        }
    };

    const twisty tw = left(left(right(right(left(right(left(right(left()))))))));

    int left_count, right_count;
    std::tie(left_count, right_count) = tw.visit(twisty_counter());

    EXPECT_EQ(5, left_count);
    EXPECT_EQ(4, right_count);
}
//-------------------------------------------------------------------------------------------------------------------
