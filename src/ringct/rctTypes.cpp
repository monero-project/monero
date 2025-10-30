// Copyright (c) 2016-2024, Monero Research Labs
//
// Author: Shen Noether <shen.noether@gmx.com>
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

#include <boost/range/algorithm/copy.hpp>
#include "misc_log_ex.h"
#include "cryptonote_config.h"
#include "rctTypes.h"
#include "int-util.h"
#include "serialization/wire.h"
#include "serialization/wire/adapted/vector.h"
#include "serialization/wire/wrappers_impl.h"
#include "serialization/wire/wrapper/array.h"
#include "serialization/wire/wrapper/variant.h"
using namespace crypto;
using namespace std;

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "ringct"

namespace rct {

    //dp 
    //Debug printing for the above types
    //Actually use DP(value) and #define DBG    
    
    void dp(key a) {
        int j = 0;
        printf("\"");
        for (j = 0; j < 32; j++) {
            printf("%02x", (unsigned char)a.bytes[j]);
        }
        printf("\"");
        printf("\n");
    }

    void dp(bool a) {
        printf(" ... %s ... ", a ? "true" : "false");
        printf("\n");
    }

    void dp(const char * a, int l) {
        int j = 0;
        printf("\"");
        for (j = 0; j < l; j++) {
            printf("%02x", (unsigned char)a[j]);
        }
        printf("\"");
        printf("\n");
    }
    void dp(keyV a) {
        size_t j = 0;
        printf("[");
        for (j = 0; j < a.size(); j++) {
            dp(a[j]);
            if (j < a.size() - 1) {
                printf(",");
            }
        }
        printf("]");
        printf("\n");
    }
    void dp(keyM a) {
        size_t j = 0;
        printf("[");
        for (j = 0; j < a.size(); j++) {
            dp(a[j]);
            if (j < a.size() - 1) {
                printf(",");
            }
        }
        printf("]");
        printf("\n");
    }
    void dp(xmr_amount vali) {
        printf("x: ");
        std::cout << vali;
        printf("\n\n");
    }

    void dp(int vali) {
        printf("x: %d\n", vali);
        printf("\n");
    }
    void dp(bits amountb) {
        for (int i = 0; i < 64; i++) {
            printf("%d", amountb[i]);
        }
        printf("\n");

    }

    void dp(const char * st) {
        printf("%s\n", st);
    }

    //Various Conversions 
    
    //uint long long to 32 byte key
    void d2h(key & amounth, const xmr_amount in) {
        sc_0(amounth.bytes);
        memcpy_swap64le(amounth.bytes, &in, 1);
    }
    
    //uint long long to 32 byte key
    key d2h(const xmr_amount in) {
        key amounth;
        d2h(amounth, in);
        return amounth;
    }

    //uint long long to int[64]
    void d2b(bits  amountb, xmr_amount val) {
        int i = 0;
        while (i < 64) {
            amountb[i++] = val & 1;
            val >>= 1;
        }
    }
    
    //32 byte key to uint long long
    // if the key holds a value > 2^64
    // then the value in the first 8 bytes is returned    
    xmr_amount h2d(const key & test) {
        xmr_amount vali = 0;
        int j = 0;
        for (j = 7; j >= 0; j--) {
            vali = (xmr_amount)(vali * 256 + (unsigned char)test.bytes[j]);
        }
        return vali;
    }
    
    //32 byte key to int[64]
    void h2b(bits amountb2, const key & test) {
        int val = 0, i = 0, j = 0;
        for (j = 0; j < 8; j++) {
            val = (unsigned char)test.bytes[j];
            i = 0;
            while (i < 8) {
                amountb2[j*8+i++] = val & 1;
                val >>= 1;
            }
        }
    }
    
    //int[64] to 32 byte key
    void b2h(key & amountdh, const bits amountb2) {
        int byte, i, j;
        for (j = 0; j < 8; j++) {
            byte = 0;
            for (i = 7; i > -1; i--) {
                byte = byte * 2 + amountb2[8 * j + i];
            }
            amountdh[j] = (unsigned char)byte;
        }
        for (j = 8; j < 32; j++) {
            amountdh[j] = (unsigned char)(0x00);
        }
    }
    
    //int[64] to uint long long
    xmr_amount b2d(bits amountb) {
        xmr_amount vali = 0;
        int j = 0;
        for (j = 63; j >= 0; j--) {
            vali = (xmr_amount)(vali * 2 + amountb[j]);
        }
        return vali;
    }

    bool is_rct_simple(int type)
    {
        switch (type)
        {
            case RCTTypeSimple:
            case RCTTypeBulletproof:
            case RCTTypeBulletproof2:
            case RCTTypeCLSAG:
            case RCTTypeBulletproofPlus:
                return true;
            default:
                return false;
        }
    }

    bool is_rct_bulletproof(int type)
    {
        switch (type)
        {
            case RCTTypeBulletproof:
            case RCTTypeBulletproof2:
            case RCTTypeCLSAG:
                return true;
            default:
                return false;
        }
    }

    bool is_rct_bulletproof_plus(int type)
    {
        switch (type)
        {
            case RCTTypeBulletproofPlus:
                return true;
            default:
                return false;
        }
    }

    bool is_rct_borromean(int type)
    {
        switch (type)
        {
            case RCTTypeSimple:
            case RCTTypeFull:
                return true;
            default:
                return false;
        }
    }

    bool is_rct_clsag(int type)
    {
        switch (type)
        {
            case RCTTypeCLSAG:
            case RCTTypeBulletproofPlus:
                return true;
            default:
                return false;
        }
    }

    static size_t n_bulletproof_amounts_base(const size_t L_size, const size_t R_size, const size_t V_size, const size_t max_outputs)
    {
        CHECK_AND_ASSERT_MES(L_size >= 6, 0, "Invalid bulletproof L size");
        CHECK_AND_ASSERT_MES(L_size == R_size, 0, "Mismatched bulletproof L/R size");
        static const size_t extra_bits = 4;
        CHECK_AND_ASSERT_MES((1 << extra_bits) == max_outputs, 0, "log2(max_outputs) is out of date");
        CHECK_AND_ASSERT_MES(L_size <= 6 + extra_bits, 0, "Invalid bulletproof L size");
        CHECK_AND_ASSERT_MES(V_size <= (1u<<(L_size-6)), 0, "Invalid bulletproof V/L");
        CHECK_AND_ASSERT_MES(V_size * 2 > (1u<<(L_size-6)), 0, "Invalid bulletproof V/L");
        CHECK_AND_ASSERT_MES(V_size > 0, 0, "Empty bulletproof");
        return V_size;
    }

    size_t n_bulletproof_amounts(const Bulletproof &proof) { return n_bulletproof_amounts_base(proof.L.size(), proof.R.size(), proof.V.size(), BULLETPROOF_MAX_OUTPUTS); }
    size_t n_bulletproof_plus_amounts(const BulletproofPlus &proof) { return n_bulletproof_amounts_base(proof.L.size(), proof.R.size(), proof.V.size(), BULLETPROOF_PLUS_MAX_OUTPUTS); }

    size_t n_bulletproof_amounts(const std::vector<Bulletproof> &proofs)
    {
        size_t n = 0;
        for (const Bulletproof &proof: proofs)
        {
            size_t n2 = n_bulletproof_amounts(proof);
            CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
            if (n2 == 0)
                return 0;
            n += n2;
        }
        return n;
    }

    size_t n_bulletproof_plus_amounts(const std::vector<BulletproofPlus> &proofs)
    {
        size_t n = 0;
        for (const BulletproofPlus &proof: proofs)
        {
            size_t n2 = n_bulletproof_plus_amounts(proof);
            CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
            if (n2 == 0)
                return 0;
            n += n2;
        }
        return n;
    }

    static size_t n_bulletproof_max_amounts_base(size_t L_size, size_t R_size, size_t max_outputs)
    {
        CHECK_AND_ASSERT_MES(L_size >= 6, 0, "Invalid bulletproof L size");
        CHECK_AND_ASSERT_MES(L_size == R_size, 0, "Mismatched bulletproof L/R size");
        static const size_t extra_bits = 4;
        CHECK_AND_ASSERT_MES((1 << extra_bits) == max_outputs, 0, "log2(max_outputs) is out of date");
        CHECK_AND_ASSERT_MES(L_size <= 6 + extra_bits, 0, "Invalid bulletproof L size");
        return 1 << (L_size - 6);
    }
    size_t n_bulletproof_max_amounts(const Bulletproof &proof) { return n_bulletproof_max_amounts_base(proof.L.size(), proof.R.size(), BULLETPROOF_MAX_OUTPUTS); }
    size_t n_bulletproof_plus_max_amounts(const BulletproofPlus &proof) { return n_bulletproof_max_amounts_base(proof.L.size(), proof.R.size(), BULLETPROOF_PLUS_MAX_OUTPUTS); }

    size_t n_bulletproof_max_amounts(const std::vector<Bulletproof> &proofs)
    {
        size_t n = 0;
        for (const Bulletproof &proof: proofs)
        {
            size_t n2 = n_bulletproof_max_amounts(proof);
            CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
            if (n2 == 0)
                return 0;
            n += n2;
        }
        return n;
    }

    size_t n_bulletproof_plus_max_amounts(const std::vector<BulletproofPlus> &proofs)
    {
        size_t n = 0;
        for (const BulletproofPlus &proof: proofs)
        {
            size_t n2 = n_bulletproof_plus_max_amounts(proof);
            CHECK_AND_ASSERT_MES(n2 < std::numeric_limits<uint32_t>::max() - n, 0, "Invalid number of bulletproofs");
            if (n2 == 0)
                return 0;
            n += n2;
        }
        return n;
    }

    namespace
    {
        template<typename F, typename T>
        void ecdh_tuple_map(F& format, T& self)
        {
            wire::object(format, WIRE_FIELD(mask), WIRE_FIELD(amount));
        }

        template<typename F, typename T, typename U>
        void boro_sig_map(F& format, T& self, U&& s0, U&& s1)
        {
            wire::object(format, WIRE_FIELD(ee), wire::field("s0", s0), wire::field("s1", s1));
        }

        template<typename F, typename T, typename U>
        void range_sig_map(F& format, T& self, U&& Ci)
        {
          wire::object(format, WIRE_FIELD(asig), wire::field("Ci", Ci));
        }

        template<typename F, typename T>
        void bulletproof_map(F& format, T& self)
        {
            wire::object(format,
                WIRE_FIELD(V),
                WIRE_FIELD(A),
                WIRE_FIELD(S),
                WIRE_FIELD(T1),
                WIRE_FIELD(T2),
                WIRE_FIELD(taux),
                WIRE_FIELD(mu),
                WIRE_FIELD(L),
                WIRE_FIELD(R),
                WIRE_FIELD(a),
                WIRE_FIELD(b),
                WIRE_FIELD(t)
            );
        }

        template<typename F, typename T>
        void bulletproof_plus_map(F& format, T& self)
        {
            wire::object(format,
                WIRE_FIELD(V),
                WIRE_FIELD(A),
                WIRE_FIELD(A1),
                WIRE_FIELD(B),
                WIRE_FIELD(r1),
                WIRE_FIELD(s1),
                WIRE_FIELD(d1),
                WIRE_FIELD(L),
                WIRE_FIELD(R)
            );
        }

        template<typename F, typename T>
        void mg_sig_map(F& format, T& self)
        {
            using max_256 = wire::max_element_count<256>;
            wire::object(format,
                wire::field("ss", wire::array<max_256>(std::ref(self.ss))),
                WIRE_FIELD(cc)
            );
        }

        template<typename F, typename T>
        void clsag_map(F& format, T& self)
        {
          wire::object(format, WIRE_FIELD(s), WIRE_FIELD(c1), WIRE_FIELD(D));
        }

        template<typename F, typename T>
        void rct_sig_prunable_map(F& format, T& self)
        {
            using rf_min_size = wire::min_element_sizeof<key64, key64, key64, key>;
            using bf_max = wire::max_element_count<BULLETPROOF_MAX_OUTPUTS>;
            using bf_plus_max = wire::max_element_count<BULLETPROOF_PLUS_MAX_OUTPUTS>;
            using max_inputs_per_tx = wire::max_element_count<WIRE_MAX_INPUTS_PER_TX>;
            using pseudo_outs_min_size = wire::min_element_sizeof<key>;

            // make all arrays required for ZMQ backwards compatability
            wire::object(format,
                wire::field("range_proofs", wire::array<rf_min_size>(std::ref(self.p.rangeSigs))),
                wire::field("bulletproofs", wire::array<bf_max>(std::ref(self.p.bulletproofs))),
                wire::field("bulletproof_plus", wire::array<bf_plus_max>(std::ref(self.p.bulletproofs_plus))),
                wire::field("mlsags", wire::array<max_inputs_per_tx>(std::ref(self.p.MGs))),
                wire::field("clsags", wire::array<max_inputs_per_tx>(std::ref(self.p.CLSAGs))),
                wire::field("pseudo_outs", wire::array<pseudo_outs_min_size>(std::ref(self.pseudo_outs)))
            );
        }

        struct prunable_read
        {
            keyV pseudo_outs;
            rctSigPrunable p;
        };
        void read_bytes(wire::reader& source, prunable_read& dest)
        {
            rct_sig_prunable_map(source, dest);
        }

        struct prunable_write
        {
            const keyV& pseudo_outs;
            const rctSigPrunable& p;

            prunable_write(const keyV& pseudo_outs, const rctSigPrunable& p)
                : pseudo_outs(pseudo_outs), p(p)
            {}
        };
        void write_bytes(wire::writer& dest, const prunable_write source)
        {
            rct_sig_prunable_map(dest, source);
        }

        template<typename F, typename T, typename U, typename V>
        void rct_sig_map(F& format, const prune_wrapper_<T> self, boost::optional<U>& fee, boost::optional<V>& prunable)
        {
            using min_commitment_size = wire::min_element_sizeof<rct::key>;
            using min_ecdh_size = wire::min_element_sizeof<key, key>;
            wire::object(format,
                wire::field("type", std::ref(self.p.get().type)),
                wire::optional_field("encrypted", wire::array<min_ecdh_size>(std::ref(self.p.get().ecdhInfo))),
                wire::optional_field("commitments", wire::array<min_commitment_size>(std::ref(self.p.get().outPk))),
                wire::optional_field("fee", std::ref(fee)),
                wire::optional_field("prunable", std::ref(prunable))
            );
        }
    } // anonymous
    void read_bytes(wire::reader& dest, ctkey& source)
    {
        wire_read::bytes(dest, source.mask);
        source.dest = {};
    }
    void write_bytes(wire::writer& source, const ctkey& dest)
    {
        wire_write::bytes(source, dest.mask);
    }
    WIRE_DEFINE_OBJECT(ecdhTuple, ecdh_tuple_map)
    static void read_bytes(wire::reader& source, rct::boroSig& dest)
    {
        using max_64 = wire::max_element_count<64>;
        std::vector<rct::key> s0;
        std::vector<rct::key> s1;
        s0.reserve(64);
        s1.reserve(64);
        boro_sig_map(source, dest, wire::array<max_64>(std::ref(s0)), wire::array<max_64>(std::ref(s1)));

        if (s0.size() != boost::size(dest.s0) || s1.size() != boost::size(dest.s1))
            WIRE_DLOG_THROW(wire::error::schema::array, "invalid array size");

        boost::range::copy(s0, boost::begin(dest.s0));
        boost::range::copy(s1, boost::begin(dest.s1));
    }
    static void write_bytes(wire::writer& dest, const rct::boroSig& source)
    {
        using key_span = epee::span<const rct::key>;
        boro_sig_map(dest, source, key_span{source.s0}, key_span{source.s1});
    }
    static void read_bytes(wire::reader& source, rct::rangeSig& dest)
    {
        using max_64 = wire::max_element_count<64>;
        std::vector<rct::key> Ci;
        Ci.reserve(64);
        range_sig_map(source, dest, wire::array<max_64>(std::ref(Ci)));

        if (Ci.size() != boost::size(dest.Ci))
            WIRE_DLOG_THROW(wire::error::schema::array, "invalid array size");

        boost::range::copy(Ci, boost::begin(dest.Ci));
    }
    static void write_bytes(wire::writer& dest, const rct::rangeSig& source)
    {
        range_sig_map(dest, source, epee::span<const rct::key>{source.Ci});
    }
    WIRE_DEFINE_OBJECT(Bulletproof, bulletproof_map)
    WIRE_DEFINE_OBJECT(BulletproofPlus, bulletproof_plus_map)
    WIRE_DEFINE_OBJECT(mgSig, mg_sig_map)
    WIRE_DEFINE_OBJECT(clsag, clsag_map);
    void read_bytes(wire::reader& source, prune_wrapper_<rctSig> dest)
    {
        boost::optional<rct::xmr_amount> fee;
        boost::optional<prunable_read> prunable;
        rct_sig_map(source, dest, fee, prunable);

        if (dest.p.get().type == rct::RCTTypeNull && (fee || !dest.p.get().ecdhInfo.empty()))
            WIRE_DLOG_THROW(wire::error::schema::missing_key, "unexpected keys");
        if (dest.p.get().type != rct::RCTTypeNull && (!fee || dest.p.get().ecdhInfo.empty()))
            WIRE_DLOG_THROW(wire::error::schema::missing_key, "expected keys");

        if (fee)
            dest.p.get().txnFee = *fee;

        if (prunable)
        {
            dest.p.get().p = std::move(prunable->p);
            dest.p.get().get_pseudo_outs() = std::move(prunable->pseudo_outs);
        }
        else
        {
            dest.p.get().p.rangeSigs.clear();
            dest.p.get().p.bulletproofs.clear();
            dest.p.get().p.bulletproofs_plus.clear();
            dest.p.get().p.MGs.clear();
            dest.p.get().p.CLSAGs.clear();
            dest.p.get().get_pseudo_outs().clear();
        }
    }
    void write_bytes(wire::writer& dest, const prune_wrapper_<const rctSig> source)
    {
        boost::optional<rct::xmr_amount> fee;
        boost::optional<prunable_write> prunable;

        if (source.p.get().type != rct::RCTTypeNull)
            fee.emplace(source.p.get().txnFee);

        if (!source.prune && (!source.p.get().p.bulletproofs.empty() || !source.p.get().p.bulletproofs_plus.empty() || !source.p.get().p.rangeSigs.empty() || !source.p.get().get_pseudo_outs().empty()))
            prunable.emplace(source.p.get().get_pseudo_outs(), source.p.get().p);

        rct_sig_map(dest, source, fee, prunable);
    }
}
