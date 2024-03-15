// Copyright (c) 2020-2024, The Monero Project

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

#include "tests/benchmark.h"

#include <boost/fusion/adapted/std_tuple.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/seq/seq.hpp>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/spirit/include/karma_char.hpp>
#include <boost/spirit/include/karma_format.hpp>
#include <boost/spirit/include/karma_repeat.hpp>
#include <boost/spirit/include/karma_right_alignment.hpp>
#include <boost/spirit/include/karma_sequence.hpp>
#include <boost/spirit/include/karma_string.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/qi_char.hpp>
#include <boost/spirit/include/qi_list.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_uint.hpp>
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "crypto/crypto.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "monero/crypto/amd64-64-24k.h"
#include "monero/crypto/amd64-51-30k.h"

#define CHECK(...)                           \
    if(!( __VA_ARGS__ ))                      \
        throw std::runtime_error{             \
            "TEST FAILED (line  "             \
            BOOST_PP_STRINGIZE( __LINE__ )    \
            "): "                             \
            BOOST_PP_STRINGIZE( __VA_ARGS__ ) \
        }

//! Define function that forwards arguments to `crypto::func`.
#define FORWARD_FUNCTION(func)                             \
    template<typename... T>                                \
    static bool func (T&&... args)                         \
    {                                                      \
        return ::crypto:: func (std::forward<T>(args)...); \
    }

#define CRYPTO_FUNCTION(library, func)                        \
    BOOST_PP_CAT(BOOST_PP_CAT(monero_crypto_, library), func)

#define CRYPTO_BENCHMARK(r, _, library)                                                                                                                                  \
    struct library                                                                                                                                                       \
    {                                                                                                                                                                    \
        static constexpr const char* name() noexcept { return BOOST_PP_STRINGIZE(library); }                                                                             \
        static bool generate_key_derivation(const ::crypto::public_key &tx_pub, const ::crypto::secret_key &view_sec, ::crypto::key_derivation &out)                     \
        {                                                                                                                                                                \
            return CRYPTO_FUNCTION(library, _generate_key_derivation) (out.data, tx_pub.data, view_sec.data) == 0;                                                       \
        }                                                                                                                                                                \
        static bool derive_subaddress_public_key(const ::crypto::public_key &spend_pub, const ::crypto::key_derivation &d, std::size_t index, ::crypto::public_key &out) \
        {                                                                                                                                                                \
            ::crypto::ec_scalar scalar;                                                                                                                                  \
            ::crypto::derivation_to_scalar(d, index, scalar);                                                                                                            \
            return CRYPTO_FUNCTION(library, _generate_subaddress_public_key) (out.data, spend_pub.data, scalar.data) == 0;                                               \
        }                                                                                                                                                                \
    };


namespace
{
    //! Default number of iterations for benchmark timing.
    constexpr const unsigned default_iterations = 1000;

    //! \return Byte compare two objects of `T`.
    template<typename T>
    bool compare(const T& lhs, const T& rhs) noexcept
    {
        static_assert(!epee::has_padding<T>(), "type might have padding");
        return std::memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(T)) == 0;
    }

    //! Benchmark default monero crypto library - a re-arranged ref10 implementation.
    struct cn
    {
        static constexpr const char* name() noexcept { return "cn"; }
        FORWARD_FUNCTION( generate_key_derivation );
        FORWARD_FUNCTION( derive_subaddress_public_key );
    };

    // Define functions for every library except for `cn` which is the head library.
    BOOST_PP_SEQ_FOR_EACH(CRYPTO_BENCHMARK, _, BOOST_PP_SEQ_TAIL(BENCHMARK_LIBRARIES));

    // All enabled benchmark libraries
    using enabled_libraries = std::tuple<BOOST_PP_SEQ_ENUM(BENCHMARK_LIBRARIES)>;


    //! Callable that runs a benchmark against all enabled libraries
    template<typename R>
    struct run_benchmark
    {
        using result = R;

        template<typename B>
        result operator()(result out, const B benchmark) const
        {
            using inner_result = typename B::result;
            out.push_back({boost::fusion::fold(enabled_libraries{}, inner_result{}, benchmark), benchmark.name()});
            std::sort(out.back().first.begin(), out.back().first.end());
            return out;
        }
    };

    //! Run 0+ benchmarks against all enabled libraries
    template<typename R, typename... B>
    R run_benchmarks(B&&... benchmarks)
    {
        auto out = boost::fusion::fold(std::make_tuple(std::forward<B>(benchmarks)...), R{}, run_benchmark<R>{});
        std::sort(out.begin(), out.end());
        return out;
    }

    //! Run a suite of benchmarks - allows for comparison against a subset of benchmarks
    template<typename S>
    std::pair<typename S::result, std::string> run_suite(const S& suite)
    {
        return {suite(), suite.name()};
    }

    //! Arguments given to every crypto library being benchmarked.
    struct bench_args
    {
        explicit bench_args(unsigned iterations)
          : iterations(iterations), one(), two()
        {
          crypto::generate_keys(one.pub, one.sec, one.sec, false);
          crypto::generate_keys(two.pub, two.sec, two.sec, false);
        }

        const unsigned iterations;
        cryptonote::keypair one;
        cryptonote::keypair two;
    };

    /*! Tests the ECDH step used for monero txes where the tx-pub is always
        de-compressed into a table every time. */
    struct tx_pub_standard
    {
        using result = std::vector<std::pair<std::chrono::steady_clock::duration, std::string>>;
        static constexpr const char* name() noexcept { return "standard"; }

        const bench_args args;

        template<typename L>
        result operator()(result out, const L library) const
        {
            crypto::key_derivation us;
            crypto::key_derivation them;
            CHECK(crypto::generate_key_derivation(args.one.pub, args.two.sec, them));
            CHECK(library.generate_key_derivation(args.one.pub, args.two.sec, us));
            CHECK(compare(us, them));

            unsigned i = 0;
            for (unsigned j = 0; j < 100; ++j)
                i += library.generate_key_derivation(args.one.pub, args.two.sec, us);
            CHECK(i == 100);

            i = 0;
            const auto start = std::chrono::steady_clock::now();
            for (unsigned j = 0; j < args.iterations; ++j)
                i += library.generate_key_derivation(args.one.pub, args.two.sec, us);
            const auto end = std::chrono::steady_clock::now();
            CHECK(i == args.iterations);
            CHECK(compare(us, them));

            out.push_back({end - start, library.name()});
            return out;
        }
    };

    //! Tests various possible optimizations for tx ECDH-step.
    struct tx_pub_suite
    {
        using result = std::vector<std::pair<tx_pub_standard::result, std::string>>;
        static constexpr const char* name() noexcept { return "generate_key_derivation step"; }

        const bench_args args;

        result operator()() const
        {
           return run_benchmarks<result>(tx_pub_standard{args});
        }
    };

    /*! Tests the shared-secret to output-key step used for monero txes where
        the users spend-public is always de-compressed. */
    struct output_pub_standard
    {
        using result = std::vector<std::pair<std::chrono::steady_clock::duration, std::string>>;
        static constexpr const char* name() noexcept { return "standard"; }

        const bench_args args;

        template<typename L>
        result operator()(result out, const L library) const
        {
            crypto::key_derivation derived;
            crypto::public_key us;
            crypto::public_key them;
            CHECK(crypto::generate_key_derivation(args.one.pub, args.two.sec, derived));
            CHECK(library.derive_subaddress_public_key(args.two.pub, derived, 0, us));
            CHECK(crypto::derive_subaddress_public_key(args.two.pub, derived, 0, them));
            CHECK(compare(us, them));

            unsigned i = 0;
            for (unsigned j = 0; j < 100; ++j)
                i += library.derive_subaddress_public_key(args.two.pub, derived, j, us);
            CHECK(i == 100);

            i = 0;
            const auto start = std::chrono::steady_clock::now();
            for (unsigned j = 0; j < args.iterations; ++j)
                i += library.derive_subaddress_public_key(args.two.pub, derived, j, us);
            const auto end = std::chrono::steady_clock::now();
            CHECK(i == args.iterations);

            out.push_back({end - start, library.name()});
            return out;
        }
    };

    //! Tests various possible optimizations for shared-secret to output-key step.
    struct output_pub_suite
    {
        using result = std::vector<std::pair<output_pub_standard::result, std::string>>;
        static constexpr const char* name() noexcept { return "derive_subaddress_public_key step"; }

        const bench_args args;

        result operator()() const
        {
            return run_benchmarks<result>(output_pub_standard{args});
        }
    };

    struct tx_bench_args
    {
        const bench_args main;
        unsigned outputs;
    };

    /*! Simulates "standard" tx scanning where a tx-pubkey is de-compressed into
        a table and user spend-public is de-compressed, every time. */
    struct tx_standard
    {
        using result = std::vector<std::pair<std::chrono::steady_clock::duration, std::string>>;
        static constexpr const char* name() noexcept { return "standard"; }

        const tx_bench_args args;

        template<typename L>
        result operator()(result out, const L library) const
        {
            crypto::key_derivation derived_us;
            crypto::key_derivation derived_them;
            crypto::public_key us;
            crypto::public_key them;
            CHECK(library.generate_key_derivation(args.main.one.pub, args.main.two.sec, derived_us));
            CHECK(crypto::generate_key_derivation(args.main.one.pub, args.main.two.sec, derived_them));
            CHECK(library.derive_subaddress_public_key(args.main.two.pub, derived_us, 0, us));
            CHECK(crypto::derive_subaddress_public_key(args.main.two.pub, derived_them, 0, them));
            CHECK(compare(us, them));

            unsigned i = 0;
            for (unsigned j = 0; j < 100; ++j)
            {
                i += library.generate_key_derivation(args.main.one.pub, args.main.two.sec, derived_us);
                i += library.derive_subaddress_public_key(args.main.two.pub, derived_us, j, us);
            }
            CHECK(i == 200);

            i = 0;
            const auto start = std::chrono::steady_clock::now();
            for (unsigned j = 0; j < args.main.iterations; ++j)
            {
                i += library.generate_key_derivation(args.main.one.pub, args.main.two.sec, derived_us);
                for (unsigned k = 0; k < args.outputs; ++k)
                    i += library.derive_subaddress_public_key(args.main.two.pub, derived_us, k, us);
            }
            const auto end = std::chrono::steady_clock::now();
            CHECK(i == args.main.iterations + args.main.iterations * args.outputs);

            out.push_back({end - start, library.name()});
            return out;
        }
    };

    //! Tests various possible optimizations for tx scanning.
    struct tx_suite
    {
        using result = std::vector<std::pair<output_pub_standard::result, std::string>>;
        std::string name() const { return "Transactions with " + std::to_string(args.outputs) + " outputs"; }

        const tx_bench_args args;

        result operator()() const
        {
            return run_benchmarks<result>(tx_standard{args});

        }
    };

    std::chrono::steady_clock::duration print(const tx_pub_standard::result& leaf, std::ostream& out, unsigned depth)
    {
        namespace karma = boost::spirit::karma;
        const std::size_t align = leaf.empty() ?
            0 : std::to_string(leaf.back().first.count()).size();
        const auto best = leaf.empty() ?
            std::chrono::steady_clock::duration::max() : leaf.front().first;
        for (auto const& entry : leaf)
        {
            out << karma::format(karma::repeat(depth ? depth - 1 : 0)["| "]) << '|';
            out << karma::format((karma::right_align(std::min(20u - depth, 20u), '-')["> " << karma::string]), entry.second);
            out << " => " << karma::format((karma::right_align(align)[karma::uint_]), entry.first.count());
            out << " ns (+";
            out << (double((entry.first - best).count()) / best.count()) * 100 << "%)" << std::endl;
        }
        out << karma::format(karma::repeat(depth ? depth - 1 : 0)["| "]) << std::endl;
        return best;
    }

    template<typename T>
    std::chrono::steady_clock::duration
    print(const std::vector<std::pair<T, std::string>>& node, std::ostream& out, unsigned depth)
    {
        auto best = std::chrono::steady_clock::duration::max();
        for (auto const& entry : node)
        {
            std::stringstream buffer{};
            auto last = print(entry.first, buffer, depth + 1);
            if (last != std::chrono::steady_clock::duration::max())
            {
                namespace karma = boost::spirit::karma;
                best = std::min(best, last);
                out << karma::format(karma::repeat(depth)["|-"]);
                out << "+ " << entry.second << ' ';
                out << last.count() << " ns (+";
                out << (double((last - best).count()) / best.count()) * 100 << "%)" << std::endl;
                out << buffer.str();
            }
        }
        return best;
    }
} // anonymous namespace

int main(int argc, char** argv)
{
    using results = std::vector<std::pair<tx_pub_suite::result, std::string>>;
    try
    {
        unsigned iterations = default_iterations;
        std::vector<unsigned> nums{};
        if (2 <= argc) iterations = std::stoul(argv[1]);
        if (3 <= argc)
        {
            namespace qi = boost::spirit::qi;
            if (!qi::parse(argv[2], argv[2] + strlen(argv[2]), (qi::uint_ % ','), nums))
                throw std::runtime_error{"bad tx outputs string"};
        }
        else
        {
            nums = {2, 4};
        }
        std::sort(nums.begin(), nums.end());
        nums.erase(std::unique(nums.begin(), nums.end()), nums.end());

        std::cout << "Running benchmark using " << iterations << " iterations" << std::endl;

        const bench_args args{iterations};

        results val{};

        std::cout << "Transaction Component Benchmarks" << std::endl;
        std::cout << "--------------------------------" << std::endl;
        val.push_back(run_suite(tx_pub_suite{args}));
        val.push_back(run_suite(output_pub_suite{args}));
        std::sort(val.begin(), val.end());
        print(val, std::cout, 0);

        val.clear();
        std::cout << "Transaction Benchmarks" << std::endl;
        std::cout << "----------------------" << std::endl;
        for (const unsigned num : nums)
            val.push_back(run_suite(tx_suite{{args, num}}));
        std::sort(val.begin(), val.end());
        print(val, std::cout, 0);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

