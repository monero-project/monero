// Copyright (c) 2018-2022, The Monero Project

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

#include <gtest/gtest.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/utility/string_ref.hpp>
#include <string>
#include <system_error>
#include <type_traits>

#include "common/expect.h"

namespace
{
    struct move_only;
    struct throw_construct;
    struct throw_copies;
    struct throw_moves;

    struct move_only
    {
        move_only() = default;
        move_only(move_only const&) = delete;
        move_only(move_only&&) = default;
        ~move_only() = default;
        move_only& operator=(move_only const&) = delete;
        move_only& operator=(move_only&&) = default;
    };

    struct throw_construct
    {
        throw_construct() {}
        throw_construct(int) {}
        throw_construct(throw_construct const&) = default;
        throw_construct(throw_construct&&) = default;
        ~throw_construct() = default;
        throw_construct& operator=(throw_construct const&) = default;
        throw_construct& operator=(throw_construct&&) = default;
    };

    struct throw_copies
    {
        throw_copies() noexcept {}
        throw_copies(throw_copies const&) {}
        throw_copies(throw_copies&&) = default;
        ~throw_copies() = default;
        throw_copies& operator=(throw_copies const&) { return *this; }
        throw_copies& operator=(throw_copies&&) = default;
        bool operator==(throw_copies const&) noexcept { return true; }
        bool operator==(throw_moves const&) noexcept { return true; }
    };

    struct throw_moves
    {
        throw_moves() noexcept {}
        throw_moves(throw_moves const&) = default;
        throw_moves(throw_moves&&) {}
        ~throw_moves() = default;
        throw_moves& operator=(throw_moves const&) = default;
        throw_moves& operator=(throw_moves&&) { return *this; }
        bool operator==(throw_moves const&) { return true; }
        bool operator==(throw_copies const&) { return true; }
    };

    template<typename T>
    void construction_bench()
    {
        EXPECT_TRUE(std::is_copy_constructible<expect<T>>());
        EXPECT_TRUE(std::is_move_constructible<expect<T>>());
        EXPECT_TRUE(std::is_copy_assignable<expect<T>>());
        EXPECT_TRUE(std::is_move_assignable<expect<T>>());
        EXPECT_TRUE(std::is_destructible<expect<T>>());
    }

    template<typename T>
    void noexcept_bench()
    {
        EXPECT_TRUE(std::is_nothrow_copy_constructible<expect<T>>());
        EXPECT_TRUE(std::is_nothrow_move_constructible<expect<T>>());
        EXPECT_TRUE(std::is_nothrow_copy_assignable<expect<T>>());
        EXPECT_TRUE(std::is_nothrow_move_assignable<expect<T>>());
        EXPECT_TRUE(std::is_nothrow_destructible<expect<T>>());

        EXPECT_TRUE(noexcept(bool(std::declval<expect<T>>())));
        EXPECT_TRUE(noexcept(std::declval<expect<T>>().has_error()));
        EXPECT_TRUE(noexcept(std::declval<expect<T>>().error()));
        EXPECT_TRUE(noexcept(std::declval<expect<T>>().equal(std::declval<expect<T>>())));
        EXPECT_TRUE(noexcept(std::declval<expect<T>>() == std::declval<expect<T>>()));
        EXPECT_TRUE(noexcept(std::declval<expect<T>>() != std::declval<expect<T>>()));
    }

    template<typename T>
    void conversion_bench()
    {
        EXPECT_TRUE((std::is_convertible<std::error_code, expect<T>>()));
        EXPECT_TRUE((std::is_convertible<std::error_code&&, expect<T>>()));
        EXPECT_TRUE((std::is_convertible<std::error_code&, expect<T>>()));
        EXPECT_TRUE((std::is_convertible<std::error_code const&, expect<T>>()));

        EXPECT_TRUE((std::is_constructible<expect<T>, std::error_code>()));
        EXPECT_TRUE((std::is_constructible<expect<T>, std::error_code&&>()));
        EXPECT_TRUE((std::is_constructible<expect<T>, std::error_code&>()));
        EXPECT_TRUE((std::is_constructible<expect<T>, std::error_code const&>()));
    }
}


TEST(Expect, Constructions)
{
    construction_bench<void>();
    construction_bench<int>();

    EXPECT_TRUE(std::is_constructible<expect<void>>());

    EXPECT_TRUE((std::is_constructible<expect<throw_construct>, expect<int>>()));

    EXPECT_TRUE(std::is_move_constructible<expect<move_only>>());
    EXPECT_TRUE(std::is_move_assignable<expect<move_only>>());
}

TEST(Expect, Conversions)
{
    struct implicit { implicit(int) {} };
    struct explicit_only { explicit explicit_only(int) {} };

    conversion_bench<void>();
    conversion_bench<int>();

    EXPECT_TRUE((std::is_convertible<int, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<int&&, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<int&, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<int const, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<expect<unsigned>, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<expect<unsigned>&&, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<expect<unsigned>&, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<expect<unsigned> const&, expect<int>>()));
    EXPECT_TRUE((std::is_convertible<expect<int>, expect<implicit>>()));
    EXPECT_TRUE((std::is_convertible<expect<int>&&, expect<implicit>>()));
    EXPECT_TRUE((std::is_convertible<expect<int>&, expect<implicit>>()));
    EXPECT_TRUE((std::is_convertible<expect<int> const&, expect<implicit>>()));
    EXPECT_TRUE(!(std::is_convertible<expect<int>, expect<explicit_only>>()));
    EXPECT_TRUE(!(std::is_convertible<expect<int>&&, expect<explicit_only>>()));
    EXPECT_TRUE(!(std::is_convertible<expect<int>&, expect<explicit_only>>()));
    EXPECT_TRUE(!(std::is_convertible<expect<int> const&, expect<explicit_only>>()));

    EXPECT_TRUE((std::is_constructible<expect<int>, int>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, int&&>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, int&>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, int const&>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, expect<unsigned>>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, expect<unsigned>&&>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, expect<unsigned>&>()));
    EXPECT_TRUE((std::is_constructible<expect<int>, expect<unsigned> const&>()));
    EXPECT_TRUE((std::is_constructible<expect<implicit>, expect<int>>()));
    EXPECT_TRUE((std::is_constructible<expect<implicit>, expect<int>&&>()));
    EXPECT_TRUE((std::is_constructible<expect<implicit>, expect<int>&>()));
    EXPECT_TRUE((std::is_constructible<expect<implicit>, expect<int> const&>()));
    EXPECT_TRUE(!(std::is_constructible<expect<explicit_only>, expect<int>>()));
    EXPECT_TRUE(!(std::is_constructible<expect<explicit_only>, expect<int>&&>()));
    EXPECT_TRUE(!(std::is_constructible<expect<explicit_only>, expect<int>&>()));
    EXPECT_TRUE(!(std::is_constructible<expect<explicit_only>, expect<int> const&>()));

    EXPECT_EQ(expect<int>{expect<short>{100}}.value(), 100);

    expect<std::string> val1{std::string{}};
    expect<const char*> val2{"foo"};

    EXPECT_EQ(val1.value(), std::string{});
    EXPECT_EQ(val2.value(), std::string{"foo"});

    const expect<std::string> val3{val2};

    EXPECT_EQ(val1.value(), std::string{});
    EXPECT_EQ(val2.value(), std::string{"foo"});
    EXPECT_EQ(val3.value(), std::string{"foo"});

    val1 = val2;

    EXPECT_EQ(val1.value(), "foo");
    EXPECT_EQ(val2.value(), std::string{"foo"});
    EXPECT_EQ(val3.value(), "foo");
}

TEST(Expect, NoExcept)
{
    noexcept_bench<void>();
    noexcept_bench<int>();

    EXPECT_TRUE(std::is_nothrow_constructible<expect<void>>());

    EXPECT_TRUE((std::is_nothrow_constructible<expect<int>, int>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<int>, expect<unsigned>>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<int>, expect<unsigned>&&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<int>, expect<unsigned>&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<int>, expect<unsigned> const&>()));

    EXPECT_TRUE(noexcept(expect<int>{std::declval<expect<unsigned>&&>()}));
    EXPECT_TRUE(noexcept(expect<int>{std::declval<expect<unsigned> const&>()}));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>().has_value()));
    EXPECT_TRUE(noexcept(*std::declval<expect<int>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>().equal(std::declval<expect<unsigned>>())));
    EXPECT_TRUE(noexcept(std::declval<expect<unsigned>>().equal(std::declval<expect<int>>())));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>().equal(0)));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>() == std::declval<expect<unsigned>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<unsigned>>() == std::declval<expect<int>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>() == 0));
    EXPECT_TRUE(noexcept(0 == std::declval<expect<int>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>() != std::declval<expect<unsigned>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<unsigned>>() != std::declval<expect<int>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<int>>() != 0));
    EXPECT_TRUE(noexcept(0 != std::declval<expect<int>>()));

    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, std::error_code>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, std::error_code&&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, std::error_code&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, std::error_code const&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, throw_construct>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, throw_construct&&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, throw_construct&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_construct>, throw_construct const&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_construct>, expect<int>>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_construct>, expect<int>&&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_construct>, expect<int>&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_construct>, expect<int> const&>()));
    EXPECT_TRUE(std::is_nothrow_copy_constructible<expect<throw_construct>>());
    EXPECT_TRUE(std::is_nothrow_move_constructible<expect<throw_construct>>());
    EXPECT_TRUE(std::is_nothrow_copy_assignable<expect<throw_construct>>());
    EXPECT_TRUE(std::is_nothrow_move_assignable<expect<throw_construct>>());
    EXPECT_TRUE(std::is_nothrow_destructible<expect<throw_construct>>());

    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, std::error_code>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, std::error_code&&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, std::error_code&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, std::error_code const&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, throw_copies>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_copies>, throw_copies&&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_copies>, throw_copies&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_copies>, throw_copies const&>()));
    EXPECT_TRUE(!std::is_nothrow_copy_constructible<expect<throw_copies>>());
    EXPECT_TRUE(std::is_nothrow_move_constructible<expect<throw_copies>>());
    EXPECT_TRUE(!std::is_nothrow_copy_assignable<expect<throw_copies>>());
    EXPECT_TRUE(std::is_nothrow_move_assignable<expect<throw_copies>>());
    EXPECT_TRUE(std::is_nothrow_destructible<expect<throw_copies>>());
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>().equal(std::declval<expect<throw_copies>>())));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>().equal(std::declval<throw_copies>())));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() == std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() == std::declval<throw_copies>()));
    EXPECT_TRUE(noexcept(std::declval<throw_copies>() == std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() != std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() != std::declval<throw_copies>()));
    EXPECT_TRUE(noexcept(std::declval<throw_copies>() != std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>().equal(std::declval<expect<throw_moves>>())));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>().equal(std::declval<throw_moves>())));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() == std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() == std::declval<throw_moves>()));
    EXPECT_TRUE(noexcept(std::declval<throw_moves>() == std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() != std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(noexcept(std::declval<expect<throw_copies>>() != std::declval<throw_moves>()));
    EXPECT_TRUE(noexcept(std::declval<throw_moves>() != std::declval<expect<throw_copies>>()));

    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_moves>, std::error_code>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_moves>, std::error_code&&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_moves>, std::error_code&>()));
    EXPECT_TRUE((std::is_nothrow_constructible<expect<throw_moves>, std::error_code const&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_moves>, throw_moves>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_moves>, throw_moves&&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_moves>, throw_moves&>()));
    EXPECT_TRUE(!(std::is_nothrow_constructible<expect<throw_moves>, throw_moves const&>()));
    EXPECT_TRUE(std::is_nothrow_copy_constructible<expect<throw_moves>>());
    EXPECT_TRUE(!std::is_nothrow_move_constructible<expect<throw_moves>>());
    EXPECT_TRUE(std::is_nothrow_copy_assignable<expect<throw_moves>>());
    EXPECT_TRUE(!std::is_nothrow_move_assignable<expect<throw_moves>>());
    EXPECT_TRUE(std::is_nothrow_destructible<expect<throw_copies>>());
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>().equal(std::declval<expect<throw_moves>>())));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>().equal(std::declval<throw_moves>())));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() == std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() == std::declval<throw_moves>()));
    EXPECT_TRUE(!noexcept(std::declval<throw_moves>() == std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() != std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() != std::declval<throw_moves>()));
    EXPECT_TRUE(!noexcept(std::declval<throw_moves>() != std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>().equal(std::declval<expect<throw_copies>>())));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>().equal(std::declval<throw_copies>())));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() == std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() == std::declval<throw_copies>()));
    EXPECT_TRUE(!noexcept(std::declval<throw_copies>() == std::declval<expect<throw_moves>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() != std::declval<expect<throw_copies>>()));
    EXPECT_TRUE(!noexcept(std::declval<expect<throw_moves>>() != std::declval<throw_copies>()));
    EXPECT_TRUE(!noexcept(std::declval<throw_copies>() != std::declval<expect<throw_moves>>()));
}

TEST(Expect, Trivial)
{
    EXPECT_TRUE(std::is_trivially_copy_constructible<expect<void>>());
    EXPECT_TRUE(std::is_trivially_move_constructible<expect<void>>());
    EXPECT_TRUE(std::is_trivially_destructible<expect<void>>());
}

TEST(Expect, Assignment)
{ 
    expect<std::string> val1{std::string{}};
    expect<std::string> val2{"foobar"};

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(bool(val1));
    EXPECT_TRUE(bool(val2));
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_EQ(val1.value(), std::string{});
    EXPECT_TRUE(*val1 == std::string{});
    EXPECT_TRUE(boost::equals(val1->c_str(), ""));
    EXPECT_TRUE(val2.value() == "foobar");
    EXPECT_TRUE(*val2 == "foobar");
    EXPECT_TRUE(boost::equals(val2->c_str(), "foobar"));
    EXPECT_EQ(val1.error(), std::error_code{});
    EXPECT_EQ(val2.error(), std::error_code{});
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val1 = std::move(val2);

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(bool(val1));
    EXPECT_TRUE(bool(val2));
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_EQ(val1.value(), "foobar");
    EXPECT_TRUE(*val1 == "foobar");
    EXPECT_TRUE(boost::equals(val1->c_str(), "foobar"));
    EXPECT_EQ(val2.value(), std::string{});
    EXPECT_TRUE(*val2 == std::string{});
    EXPECT_TRUE(boost::equals(val2->c_str(), ""));
    EXPECT_EQ(val1.error(), std::error_code{});
    EXPECT_EQ(val2.error(), std::error_code{});
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val2 = val1;

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(bool(val1));
    EXPECT_TRUE(bool(val2));
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_EQ(val1.value(), "foobar");
    EXPECT_TRUE(*val1 == "foobar");
    EXPECT_TRUE(boost::equals(val1->c_str(), "foobar"));
    EXPECT_EQ(val2.value(), "foobar");
    EXPECT_TRUE(*val2 == "foobar");
    EXPECT_TRUE(boost::equals(val2->c_str(), "foobar"));
    EXPECT_EQ(val1.error(), std::error_code{});
    EXPECT_EQ(val2.error(), std::error_code{});
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val1 = make_error_code(common_error::kInvalidArgument);

    ASSERT_TRUE(val1.has_error());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(!val1);
    EXPECT_TRUE(bool(val2));
    EXPECT_TRUE(!val1.has_value());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_EQ(val1.error(), common_error::kInvalidArgument);
    EXPECT_TRUE(val1 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val1);
    EXPECT_STREQ(val2.value().c_str(), "foobar");
    EXPECT_TRUE(*val2 == "foobar");
    EXPECT_TRUE(boost::equals(val2->c_str(), "foobar"));
    EXPECT_NE(val1.error(), std::error_code{});
    EXPECT_EQ(val2.error(), std::error_code{});
    EXPECT_TRUE(val1.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(val1.matches(std::errc::invalid_argument));
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val2 = val1;

    ASSERT_TRUE(val1.has_error());
    ASSERT_TRUE(val2.has_error());
    EXPECT_TRUE(!val1);
    EXPECT_TRUE(!val2);
    EXPECT_TRUE(!val1.has_value());
    EXPECT_TRUE(!val2.has_value());
    EXPECT_EQ(val1.error(), common_error::kInvalidArgument);
    EXPECT_TRUE(val1 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val1);
    EXPECT_EQ(val2.error(), common_error::kInvalidArgument);
    EXPECT_TRUE(val2 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val2);
    EXPECT_NE(val1.error(), std::error_code{});
    EXPECT_NE(val2.error(), std::error_code{});
    EXPECT_TRUE(val1.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(val2.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(val1.matches(std::errc::invalid_argument));
    EXPECT_TRUE(val2.matches(std::errc::invalid_argument));
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val1 = std::string{"barfoo"};

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_error());
    EXPECT_TRUE(bool(val1));
    EXPECT_TRUE(!val2);
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_value());
    EXPECT_STREQ(val1.value().c_str(), "barfoo");
    EXPECT_TRUE(*val1 == "barfoo");
    EXPECT_TRUE(boost::equals(val1->c_str(), "barfoo"));
    EXPECT_EQ(val2.error(), common_error::kInvalidArgument);
    EXPECT_TRUE(val2 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val2);
    EXPECT_EQ(val1.error(), std::error_code{});
    EXPECT_NE(val2.error(), std::error_code{});
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(val2.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(val2.matches(std::errc::invalid_argument));
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val2 = val1;

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(bool(val1));
    EXPECT_TRUE(bool(val2));
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_EQ(val1.value(), "barfoo");
    EXPECT_TRUE(*val1 == "barfoo");
    EXPECT_TRUE(boost::equals(val1->c_str(), "barfoo"));
    EXPECT_EQ(val2.value(), "barfoo");
    EXPECT_TRUE(*val2 == "barfoo");
    EXPECT_TRUE(boost::equals(val2->c_str(), "barfoo"));
    EXPECT_EQ(val1.error(), std::error_code{});
    EXPECT_EQ(val2.error(), std::error_code{});
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));
}

TEST(Expect, AssignmentThrowsOnMove)
{
    struct construct_error {};
    struct assignment_error {};

    struct throw_on_move {
        std::string msg;

        throw_on_move(const char* msg) : msg(msg) {}
        throw_on_move(throw_on_move&&) {
            throw construct_error{};
        }
        throw_on_move(throw_on_move const&) = default;
        ~throw_on_move() = default;
        throw_on_move& operator=(throw_on_move&&) {
            throw assignment_error{};
        }
        throw_on_move& operator=(throw_on_move const&) = default;
    };

    expect<throw_on_move> val1{expect<const char*>{"foobar"}};
    expect<throw_on_move> val2{common_error::kInvalidArgument};

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_error());
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_value());
    EXPECT_STREQ(val1->msg.c_str(), "foobar");
    EXPECT_EQ(val2.error(), common_error::kInvalidArgument);

    EXPECT_THROW(val2 = std::move(val1), construct_error);

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_error());
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_value());
    EXPECT_STREQ(val1->msg.c_str(), "foobar");
    EXPECT_EQ(val2.error(), common_error::kInvalidArgument);

    EXPECT_THROW(val1 = expect<const char*>{"barfoo"}, assignment_error);

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_error());
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_value());
    EXPECT_STREQ(val1->msg.c_str(), "foobar");
    EXPECT_EQ(val2.error(), common_error::kInvalidArgument);

    EXPECT_NO_THROW(val2 = val1);

    ASSERT_TRUE(val1.has_value());
    ASSERT_TRUE(val2.has_value());
    EXPECT_TRUE(!val1.has_error());
    EXPECT_TRUE(!val2.has_error());
    EXPECT_STREQ(val1->msg.c_str(), "foobar");
    EXPECT_STREQ(val2->msg.c_str(), "foobar");
}

TEST(Expect, EqualWithStrings)
{
    expect<std::string> val1{std::string{}};
    expect<std::string> val2{"barfoo"};
    expect<boost::string_ref> val3{boost::string_ref{}};

    EXPECT_TRUE(!val1.equal(val2));
    EXPECT_TRUE(val1.equal(val3));
    EXPECT_TRUE(!val2.equal(val1));
    EXPECT_TRUE(!val2.equal(val3));
    EXPECT_TRUE(val3.equal(val1));
    EXPECT_TRUE(!val3.equal(val2));
    EXPECT_TRUE(!(val1 == val2));
    EXPECT_TRUE(!(val2 == val1));
    EXPECT_TRUE(val1 == val3);
    EXPECT_TRUE(val3 == val1);
    EXPECT_TRUE(!(val2 == val3));
    EXPECT_TRUE(!(val3 == val2));
    EXPECT_TRUE(val1 != val2);
    EXPECT_TRUE(val2 != val1);
    EXPECT_TRUE(!(val1 != val3));
    EXPECT_TRUE(!(val3 != val1));
    EXPECT_TRUE(val2 != val3);
    EXPECT_TRUE(val3 != val2);

    EXPECT_TRUE(val1.equal(""));
    EXPECT_TRUE(val2.equal("barfoo"));
    EXPECT_TRUE(val3.equal(""));
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!val3.equal(std::error_code{}));
    EXPECT_TRUE(val1 == "");
    EXPECT_TRUE("" == val1);
    EXPECT_TRUE(val2 == "barfoo");
    EXPECT_TRUE("barfoo" == val2);
    EXPECT_TRUE(val3 == "");
    EXPECT_TRUE("" == val3);
    EXPECT_TRUE(!(val1 != ""));
    EXPECT_TRUE(!("" != val1));
    EXPECT_TRUE(!(val2 != "barfoo"));
    EXPECT_TRUE(!("barfoo" != val2));
    EXPECT_TRUE(!(val3 != ""));
    EXPECT_TRUE(!("" != val3));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(!(val3 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val3));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
    EXPECT_TRUE(val3 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val3);
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));
    EXPECT_TRUE(!val3.matches(std::error_condition{}));

    val2 = make_error_code(common_error::kInvalidArgument);

    EXPECT_TRUE(!val1.equal(val2));
    EXPECT_TRUE(val1.equal(val3));
    EXPECT_TRUE(!val2.equal(val1));
    EXPECT_TRUE(!val2.equal(val3));
    EXPECT_TRUE(val3.equal(val1));
    EXPECT_TRUE(!val3.equal(val2));
    EXPECT_TRUE(!(val1 == val2));
    EXPECT_TRUE(!(val2 == val1));
    EXPECT_TRUE(val1 == val3);
    EXPECT_TRUE(val3 == val1);
    EXPECT_TRUE(!(val2 == val3));
    EXPECT_TRUE(!(val3 == val2));
    EXPECT_TRUE(val1 != val2);
    EXPECT_TRUE(val2 != val1);
    EXPECT_TRUE(!(val1 != val3));
    EXPECT_TRUE(!(val3 != val1));
    EXPECT_TRUE(val2 != val3);
    EXPECT_TRUE(val3 != val2);

    EXPECT_TRUE(!val1.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(val2.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!val3.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(val2 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val2);
    EXPECT_TRUE(!(val2 != common_error::kInvalidArgument));
    EXPECT_TRUE(!(common_error::kInvalidArgument != val2));
    EXPECT_TRUE(val2.matches(std::errc::invalid_argument));
    EXPECT_TRUE(!val2.matches(std::error_condition{}));

    val1 = expect<std::string>{"barfoo"};

    EXPECT_TRUE(!val1.equal(val2));
    EXPECT_TRUE(!val1.equal(val3));
    EXPECT_TRUE(!val2.equal(val1));
    EXPECT_TRUE(!val2.equal(val3));
    EXPECT_TRUE(!val3.equal(val1));
    EXPECT_TRUE(!val3.equal(val2));
    EXPECT_TRUE(!(val1 == val2));
    EXPECT_TRUE(!(val2 == val1));
    EXPECT_TRUE(!(val1 == val3));
    EXPECT_TRUE(!(val3 == val1));
    EXPECT_TRUE(!(val2 == val3));
    EXPECT_TRUE(!(val3 == val2));
    EXPECT_TRUE(val1 != val2);
    EXPECT_TRUE(val2 != val1);
    EXPECT_TRUE(val1 != val3);
    EXPECT_TRUE(val3 != val1);
    EXPECT_TRUE(val2 != val3);
    EXPECT_TRUE(val3 != val2);

    EXPECT_TRUE(val1.equal("barfoo"));
    EXPECT_TRUE(val1 == "barfoo");
    EXPECT_TRUE("barfoo" == val1);
    EXPECT_TRUE(!(val1 != "barfoo"));
    EXPECT_TRUE(!("barfoo" != val1));
    EXPECT_TRUE(!val1.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!(val1 == common_error::kInvalidArgument));
    EXPECT_TRUE(!(common_error::kInvalidArgument == val1));
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!val1.matches(std::error_condition{}));
    EXPECT_TRUE(!val1.matches(std::errc::invalid_argument));
}

TEST(Expect, EqualWithVoid)
{
    const expect<void> val1;
    expect<void> val2;

    EXPECT_TRUE(val1.equal(val2));
    EXPECT_TRUE(val2.equal(val1));
    EXPECT_TRUE(!val1.equal(std::error_code{}));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(val1 == val2);
    EXPECT_TRUE(val2 == val1);
    EXPECT_TRUE(!(val1 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val1));
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(!(val1 != val2));
    EXPECT_TRUE(!(val2 != val1));
    EXPECT_TRUE(val1 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val1);
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));

    val2 = make_error_code(common_error::kInvalidArgument);

    EXPECT_TRUE(!val1.equal(val2));
    EXPECT_TRUE(!val2.equal(val1));
    EXPECT_TRUE(!val1.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(val2.equal(common_error::kInvalidArgument));
    EXPECT_TRUE(!val2.equal(std::error_code{}));
    EXPECT_TRUE(!(val1 == val2));
    EXPECT_TRUE(!(val2 == val1));
    EXPECT_TRUE(val2 == common_error::kInvalidArgument);
    EXPECT_TRUE(common_error::kInvalidArgument == val2);
    EXPECT_TRUE(!(val2 == std::error_code{}));
    EXPECT_TRUE(!(std::error_code{} == val2));
    EXPECT_TRUE(val1 != val2);
    EXPECT_TRUE(val2 != val1);
    EXPECT_TRUE(!(val2 != common_error::kInvalidArgument));
    EXPECT_TRUE(!(common_error::kInvalidArgument != val2));
    EXPECT_TRUE(val2 != std::error_code{});
    EXPECT_TRUE(std::error_code{} != val2);
}

TEST(Expect, EqualNoCopies)
{
    struct copy_error {};

    struct throw_on_copy {
        throw_on_copy() = default;
        throw_on_copy(int) noexcept {}
        throw_on_copy(throw_on_copy const&) {
            throw copy_error{};
        }
        ~throw_on_copy() = default;
        throw_on_copy& operator=(throw_on_copy const&) {
            throw copy_error{};
        }

        bool operator==(throw_on_copy const&) const noexcept { return true; }
    };

    expect<throw_on_copy> val1{expect<int>{0}};
    expect<throw_on_copy> val2{expect<int>{0}};

    EXPECT_TRUE(val1.equal(val2));
    EXPECT_TRUE(val2.equal(val1));
    EXPECT_TRUE(val1 == val2);
    EXPECT_TRUE(val2 == val1);
    EXPECT_TRUE(!(val1 != val2));
    EXPECT_TRUE(!(val2 != val1));

    EXPECT_TRUE(val1.equal(throw_on_copy{}));
    EXPECT_TRUE(val1 == throw_on_copy{});
    EXPECT_TRUE(throw_on_copy{} == val1);
    EXPECT_TRUE(!(val1 != throw_on_copy{}));
    EXPECT_TRUE(!(throw_on_copy{} != val1));

    throw_on_copy val3;

    EXPECT_TRUE(val1.equal(val3));
    EXPECT_TRUE(val1 == val3);
    EXPECT_TRUE(val3 == val1);
    EXPECT_TRUE(!(val1 != val3));
    EXPECT_TRUE(!(val3 != val1));

    expect<throw_on_copy> val4{common_error::kInvalidArgument};

    EXPECT_TRUE(!val4.equal(throw_on_copy{}));
    EXPECT_TRUE(!(val4 == throw_on_copy{}));
    EXPECT_TRUE(!(throw_on_copy{} == val4));
    EXPECT_TRUE(val4 != throw_on_copy{});
    EXPECT_TRUE(throw_on_copy{} != val4);
    EXPECT_TRUE(!val4.equal(val3));
    EXPECT_TRUE(!(val4 == val3));
    EXPECT_TRUE(!(val3 == val4));
    EXPECT_TRUE(val4 != val3);
    EXPECT_TRUE(val3 != val4);
}

TEST(Expect, Macros) {
    EXPECT_TRUE(
        [] () -> ::common_error {
            MONERO_PRECOND(true);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> ::common_error {
            MONERO_PRECOND(false);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );
    EXPECT_TRUE(
        [] () -> std::error_code {
            MONERO_PRECOND(true);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> std::error_code {
            MONERO_PRECOND(false);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );
    EXPECT_TRUE(
        [] () -> expect<void> {
            MONERO_PRECOND(true);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> expect<void> {
            MONERO_PRECOND(false);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );
    EXPECT_TRUE(
        [] () -> expect<int> {
            MONERO_PRECOND(true);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> expect<int> {
            MONERO_PRECOND(false);
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );

    EXPECT_TRUE(
        [] () -> std::error_code {
            MONERO_CHECK(expect<void>{});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> std::error_code {
            MONERO_CHECK(expect<void>{common_error::kInvalidArgument});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );
    EXPECT_TRUE(
        [] () -> expect<void> {
            MONERO_CHECK(expect<void>{});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> expect<void> {
            MONERO_CHECK(expect<void>{common_error::kInvalidArgument});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );
    EXPECT_TRUE(
        [] () -> expect<int> {
            MONERO_CHECK(expect<void>{});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidErrorCode
    );
    EXPECT_TRUE(
        [] () -> expect<int> {
            MONERO_CHECK(expect<void>{common_error::kInvalidArgument});
            return {common_error::kInvalidErrorCode};
        } () == common_error::kInvalidArgument
    );

    EXPECT_NO_THROW(MONERO_UNWRAP(success()));
    EXPECT_NO_THROW(MONERO_UNWRAP(expect<void>{}));
    EXPECT_NO_THROW(MONERO_UNWRAP(expect<int>{0}));
    EXPECT_THROW(
        MONERO_UNWRAP(expect<void>{common_error::kInvalidArgument}), std::system_error
    );
    EXPECT_THROW(
        MONERO_UNWRAP(expect<int>{common_error::kInvalidArgument}), std::system_error
    );
}

