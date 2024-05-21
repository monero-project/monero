// Copyright (c) 2014-2024, The Monero Project
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

#include <boost/range/algorithm_ext/iota.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <gtest/gtest.h>

#include "blockchain_db/lmdb/db_lmdb.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "hex.h"
#include "lmdb/database.h"
#include "lmdb/table.h"
#include "lmdb/transaction.h"
#include "lmdb/util.h"
#include "string_tools.h"

namespace
{
    enum class choice : unsigned {};
    enum class big_choice : unsigned long {};

    struct bytes {
        char data[16];
    };

    MONERO_CURSOR(test_cursor);

    template<typename T>
    int run_compare(T left, T right, MDB_cmp_func* cmp)
    {
        MDB_val left_val = lmdb::to_val(left);
        MDB_val right_val = lmdb::to_val(right);
        return (*cmp)(&left_val, &right_val);
    }

    crypto::hash postfix_hex_to_hash(const std::string& hex)
    {
        if (hex.size() > 64) throw std::logic_error("postfix_hex_to_hash");
        std::string decoded_bytes;
        if (!epee::from_hex::to_string(decoded_bytes, hex)) throw std::logic_error("postfix_hex_to_hash");
        crypto::hash res = crypto::null_hash;
        memcpy(res.data + 32 - decoded_bytes.size(), decoded_bytes.data(), decoded_bytes.size());
        return res;
    }

    void test_make_template(const std::string& input_hex, unsigned int nbits, const std::string& expected_hex)
    {
        const crypto::hash input = postfix_hex_to_hash(input_hex);
        const crypto::hash expected = postfix_hex_to_hash(expected_hex);
        const crypto::hash actual = cryptonote::make_hash32_loose_template(nbits, input);
        ASSERT_EQ(expected, actual);
    }
}

TEST(LMDB, Traits)
{
    EXPECT_TRUE((std::is_same<void, lmdb::identity<void>::type>()));
    EXPECT_TRUE((std::is_same<unsigned, lmdb::identity<unsigned>::type>()));

    EXPECT_TRUE((std::is_same<void, lmdb::native_type<void>>()));
    EXPECT_TRUE((std::is_same<unsigned, lmdb::native_type<unsigned>>()));
    EXPECT_TRUE((std::is_same<unsigned, lmdb::native_type<choice>>()));
    EXPECT_TRUE((std::is_same<unsigned long, lmdb::native_type<big_choice>>()));
}

TEST(LMDB, ToNative)
{
    enum class negative_choice : int {};

    EXPECT_TRUE((std::is_same<unsigned, decltype(lmdb::to_native(choice(0)))>()));
    EXPECT_TRUE(
        (std::is_same<unsigned long, decltype(lmdb::to_native(big_choice(0)))>())
    );
    EXPECT_TRUE(
        (std::is_same<int, decltype(lmdb::to_native(negative_choice(0)))>())
    );

    EXPECT_EQ(unsigned(0), lmdb::to_native(choice(0)));
    EXPECT_EQ(unsigned(0xffffffff), lmdb::to_native(choice(0xffffffff)));
    EXPECT_EQ(-1, lmdb::to_native(negative_choice(-1)));

    // test constexpr 
    static_assert(100 == lmdb::to_native(choice(100)), "to_native failed");
    static_assert(-100 == lmdb::to_native(negative_choice(-100)), "to_native failed");
}

TEST(LMDB, Conversions)
{
    struct one
    {
        big_choice i;
        choice j;
    };

    const one test{big_choice(100), choice(95)};
    one test2{big_choice(1000), choice(950)};

    EXPECT_EQ(&test, lmdb::to_val(test).mv_data);
    EXPECT_NE(&test2, lmdb::to_val(test).mv_data);
    EXPECT_EQ(
        &test,
        static_cast<const void*>(lmdb::to_byte_span(lmdb::to_val(test)).begin())
    );
    EXPECT_EQ(sizeof(test), lmdb::to_val(test).mv_size);
    EXPECT_EQ(sizeof(test), lmdb::to_byte_span(lmdb::to_val(test)).size());

    EXPECT_EQ(&test2, lmdb::to_val(test2).mv_data);
    EXPECT_NE(&test, lmdb::to_val(test2).mv_data);
    EXPECT_EQ(
        &test2,
        static_cast<const void*>(lmdb::to_byte_span(lmdb::to_val(test2)).begin())
    );
    EXPECT_EQ(sizeof(test2), lmdb::to_val(test2).mv_size);
    EXPECT_EQ(sizeof(test2), lmdb::to_byte_span(lmdb::to_val(test2)).size());
}

TEST(LMDB, LessSort)
{
    struct one
    {
        unsigned i;
        unsigned j;
    };

    struct two
    {
        unsigned i;
        choice j;
    };

    EXPECT_EQ(0, run_compare(0u, 0u, &lmdb::less<unsigned>));
    EXPECT_EQ(-1, run_compare(0u, 1u, &lmdb::less<unsigned>));
    EXPECT_EQ(1, run_compare(1u, 0u, &lmdb::less<unsigned>));

    EXPECT_EQ(0, run_compare<one>({0, 1}, {0, 1}, &lmdb::less<unsigned, sizeof(unsigned)>));
    EXPECT_EQ(-1, run_compare<one>({0, 0}, {0, 1}, &lmdb::less<unsigned, sizeof(unsigned)>));
    EXPECT_EQ(1, run_compare<one>({0, 1}, {0, 0}, &lmdb::less<unsigned, sizeof(unsigned)>));

    EXPECT_EQ(0, run_compare<one>({0, 1}, {0, 1}, MONERO_SORT_BY(one, j)));
    EXPECT_EQ(-1, run_compare<one>({0, 0}, {0, 1}, MONERO_SORT_BY(one, j)));
    EXPECT_EQ(1, run_compare<one>({0, 1}, {0, 0}, MONERO_SORT_BY(one, j)));

    EXPECT_EQ(0, run_compare<two>({0, choice(1)}, {0, choice(1)}, MONERO_SORT_BY(two, j)));
    EXPECT_EQ(-1, run_compare<two>({0, choice(0)}, {0, choice(1)}, MONERO_SORT_BY(two, j)));
    EXPECT_EQ(1, run_compare<two>({0, choice(1)}, {0, choice(0)}, MONERO_SORT_BY(two, j)));

    // compare function addresses
    EXPECT_EQ((MONERO_SORT_BY(one, i)), (MONERO_SORT_BY(two, i)));
    EXPECT_EQ((MONERO_SORT_BY(one, j)), (MONERO_SORT_BY(two, j)));
    EXPECT_NE((MONERO_SORT_BY(one, i)), (MONERO_SORT_BY(two, j)));
    EXPECT_NE((MONERO_SORT_BY(one, j)), (MONERO_SORT_BY(two, i)));
}

TEST(LMDB, SortCompare)
{
    struct one
    {
        unsigned i;
        bytes j;
    };

    one test{55};
    boost::iota(test.j.data, 10);

    const one test2 = test;

    EXPECT_EQ(0, run_compare(test, test2, MONERO_COMPARE(one, j)));

    test.j.data[15] = 1;
    EXPECT_GT(0, run_compare(test, test2, MONERO_COMPARE(one, j)));

    test.j.data[15] = 100;
    EXPECT_LT(0, run_compare(test, test2, MONERO_COMPARE(one, j)));
}

TEST(LMDB, Table)
{
    struct one
    {
        bytes i;
        bytes j;
    };

    constexpr lmdb::basic_table<choice, bytes> test{"foo"};

    EXPECT_STREQ("foo", test.name);
    static_assert(test.flags == 0, "bad flags");
    static_assert(&lmdb::less<unsigned> == test.key_cmp, "bad key_cmp");
    static_assert(test.value_cmp == nullptr, "bad value_cmp");
    EXPECT_TRUE(test.get_value<bytes>(MDB_val{}).matches(std::errc::invalid_argument));

    lmdb::basic_table<big_choice, one> test2{
        "foo2", MDB_DUPSORT, &lmdb::compare<one>
    };

    EXPECT_STREQ("foo2", test2.name);
    EXPECT_EQ((MDB_DUPSORT | MDB_DUPFIXED), test2.flags);
    EXPECT_EQ(&lmdb::less<unsigned long>, test2.key_cmp);
    EXPECT_EQ(&lmdb::compare<one>, test2.value_cmp);
    EXPECT_TRUE(test2.get_value<one>(MDB_val{}).matches(std::errc::invalid_argument));

    one record{};
    boost::iota(record.i.data, 0);
    boost::iota(record.i.data, 20);

    const one record_copy = MONERO_UNWRAP(test2.get_value<one>(lmdb::to_val(record)));
    EXPECT_TRUE(boost::equal(record.i.data, record_copy.i.data));
    EXPECT_TRUE(boost::equal(record.j.data, record_copy.j.data));

    const bytes j_copy = MONERO_UNWRAP(
        test2.get_value<MONERO_FIELD(one, j)>(lmdb::to_val(record))
    );
    EXPECT_TRUE(boost::equal(record.j.data, j_copy.data));

    EXPECT_TRUE(
        test.get_key_stream(test_cursor{}).matches(std::errc::invalid_argument)
    );
    EXPECT_TRUE(
        test2.get_key_stream(test_cursor{}).matches(std::errc::invalid_argument)
    );


    EXPECT_TRUE(
        test.get_value_stream(choice(0), test_cursor{}).matches(std::errc::invalid_argument)
    );
    EXPECT_TRUE(
        test2.get_value_stream(big_choice(0), test_cursor{}).matches(std::errc::invalid_argument)
    );
}

TEST(LMDB, InvalidDatabase)
{
   lmdb::database test{lmdb::environment{}};

    EXPECT_TRUE(test.resize().matches(std::errc::invalid_argument));
    EXPECT_TRUE(test.create_read_txn().matches(std::errc::invalid_argument));
    EXPECT_TRUE(test.reset_txn(lmdb::read_txn{}).matches(std::errc::invalid_argument));
    EXPECT_TRUE(test.create_write_txn().matches(std::errc::invalid_argument));
    EXPECT_TRUE(test.commit(lmdb::write_txn{}).matches(std::errc::invalid_argument));

    EXPECT_TRUE(
        test.try_write( [](MDB_txn&) { return success(); } ).matches(std::errc::invalid_argument)
    );
}

TEST(LMDB, InvalidValueStream)
{
    struct one
    {
        choice i;
        choice j;
        bytes k;
    };

    lmdb::value_stream<one, close_test_cursor> test{test_cursor{}};

    EXPECT_TRUE((std::is_same<one, decltype(*(test.make_iterator()))>()));
    EXPECT_TRUE((std::is_same<one, decltype(*(test.make_range().begin()))>()));
    EXPECT_TRUE(
        (std::is_same<bytes, decltype(*(test.make_iterator<MONERO_FIELD(one, k)>()))>())
    );
    EXPECT_TRUE(
        (std::is_same<bytes, decltype(*(test.make_range<MONERO_FIELD(one, k)>().begin()))>())
    );

    EXPECT_NO_THROW(test.reset());
    EXPECT_EQ(0u, test.count());
    EXPECT_TRUE(test.make_iterator().is_end());
    EXPECT_TRUE(test.make_range().empty());
    EXPECT_EQ(nullptr, test.give_cursor());

    EXPECT_EQ(0u, test.count());
    EXPECT_TRUE(test.make_iterator().is_end());
    EXPECT_TRUE(test.make_range().empty());
    EXPECT_EQ(nullptr, test.give_cursor());
}

TEST(LMDB, InvalidValueIterator)
{
    struct one
    {
        choice i;
        choice j;
        bytes k;
    };

    lmdb::value_iterator<one> test1{};

    EXPECT_TRUE((std::is_same<one, decltype(*test1)>()));
    EXPECT_TRUE(
        (std::is_same<bytes, decltype(test1.get_value<MONERO_FIELD(one, k)>())>())
    );

    EXPECT_TRUE(test1.is_end());
    EXPECT_NO_THROW(++test1);
    EXPECT_NO_THROW(test1++);
    EXPECT_TRUE(test1.is_end());

    lmdb::value_iterator<one> test2{nullptr};

    EXPECT_TRUE(test2.is_end());
    EXPECT_NO_THROW(++test2);
    EXPECT_NO_THROW(test2++);
    EXPECT_TRUE(test2.is_end());

    EXPECT_TRUE(test1.equal(test2));
    EXPECT_TRUE(test2.equal(test1));
    EXPECT_TRUE(test1 == test2);
    EXPECT_TRUE(test2 == test1);
    EXPECT_FALSE(test1 != test2);
    EXPECT_FALSE(test2 != test1);

    lmdb::value_iterator<MONERO_FIELD(one, k)> test3{};

    EXPECT_TRUE((std::is_same<bytes, decltype(*test3)>()));
    EXPECT_TRUE((std::is_same<one, decltype(test3.get_value<one>())>()));
    EXPECT_TRUE(
        (std::is_same<choice, decltype(test1.get_value<MONERO_FIELD(one, j)>())>())
    );

    EXPECT_TRUE(test3.is_end());
    EXPECT_NO_THROW(++test3);
    EXPECT_NO_THROW(test3++);
    EXPECT_TRUE(test3.is_end());
}

TEST(LMDB, InvalidKeyStream)
{
    struct one
    {
        choice i;
        choice j;
        bytes k;
    };

    using record = std::pair<choice, boost::iterator_range<lmdb::value_iterator<one>>>;
    
    lmdb::key_stream<choice, one, close_test_cursor> test{test_cursor{}};

    EXPECT_TRUE((std::is_same<record, decltype(*(test.make_iterator()))>()));
    EXPECT_TRUE((std::is_same<record, decltype(*(test.make_range().begin()))>()));

    EXPECT_NO_THROW(test.reset());
    EXPECT_TRUE(test.make_iterator().is_end());
    EXPECT_TRUE(test.make_range().empty());
    EXPECT_EQ(nullptr, test.give_cursor());

    EXPECT_TRUE(test.make_iterator().is_end());
    EXPECT_TRUE(test.make_range().empty());
    EXPECT_EQ(nullptr, test.give_cursor());
}

TEST(LMDB, InvalidKeyIterator)
{
    struct one
    {
        choice i;
        choice j;
        bytes k;
    };

    using record = std::pair<choice, boost::iterator_range<lmdb::value_iterator<one>>>;

    lmdb::key_iterator<choice, one> test1{};

    EXPECT_TRUE((std::is_same<record, decltype(*test1)>()));
    EXPECT_TRUE((std::is_same<choice, decltype(test1.get_key())>()));
    EXPECT_TRUE((std::is_same<one, decltype(*(test1.make_value_iterator()))>()));
    EXPECT_TRUE((std::is_same<one, decltype(*(test1.make_value_range().begin()))>()));
    EXPECT_TRUE(
        (std::is_same<bytes, decltype(*(test1.make_value_iterator<MONERO_FIELD(one, k)>()))>())
    );
    EXPECT_TRUE(
        (std::is_same<bytes, decltype(*(test1.make_value_range<MONERO_FIELD(one, k)>().begin()))>())
    );

    EXPECT_TRUE(test1.is_end());
    EXPECT_NO_THROW(++test1);
    EXPECT_NO_THROW(test1++);
    EXPECT_TRUE(test1.is_end());
    EXPECT_TRUE(test1.make_value_iterator().is_end());
    EXPECT_TRUE(test1.make_value_range().empty());

    lmdb::key_iterator<choice, one> test2{nullptr};

    EXPECT_TRUE(test2.is_end());
    EXPECT_NO_THROW(++test2);
    EXPECT_NO_THROW(test2++);
    EXPECT_TRUE(test2.is_end());
    EXPECT_TRUE(test2.make_value_iterator().is_end());
    EXPECT_TRUE(test2.make_value_range().empty());

    EXPECT_TRUE(test1.equal(test2));
    EXPECT_TRUE(test2.equal(test1));
    EXPECT_TRUE(test1 == test2);
    EXPECT_TRUE(test2 == test1);
    EXPECT_FALSE(test1 != test2);
    EXPECT_FALSE(test2 != test1);
}

TEST(LMDB_kanonymity, compare_hash32_reversed_nbits)
{
    static constexpr size_t NUM_RANDOM_HASHES = 128;
    std::vector<crypto::hash> random_hashes;
    random_hashes.reserve(500);
    for (size_t i = 0; i < NUM_RANDOM_HASHES; ++i)
        random_hashes.push_back(crypto::rand<crypto::hash>());

    bool r = true;

    // Compare behavior of compare_hash32_reversed_nbits(nbits=256) to BlockchainLMDB::compare_hash32
    for (size_t i = 0; i < NUM_RANDOM_HASHES; ++i)
    {
        for (size_t j = 0; j < NUM_RANDOM_HASHES; ++j)
        {
            const crypto::hash& ha = random_hashes[i];
            const crypto::hash& hb = random_hashes[j];
            const MDB_val mva = {sizeof(crypto::hash), (void*)(&ha)};
            const MDB_val mvb = {sizeof(crypto::hash), (void*)(&hb)};
            const int expected = cryptonote::BlockchainLMDB::compare_hash32(&mva, &mvb);
            const int actual = cryptonote::compare_hash32_reversed_nbits(ha, hb, 256);
            if (actual != expected)
            {
                std::cerr << "Failed compare_hash32_reversed_nbits test case with hashes:" << std::endl;
                std::cerr << "    " << epee::string_tools::pod_to_hex(ha) << std::endl;
                std::cerr << "    " << epee::string_tools::pod_to_hex(hb) << std::endl;
                r = false;
            }
            EXPECT_EQ(expected, actual);
        }
    }

    ASSERT_TRUE(r);

    const auto cmp_byte_rev = [](const crypto::hash& ha, const crypto::hash& hb, unsigned int nbytes) -> int
    {
        if (nbytes > sizeof(crypto::hash)) throw std::logic_error("can't compare with nbytes too big");
        const uint8_t* va = (const uint8_t*)ha.data;
        const uint8_t* vb = (const uint8_t*)hb.data;
        for (size_t i = 31; nbytes; --i, --nbytes)
        {
            if (va[i] < vb[i]) return -1;
            else if (va[i] > vb[i]) return 1;
        }
        return 0;
    };

    // Test partial hash compares w/o partial bytes
    for (size_t i = 0; i < NUM_RANDOM_HASHES; ++i)
    {
        for (size_t j = 0; j < NUM_RANDOM_HASHES; ++j)
        {
            for (unsigned int nbytes = 0; nbytes <= 32; ++nbytes)
            {
                const crypto::hash& ha = random_hashes[i];
                const crypto::hash& hb = random_hashes[j];
                const int expected = cmp_byte_rev(ha, hb, nbytes);
                const int actual = cryptonote::compare_hash32_reversed_nbits(ha, hb, nbytes * 8);
                if (actual != expected)
                {
                    std::cerr << "Failed compare_hash32_reversed_nbits test case with hashes and args:" << std::endl;
                    std::cerr << "    " << epee::string_tools::pod_to_hex(ha) << std::endl;
                    std::cerr << "    " << epee::string_tools::pod_to_hex(hb) << std::endl;
                    std::cerr << "    nbytes=" << nbytes << std::endl;
                    r = false;
                }
                EXPECT_EQ(expected, actual);
            }
        }
    }

    ASSERT_TRUE(r);

    // Test partial hash compares w/ partial bytes
    for (size_t i = 0; i < NUM_RANDOM_HASHES; ++i)
    {
        const crypto::hash& ha = random_hashes[i];
        for (size_t modnbytes = 0; modnbytes < 32; ++modnbytes)
        {
            for (size_t modbitpos = 0; modbitpos < 8; ++modbitpos)
            {
                const size_t modbytepos = 31 - modnbytes;
                const uint8_t mask = 1 << modbitpos;
                const bool bit_was_zero = 0 == (static_cast<uint8_t>(ha.data[modbytepos]) & mask);
                const unsigned int modnbits = modnbytes * 8 + (7 - modbitpos);

                // Create modified random hash by flipping one bit
                crypto::hash hb = ha;
                hb.data[modbytepos] = static_cast<uint8_t>(hb.data[modbytepos]) ^ mask;

                for (unsigned int cmpnbits = 0; cmpnbits <= 256; ++cmpnbits)
                {
                    const int expected = cmpnbits <= modnbits ? 0 : bit_was_zero ? -1 : 1;
                    const int actual = cryptonote::compare_hash32_reversed_nbits(ha, hb, cmpnbits);
                    if (actual != expected)
                    {
                        std::cerr << "Failed compare_hash32_reversed_nbits test case with hashes and args:" << std::endl;
                        std::cerr << "    " << epee::string_tools::pod_to_hex(ha) << std::endl;
                        std::cerr << "    " << epee::string_tools::pod_to_hex(hb) << std::endl;
                        std::cerr << "    modnbytes=" << modnbytes << std::endl;
                        std::cerr << "    modbitpos=" << modbitpos << std::endl;
                        std::cerr << "    cmpnbits=" << cmpnbits << std::endl;
                        r = false;
                    }
                    EXPECT_EQ(expected, actual);
                }
            }
        }
    }

    ASSERT_TRUE(r);

    // Test equality
    for (size_t i = 0; i < NUM_RANDOM_HASHES; ++i)
    {
        const crypto::hash& ha = random_hashes[i];
        for (unsigned int nbits = 0; nbits <= 256; ++nbits)
        {
            const int actual = cryptonote::compare_hash32_reversed_nbits(ha, ha, nbits);
            if (actual)
            {
                std::cerr << "Failed compare_hash32_reversed_nbits test case with hash and args:" << std::endl;
                std::cerr << "    " << epee::string_tools::pod_to_hex(ha) << std::endl;
                std::cerr << "    nbits=" << nbits << std::endl;
                r = false;
            }
            EXPECT_EQ(0, actual);
        }
    }

}

TEST(LMDB_kanonymity, make_hash32_loose_template)
{
    const std::string example_1 = "0abcdef1234567890abcdef1234567890abcdef1234567890abcdef123456789";

    test_make_template(example_1, 0, "");

    test_make_template(example_1, 1, "80");
    test_make_template(example_1, 2, "80");
    test_make_template(example_1, 3, "80");
    test_make_template(example_1, 4, "80");
    test_make_template(example_1, 5, "88");
    test_make_template(example_1, 6, "88");
    test_make_template(example_1, 7, "88");
    test_make_template(example_1, 8, "89");

    test_make_template(example_1, 9, "0089");
    test_make_template(example_1, 10, "4089");
    test_make_template(example_1, 11, "6089");
    test_make_template(example_1, 12, "6089");
    test_make_template(example_1, 13, "6089");
    test_make_template(example_1, 14, "6489");
    test_make_template(example_1, 15, "6689");
    test_make_template(example_1, 16, "6789");

    test_make_template(example_1, 32, "23456789");
    test_make_template(example_1, 64, "0abcdef123456789");
    test_make_template(example_1, 128, "0abcdef1234567890abcdef123456789");
    test_make_template(example_1, 256, example_1);
}
