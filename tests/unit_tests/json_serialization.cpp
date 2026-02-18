
#include <boost/optional/optional.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <gtest/gtest.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <vector>

#include "byte_stream.h"
#include "crypto/hash.h"
#include "cryptonote_basic/account.h"
#include "cryptonote_basic/cryptonote_basic.h"
#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_core/cryptonote_tx_utils.h"
#include "file_io_utils.h"
#include "rpc/daemon_messages.h"
#include "serialization/json_object.h"
#include "string_tools.h"
#include "unit_tests_utils.h"

#undef MONERO_DEFAULT_LOG_CATEGORY
#define MONERO_DEFAULT_LOG_CATEGORY "cn"

namespace test
{
    cryptonote::transaction
    make_miner_transaction(cryptonote::account_public_address const& to)
    {
        cryptonote::transaction tx{};
        if (!cryptonote::construct_miner_tx(0, 0, 5000, 500, 500, to, tx))
            throw std::runtime_error{"transaction construction error"};

        crypto::hash id{0};
        if (!cryptonote::get_transaction_hash(tx, id))
            throw std::runtime_error{"could not get transaction hash"};

        return tx;
    }

    cryptonote::transaction
    make_transaction(
        cryptonote::account_keys const& from,
        std::vector<cryptonote::transaction> const& sources,
        std::vector<cryptonote::account_public_address> const& destinations,
        bool rct,
        bool bulletproof)
    {
        std::uint64_t source_amount = 0;
        std::vector<cryptonote::tx_source_entry> actual_sources;
        for (auto const& source : sources)
        {
            std::vector<cryptonote::tx_extra_field> extra_fields;
            if (!cryptonote::parse_tx_extra(source.extra, extra_fields))
                throw std::runtime_error{"invalid transaction"};

            cryptonote::tx_extra_pub_key key_field{};
            if (!cryptonote::find_tx_extra_field_by_type(extra_fields, key_field))
                throw std::runtime_error{"invalid transaction"};

            for (auto const input : boost::adaptors::index(source.vout))
            {
                source_amount += input.value().amount;
                auto const& key = boost::get<cryptonote::txout_to_key>(input.value().target);

                actual_sources.push_back(
                    {{}, 0, key_field.pub_key, {}, std::size_t(input.index()), input.value().amount, rct, rct::identity()}
                );

                for (unsigned ring = 0; ring < 10; ++ring)
                    actual_sources.back().push_output(input.index(), key.key, input.value().amount);
            }
        }

        std::vector<cryptonote::tx_destination_entry> to;
        for (auto const& destination : destinations)
            to.push_back({(source_amount / destinations.size()), destination, false});

        cryptonote::transaction tx{};

        crypto::secret_key tx_key{};
        std::vector<crypto::secret_key> extra_keys{};

        std::unordered_map<crypto::public_key, cryptonote::subaddress_index> subaddresses;
        subaddresses[from.m_account_address.m_spend_public_key] = {0,0};

        if (!cryptonote::construct_tx_and_get_tx_key(from, subaddresses, actual_sources, to, boost::none, {}, tx, tx_key, extra_keys, rct, { bulletproof ? rct::RangeProofPaddedBulletproof : rct::RangeProofBorromean, bulletproof ? 2 : 0 }))
            throw std::runtime_error{"transaction construction error"};

        return tx;
    }
}

namespace
{
    template<typename T>
    T test_json(const T& value)
    {
      epee::byte_stream buffer;
      {
        rapidjson::Writer<epee::byte_stream> dest{buffer};
        cryptonote::json::toJsonValue(dest, value);
      }

      rapidjson::Document doc;
      doc.Parse(reinterpret_cast<const char*>(buffer.data()), buffer.size());
      if (doc.HasParseError())
      {
        throw cryptonote::json::PARSE_FAIL();
      }

      T out{};
      cryptonote::json::fromJsonValue(doc, out);
      return out;
    }

    bool compare_tx_to_json_tx(const cryptonote::transaction &tx, const std::string &tx_json)
    {
        using namespace epee::string_tools; // for pod_to_hex()

        const crypto::hash txid = get_transaction_hash(tx);
        MINFO("Asserting validity of JSON representation of tx " << txid);

        rapidjson::Document tx_document;
        tx_document.Parse(tx_json.data());
        CHECK_AND_ASSERT_MES(!tx_document.HasParseError(), false, "compare_tx_to_json_tx: failed to parse json");

        // check existence of prefix members
        CHECK_AND_ASSERT_MES(tx_document.HasMember("version") && tx_document.HasMember("unlock_time") &&
            tx_document.HasMember("vin") && tx_document.HasMember("vout") && tx_document.HasMember("extra"),
            false, "compare_tx_to_json_tx: missing prefix members");

        // version
        CHECK_AND_ASSERT_MES(tx_document["version"].IsInt(), false, "compare_tx_to_json_tx: version isnt integer");
        CHECK_AND_ASSERT_MES(tx_document["version"].GetInt() == tx.version, false, "compare_tx_to_json_tx: wrong version");

        // unlock_time
        CHECK_AND_ASSERT_MES(tx_document["unlock_time"].IsUint64(), false, "compare_tx_to_json_tx: wrong unlock time");
        CHECK_AND_ASSERT_MES(tx_document["unlock_time"].GetUint64() == tx.unlock_time, false, "compare_tx_to_json_tx: wrong unlock time");

        // extra
        CHECK_AND_ASSERT_MES(tx_document["extra"].IsArray(), false, "compare_tx_to_json_tx: extra is not string");
        CHECK_AND_ASSERT_MES(tx_document["extra"].Size() == tx.extra.size(), false, "compare_tx_to_json_tx: wrong extra size");
        for (size_t i = 0; i < tx.extra.size(); ++i)
        {
            CHECK_AND_ASSERT_MES(tx_document["extra"][i].IsInt(), false, "compare_tx_to_json_tx: extra array contains non-int");
            CHECK_AND_ASSERT_MES(tx_document["extra"][i].GetInt() == tx.extra[i], false, "compare_tx_to_json_tx: extra array contains wrong int");
        }

        // vin
        const size_t num_inputs = tx.vin.size();
        size_t ring_size = 0;
        CHECK_AND_ASSERT_MES(tx_document["vin"].IsArray(), false, "compare_tx_to_json_tx: vin is not array");
        CHECK_AND_ASSERT_MES(tx_document["vin"].Size() == tx.vin.size(), false, "compare_tx_to_json_tx: vin wrong size");
        for (size_t i = 0; i < tx_document["vin"].Size(); i++)
        {
            CHECK_AND_ASSERT_MES(tx_document["vin"][i].IsObject(), false, "compare_tx_to_json_tx: txin not object");
            if (tx.vin[i].type() == typeid(cryptonote::txin_to_key))
            {
                const cryptonote::txin_to_key &k = boost::get<cryptonote::txin_to_key>(tx.vin[i]);
                ring_size = k.key_offsets.size();
                CHECK_AND_ASSERT_MES(tx_document["vin"][i].HasMember("key"), false, "compare_tx_to_json_tx: txin wrong type");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"].IsObject(), false, "compare_tx_to_json_tx: txin is not object");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"].HasMember("amount"), false, "compare_tx_to_json_tx: txin does not have amount");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["amount"].IsUint64(), false, "compare_tx_to_json_tx: txin amount is not int");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["amount"].GetUint64() == k.amount, false, "compare_tx_to_json_tx: txin wrong amount");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"].HasMember("k_image"), false, "compare_tx_to_json_tx: txin does not have k_image");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["k_image"].IsString(), false, "compare_tx_to_json_tx: txin kimage is not string");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["k_image"].GetString() == pod_to_hex(k.k_image), false, "compare_tx_to_json_tx: txin wrong k_image");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"].HasMember("key_offsets"), false, "compare_tx_to_json_tx: txin does not have key_offsets");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["key_offsets"].IsArray(), false, "compare_tx_to_json_tx: txin key_offsets is not array");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["key_offsets"].Size() == k.key_offsets.size(), false, "compare_tx_to_json_tx: txin wrong key_offsets size");
                for (size_t o = 0; o < k.key_offsets.size(); ++o)
                {
                    CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["key_offsets"][o].IsInt(), false, "compare_tx_to_json_tx: txin key offset is not int");
                    CHECK_AND_ASSERT_MES(tx_document["vin"][i]["key"]["key_offsets"][o].GetInt() == k.key_offsets[o], false, "compare_tx_to_json_tx: wrong key_offsets entry in txin");
                }
            }
            else if (tx.vin[i].type() == typeid(cryptonote::txin_gen))
            {
                const cryptonote::txin_gen &g = boost::get<cryptonote::txin_gen>(tx.vin[i]);
                CHECK_AND_ASSERT_MES(tx_document["vin"][i].HasMember("gen"), false, "compare_tx_to_json_tx: txin wrong type");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["gen"].IsObject(), false, "compare_tx_to_json_tx: txin is not object");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["gen"].HasMember("height"), false, "compare_tx_to_json_tx: txin does not have height");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["gen"]["height"].IsInt(), false, "compare_tx_to_json_tx: txin height is not int");
                CHECK_AND_ASSERT_MES(tx_document["vin"][i]["gen"]["height"].GetInt() == g.height, false, "compare_tx_to_json_tx: txin wrong amount");
            }
            else
            {
                CHECK_AND_ASSERT_MES(false, false, "compare_tx_to_json_tx: can't check non to_key and gen inputs");
            }
        }

        // vout
        CHECK_AND_ASSERT_MES(tx_document["vout"].IsArray(), false, "compare_tx_to_json_tx: vout is not array");
        CHECK_AND_ASSERT_MES(tx_document["vout"].Size() == tx.vout.size(), false, "compare_tx_to_json_tx: vout wrong size");
        for (size_t i = 0; i < tx_document["vout"].Size(); i++)
        {
            CHECK_AND_ASSERT_MES(tx_document["vout"][i].IsObject(), false, "compare_tx_to_json_tx: txout not object");
            CHECK_AND_ASSERT_MES(tx_document["vout"][i].HasMember("amount"), false, "compare_tx_to_json_tx: txout does not have amount");
            CHECK_AND_ASSERT_MES(tx_document["vout"][i]["amount"].IsUint64(), false, "compare_tx_to_json_tx: txout amount is not int");
            CHECK_AND_ASSERT_MES(tx_document["vout"][i]["amount"].GetUint64() == tx.vout[i].amount, false, "compare_tx_to_json_tx: txout wrong amount");
            CHECK_AND_ASSERT_MES(tx_document["vout"][i].HasMember("target"), false, "compare_tx_to_json_tx: txout has no target");
            CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"].IsObject(), false, "compare_tx_to_json_tx: txout target is not an object");
            if (tx.vout[i].target.type() == typeid(cryptonote::txout_to_key))
            {
                const cryptonote::txout_to_key &k = boost::get<cryptonote::txout_to_key>(tx.vout[i].target);
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"].HasMember("key"), false, "compare_tx_to_json_tx: txout target wrong type");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["key"].IsString(), false, "compare_tx_to_json_tx: txout target onetime-address not string");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["key"].GetString() == pod_to_hex(k.key), false, "compare_tx_to_json_tx: txout target wrong onetime-address");
            }
            else if (tx.vout[i].target.type() == typeid(cryptonote::txout_to_tagged_key))
            {
                const cryptonote::txout_to_tagged_key &k = boost::get<cryptonote::txout_to_tagged_key>(tx.vout[i].target);
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"].HasMember("tagged_key"), false, "compare_tx_to_json_tx: txout target wrong type");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"].IsObject(), false, "compare_tx_to_json_tx: txout target tagged key not object");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"].HasMember("key"), false, "compare_tx_to_json_tx: txout target tagged key does not have onetime-address");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"]["key"].IsString(), false, "compare_tx_to_json_tx: txout target tagged key onetime-address not string");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"]["key"].GetString() == pod_to_hex(k.key), false, "compare_tx_to_json_tx: txout target tagged key wrong onetime-address");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"].HasMember("view_tag"), false, "compare_tx_to_json_tx: txout target tagged key does not have view tag");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"]["view_tag"].IsString(), false, "compare_tx_to_json_tx: txout target tagged key view_tag not string");
                CHECK_AND_ASSERT_MES(tx_document["vout"][i]["target"]["tagged_key"]["view_tag"].GetString() == pod_to_hex(k.view_tag), false, "compare_tx_to_json_tx: txout target tagged key wrong view_tag");
            }
            else
            {
                CHECK_AND_ASSERT_MES(false, false, "compare_tx_to_json_tx: can't check non to_key and to_tagged_key outputs");
            }
        }

        if (tx.version == 1)
        {
            // signatures
            CHECK_AND_ASSERT_MES(tx_document.HasMember("signatures"), false, "compare_tx_to_json_tx: missing signatures");
            CHECK_AND_ASSERT_MES(tx_document["signatures"].IsArray(), false, "compare_tx_to_json_tx: signatures is not array");

            if (!cryptonote::is_coinbase(tx))
            {
                CHECK_AND_ASSERT_MES(tx_document["signatures"].Size() == tx.signatures.size(), false, "compare_tx_to_json_tx: signatures wrong size");
                for (size_t i = 0; i < tx.signatures.size(); ++i)
                {
                    CHECK_AND_ASSERT_MES(tx_document["signatures"][i].IsString(), false, "compare_tx_to_json_tx: signatures element not string");
                    const std::string sig_hexbuffer{epee::to_hex::string({reinterpret_cast<const unsigned char*>(tx.signatures[i].data()), tx.signatures[i].size() * sizeof(crypto::signature)})};
                    CHECK_AND_ASSERT_MES(tx_document["signatures"][i].GetString() == sig_hexbuffer, false, "compare_tx_to_json_tx: signatures element wrong");
                }
            }
            else // coinbase
            {
                CHECK_AND_ASSERT_MES(tx_document["signatures"].Size() == 0, false, "compare_tx_to_json_tx: coinbase signatures not size 0");
            }
        }
        else // v2 tx
        {
            // rct_signatures
            CHECK_AND_ASSERT_MES(tx_document.HasMember("rct_signatures"), false, "compare_tx_to_json_tx: missing rct_signatures");
            const rapidjson::Value &jrv = tx_document["rct_signatures"];
            CHECK_AND_ASSERT_MES(jrv.IsObject(), false, "compare_tx_to_json_tx: rct_signatures is not object");

            // type
            CHECK_AND_ASSERT_MES(jrv.HasMember("type"), false, "compare_tx_to_json_tx: rct_signatures has no type");
            CHECK_AND_ASSERT_MES(jrv["type"].IsUint(), false, "compare_tx_to_json_tx: rct_signatures type is not int");
            CHECK_AND_ASSERT_MES(jrv["type"].GetUint() == tx.rct_signatures.type, false, "compare_tx_to_json_tx: rct_signatures wrong type");

            if (!cryptonote::is_coinbase(tx))
            {
                // txnFee
                CHECK_AND_ASSERT_MES(jrv.HasMember("txnFee"), false, "compare_tx_to_json_tx: rct_signatures has no fee");
                CHECK_AND_ASSERT_MES(jrv["txnFee"].IsUint64(), false, "compare_tx_to_json_tx: rct_signatures type is not uint64");
                CHECK_AND_ASSERT_MES(jrv["txnFee"].GetUint64() == tx.rct_signatures.txnFee, false, "compare_tx_to_json_tx: rct_signatures wrong fee");

                // outPk
                CHECK_AND_ASSERT_MES(jrv.HasMember("outPk"), false, "compare_tx_to_json_tx: rct_signatures has no outPk");
                CHECK_AND_ASSERT_MES(jrv["outPk"].IsArray(), false, "compare_tx_to_json_tx: rct_signatures outPk is not array");
                CHECK_AND_ASSERT_MES(jrv["outPk"].Size() == tx.rct_signatures.outPk.size(), false, "compare_tx_to_json_tx: rct_signatures wrong outPk size");
                for (size_t i = 0; i < tx.rct_signatures.outPk.size(); ++i)
                {
                    CHECK_AND_ASSERT_MES(jrv["outPk"][i].IsString(), false, "compare_tx_to_json_tx: rct_signatures outPk entry not string");
                    CHECK_AND_ASSERT_MES(jrv["outPk"][i].GetString() == pod_to_hex(tx.rct_signatures.outPk[i].mask), false, "compare_tx_to_json_tx: rct_signatures outPk wrong mask");
                }

                // ecdhInfo
                const bool short_ecdh_info = tx.rct_signatures.type >= rct::RCTTypeBulletproof2;
                const size_t ecdh_amount_hexlength = 2 * (short_ecdh_info ? 8 : 32);
                CHECK_AND_ASSERT_MES(jrv.HasMember("ecdhInfo"), false, "compare_tx_to_json_tx: rct_signatures has no ecdhInfo");
                CHECK_AND_ASSERT_MES(jrv["ecdhInfo"].IsArray(), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo is not array");
                CHECK_AND_ASSERT_MES(jrv["ecdhInfo"].Size() == tx.rct_signatures.ecdhInfo.size(), false, "compare_tx_to_json_tx: rct_signatures wrong ecdhInfo size");
                for (size_t i = 0; i < tx.rct_signatures.ecdhInfo.size(); ++i)
                {
                    CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i].IsObject(), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry not object");
                    // amount (encrypted)
                    CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i].HasMember("amount"), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry missing amount");
                    CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i]["amount"].IsString(), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry amount info not string");
                    CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i]["amount"].GetStringLength() == ecdh_amount_hexlength, false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry amount string wrong size");
                    const auto hex_amount{pod_to_hex(tx.rct_signatures.ecdhInfo[i].amount).substr(0, ecdh_amount_hexlength)};
                    CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i]["amount"].GetString() == hex_amount, false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry wrong amount");
                    if (!short_ecdh_info)
                    {
                        // mask (encrypted blinding factor)
                        CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i].HasMember("mask"), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry missing mask");
                        CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i]["mask"].IsString(), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry mask not string");
                        CHECK_AND_ASSERT_MES(jrv["ecdhInfo"][i]["mask"].GetString() == pod_to_hex(tx.rct_signatures.ecdhInfo[i].mask), false, "compare_tx_to_json_tx: rct_signatures ecdhInfo entry wrong mask");
                    }
                }

                // pseudoOuts (not pruned w/ v2 RingCT)
                if (tx.rct_signatures.type == rct::RCTTypeSimple)
                {
                    CHECK_AND_ASSERT_MES(tx.rct_signatures.pseudoOuts.size() == tx.vin.size(), false, "sanity check pseudoOuts size");
                    CHECK_AND_ASSERT_MES(jrv.HasMember("pseudoOuts"), false, "compare_tx_to_json_tx: rct_signatures missing pseudoOuts");
                    CHECK_AND_ASSERT_MES(jrv["pseudoOuts"].IsArray(), false, "compare_tx_to_json_tx: rct_signatures pseudoOuts is not array");
                    CHECK_AND_ASSERT_MES(jrv["pseudoOuts"].Size() == tx.rct_signatures.pseudoOuts.size(), false, "compare_tx_to_json_tx: rct_signatures pseudoOuts wrong size");
                    for (size_t i = 0; i < tx.rct_signatures.pseudoOuts.size(); ++i)
                    {
                        CHECK_AND_ASSERT_MES(jrv["pseudoOuts"][i].IsString(), false, "compare_tx_to_json_tx: rct_signatures pseudoOuts entry not string");
                        CHECK_AND_ASSERT_MES(jrv["pseudoOuts"][i].GetString() == pod_to_hex(tx.rct_signatures.pseudoOuts[i]), false, "compare_tx_to_json_tx: rct_signatures pseudoOuts wrong entry");
                    }
                }

                // rctsig_prunable
                CHECK_AND_ASSERT_MES(tx_document.HasMember("rctsig_prunable"), false, "compare_tx_to_json_tx: rct_signatures missing rctsig_prunable");
                const rapidjson::Value &jrvp = tx_document["rctsig_prunable"];
                const rct::rctSigPrunable &rvp = tx.rct_signatures.p;

                // range proofs
                switch (tx.rct_signatures.type)
                {
                case rct::RCTTypeFull:
                case rct::RCTTypeSimple:
                    // boro sig
                    CHECK_AND_ASSERT_MES(rvp.rangeSigs.size() == tx.vout.size(), false, "sanity check rangeSigs length");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("rangeSigs"), false, "compare_tx_to_json_tx: rctsig_prunable missing rangeSigs");
                    CHECK_AND_ASSERT_MES(jrvp["rangeSigs"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable rangeSigs not array");
                    CHECK_AND_ASSERT_MES(jrvp["rangeSigs"].Size() == rvp.rangeSigs.size(), false, "compare_tx_to_json_tx: rctsig_prunable rangeSigs wrong size");
                    for (size_t i = 0; i < rvp.rangeSigs.size(); ++i)
                    {
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i].IsObject(), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig not object");
                        // asig
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i].HasMember("asig"), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig missing asig");
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i]["asig"].IsString(), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig asig is not string");
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i]["asig"].GetString() == pod_to_hex(rvp.rangeSigs[i].asig), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig wrong asig");
                        // Ci
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i].HasMember("Ci"), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig missing Ci");
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i]["Ci"].IsString(), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig Ci is not string");
                        CHECK_AND_ASSERT_MES(jrvp["rangeSigs"][i]["Ci"].GetString() == pod_to_hex(rvp.rangeSigs[i].Ci), false, "compare_tx_to_json_tx: rctsig_prunable rangeSig wrong Ci");
                    }
                    break;
                case rct::RCTTypeBulletproof:
                case rct::RCTTypeBulletproof2:
                case rct::RCTTypeCLSAG:
                    // bulletproof sig
                    CHECK_AND_ASSERT_MES(rvp.bulletproofs.size(), false, "sanity check bulletproofs length");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("nbp"), false, "compare_tx_to_json_tx: rctsig_prunable missing nbp");
                    CHECK_AND_ASSERT_MES(jrvp["nbp"].IsUint(), false, "compare_tx_to_json_tx: rctsig_prunable nbp not unsigned int");
                    CHECK_AND_ASSERT_MES(jrvp["nbp"].GetUint() == rvp.bulletproofs.size(), false, "compare_tx_to_json_tx: rctsig_prunable nbp wrong value");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("bp"), false, "compare_tx_to_json_tx: rctsig_prunable missing bp");
                    CHECK_AND_ASSERT_MES(jrvp["bp"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable bp not array");
                    CHECK_AND_ASSERT_MES(jrvp["bp"].Size() == jrvp["nbp"].GetUint(), false, "compare_tx_to_json_tx: rctsig_prunable bp size not equal to nbp");
                    for (size_t i = 0; i < jrvp["nbp"].GetUint(); ++i)
                    {
                        /* A, S, T1, T2, taux, mu, L[], R[], a, b, t */
                        const rapidjson::Value &jbp = jrvp["bp"][i];
                        const rct::Bulletproof &bp = rvp.bulletproofs[i];
                        CHECK_AND_ASSERT_MES(jbp.IsObject(), false, "compare_tx_to_json_tx: bp entry not object");
                        CHECK_AND_ASSERT_MES(jbp.HasMember("A") && jbp.HasMember("S") && jbp.HasMember("T1") &&
                            jbp.HasMember("T2") && jbp.HasMember("taux") && jbp.HasMember("mu") &&
                            jbp.HasMember("L") && jbp.HasMember("R") && jbp.HasMember("a") &&
                            jbp.HasMember("b") && jbp.HasMember("t"), false, "compare_tx_to_json_tx: bp entry missing fields");
                        CHECK_AND_ASSERT_MES(jbp["A"].IsString() && jbp["S"].IsString() &&
                            jbp["T1"].IsString() && jbp["T2"].IsString() && jbp["taux"].IsString() &&
                            jbp["mu"].IsString() && jbp["L"].IsArray() && jbp["R"].IsArray() &&
                            jbp["a"].IsString() && jbp["b"].IsString() && jbp["t"].IsString(), false,
                            "compare_tx_to_json_tx: bp entry fields bad type");
                        CHECK_AND_ASSERT_MES(jbp["A"].GetString() == pod_to_hex(bp.A), false, "compare_tx_to_json_tx: bp entry wrong A");
                        CHECK_AND_ASSERT_MES(jbp["S"].GetString() == pod_to_hex(bp.S), false, "compare_tx_to_json_tx: bp entry wrong S");
                        CHECK_AND_ASSERT_MES(jbp["T1"].GetString() == pod_to_hex(bp.T1), false, "compare_tx_to_json_tx: bp entry wrong T1");
                        CHECK_AND_ASSERT_MES(jbp["T2"].GetString() == pod_to_hex(bp.T2), false, "compare_tx_to_json_tx: bp entry wrong T2");
                        CHECK_AND_ASSERT_MES(jbp["taux"].GetString() == pod_to_hex(bp.taux), false, "compare_tx_to_json_tx: bp entry wrong taux");
                        CHECK_AND_ASSERT_MES(jbp["mu"].GetString() == pod_to_hex(bp.mu), false, "compare_tx_to_json_tx: bp entry wrong mu");
                        CHECK_AND_ASSERT_MES(jbp["a"].GetString() == pod_to_hex(bp.a), false, "compare_tx_to_json_tx: bp entry wrong a");
                        CHECK_AND_ASSERT_MES(jbp["b"].GetString() == pod_to_hex(bp.b), false, "compare_tx_to_json_tx: bp entry wrong b");
                        CHECK_AND_ASSERT_MES(jbp["t"].GetString() == pod_to_hex(bp.t), false, "compare_tx_to_json_tx: bp entry wrong t");
                        CHECK_AND_ASSERT_MES(jbp["L"].Size() == bp.L.size(), false, "compare_tx_to_json_tx: bp entry L wrong size");
                        for (size_t j = 0; j < bp.L.size(); ++j)
                        {
                            CHECK_AND_ASSERT_MES(jbp["L"][j].IsString(), false, "compare_tx_to_json_tx: bp entry L entry not string");
                            CHECK_AND_ASSERT_MES(jbp["L"][j].GetString() == pod_to_hex(bp.L[j]), false, "compare_tx_to_json_tx: bp entry L entry wrong value");
                        }
                        CHECK_AND_ASSERT_MES(jbp["R"].Size() == bp.R.size(), false, "compare_tx_to_json_tx: bp entry R wrong size");
                        for (size_t j = 0; j < bp.R.size(); ++j)
                        {
                            CHECK_AND_ASSERT_MES(jbp["R"][j].IsString(), false, "compare_tx_to_json_tx: bp entry R entry not string");
                            CHECK_AND_ASSERT_MES(jbp["R"][j].GetString() == pod_to_hex(bp.R[j]), false, "compare_tx_to_json_tx: bp entry R entry wrong value");
                        }
                    }
                    break;
                case rct::RCTTypeBulletproofPlus:
                    // bulletproof+ sig
                    CHECK_AND_ASSERT_MES(rvp.bulletproofs_plus.size(), false, "sanity check bulletproofs+ length");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("nbp"), false, "compare_tx_to_json_tx: rctsig_prunable missing nbp");
                    CHECK_AND_ASSERT_MES(jrvp["nbp"].IsUint(), false, "compare_tx_to_json_tx: rctsig_prunable nbp not unsigned int");
                    CHECK_AND_ASSERT_MES(jrvp["nbp"].GetUint() == rvp.bulletproofs_plus.size(), false, "compare_tx_to_json_tx: rctsig_prunable nbp wrong value");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("bpp"), false, "compare_tx_to_json_tx: rctsig_prunable missing bp");
                    CHECK_AND_ASSERT_MES(jrvp["bpp"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable bp not array");
                    CHECK_AND_ASSERT_MES(jrvp["bpp"].Size() == jrvp["nbp"].GetUint(), false, "compare_tx_to_json_tx: rctsig_prunable bp size not equal to nbp");
                    for (size_t i = 0; i < jrvp["nbp"].GetUint(); ++i)
                    {
                        /* A, A1, B, r1, s1, d1, L[], R[] */
                        const rapidjson::Value &jbpp = jrvp["bpp"][i];
                        const rct::BulletproofPlus &bpp = rvp.bulletproofs_plus[i];
                        CHECK_AND_ASSERT_MES(jbpp.IsObject(), false, "compare_tx_to_json_tx: bpp entry not object");
                        CHECK_AND_ASSERT_MES(jbpp.HasMember("A") && jbpp.HasMember("A1") && jbpp.HasMember("B") &&
                            jbpp.HasMember("r1") && jbpp.HasMember("s1") && jbpp.HasMember("d1") &&
                            jbpp.HasMember("L") && jbpp.HasMember("R"), false, "compare_tx_to_json_tx: bpp entry missing fields");
                        CHECK_AND_ASSERT_MES(jbpp["A"].IsString() && jbpp["A1"].IsString() &&
                            jbpp["B"].IsString() && jbpp["r1"].IsString() && jbpp["s1"].IsString() &&
                            jbpp["d1"].IsString() && jbpp["L"].IsArray() && jbpp["R"].IsArray(), false,
                            "compare_tx_to_json_tx: bpp entry fields bad type");
                        CHECK_AND_ASSERT_MES(jbpp["A"].GetString() == pod_to_hex(bpp.A), false, "compare_tx_to_json_tx: bpp entry wrong A");
                        CHECK_AND_ASSERT_MES(jbpp["A1"].GetString() == pod_to_hex(bpp.A1), false, "compare_tx_to_json_tx: bpp entry wrong S");
                        CHECK_AND_ASSERT_MES(jbpp["B"].GetString() == pod_to_hex(bpp.B), false, "compare_tx_to_json_tx: bpp entry wrong T1");
                        CHECK_AND_ASSERT_MES(jbpp["r1"].GetString() == pod_to_hex(bpp.r1), false, "compare_tx_to_json_tx: bpp entry wrong T2");
                        CHECK_AND_ASSERT_MES(jbpp["s1"].GetString() == pod_to_hex(bpp.s1), false, "compare_tx_to_json_tx: bpp entry wrong taux");
                        CHECK_AND_ASSERT_MES(jbpp["d1"].GetString() == pod_to_hex(bpp.d1), false, "compare_tx_to_json_tx: bpp entry wrong mu");
                        CHECK_AND_ASSERT_MES(jbpp["L"].Size() == bpp.L.size(), false, "compare_tx_to_json_tx: bpp entry L wrong size");
                        for (size_t j = 0; j < bpp.L.size(); ++j)
                        {
                            CHECK_AND_ASSERT_MES(jbpp["L"][j].IsString(), false, "compare_tx_to_json_tx: bp entry L entry not string");
                            CHECK_AND_ASSERT_MES(jbpp["L"][j].GetString() == pod_to_hex(bpp.L[j]), false, "compare_tx_to_json_tx: bpp entry L entry wrong value");
                        }
                        CHECK_AND_ASSERT_MES(jbpp["R"].Size() == bpp.R.size(), false, "compare_tx_to_json_tx: bpp entry R wrong size");
                        for (size_t j = 0; j < bpp.R.size(); ++j)
                        {
                            CHECK_AND_ASSERT_MES(jbpp["R"][j].IsString(), false, "compare_tx_to_json_tx: bpp entry R entry not string");
                            CHECK_AND_ASSERT_MES(jbpp["R"][j].GetString() == pod_to_hex(bpp.R[j]), false, "compare_tx_to_json_tx: bpp entry R entry wrong value");
                        }
                    }
                    break;
                default:
                    MERROR("Unrecognized RCT Type");
                    return false;
                }

                // ring signatures
                const bool is_full_rct = tx.rct_signatures.type == rct::RCTTypeFull;
                const size_t num_mgs = is_full_rct ? 1 : num_inputs;
                const size_t num_mg_cols = 1 + (is_full_rct ? num_inputs : 1);
                switch (tx.rct_signatures.type)
                {
                case rct::RCTTypeFull:
                case rct::RCTTypeSimple:
                case rct::RCTTypeBulletproof:
                case rct::RCTTypeBulletproof2:
                    // MLSAG sig
                    CHECK_AND_ASSERT_MES(rvp.MGs.size() == num_mgs, false, "sanity check tx has correct num MGs");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("MGs"), false, "compare_tx_to_json_tx: rctsig_prunable missing MGs");
                    CHECK_AND_ASSERT_MES(jrvp["MGs"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable MGs not array");
                    CHECK_AND_ASSERT_MES(jrvp["MGs"].Size() == num_mgs, false, "compare_tx_to_json_tx: rctsig_prunable MGs wrong size");
                    for (size_t i = 0; i < num_mgs; ++i)
                    {
                        const rapidjson::Value &jmg = jrvp["MGs"][i];
                        const rct::mgSig &mg = rvp.MGs[i];
                        CHECK_AND_ASSERT_MES(jmg.IsObject(), false, "compare_tx_to_json_tx: rctsig_prunable MGs entry not object");
                        CHECK_AND_ASSERT_MES(jmg.HasMember("ss") && jmg.HasMember("cc"), false, "compare_tx_to_json_tx: rctsig_prunable MGs entry missing fields");
                        CHECK_AND_ASSERT_MES(jmg["cc"].IsString(), false, "compare_tx_to_json_tx: rctsig_prunable MGs cc not string");
                        CHECK_AND_ASSERT_MES(jmg["cc"].GetString() == pod_to_hex(mg.cc), false, "compare_tx_to_json_tx: rctsig_prunable MGs cc wrong value");
                        CHECK_AND_ASSERT_MES(jmg["ss"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable MG ss not array");
                        CHECK_AND_ASSERT_MES(jmg["ss"].Size() == ring_size, false, "compare_tx_to_json_tx: rctsig_prunable MG ss num rows not ring size");
                        for (size_t j = 0; j < ring_size; ++j)
                        {
                            const rapidjson::Value &jrow = jmg["ss"][j];
                            const rct::keyV &row = mg.ss[j];
                            CHECK_AND_ASSERT_MES(jrow.IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable MG ss row not array");
                            CHECK_AND_ASSERT_MES(row.size() == num_mg_cols, false, "sanity check MG ss row size");
                            CHECK_AND_ASSERT_MES(jrow.Size() == num_mg_cols, false, "compare_tx_to_json_tx: rctsig_prunable MG ss row wrong size");
                            for (size_t k = 0; k < num_mg_cols; ++k)
                            {
                                CHECK_AND_ASSERT_MES(jrow[k].IsString(), false, "compare_tx_to_json_tx: rctsig_prunable MG ss scalar not string");
                                CHECK_AND_ASSERT_MES(jrow[k].GetString() == pod_to_hex(row[k]), false, "compare_tx_to_json_tx: rctsig_prunable MG ss wrong scalar");
                            }
                        }
                    }
                    break;
                case rct::RCTTypeCLSAG:
                case rct::RCTTypeBulletproofPlus:
                    // CLSAG sig
                    CHECK_AND_ASSERT_MES(rvp.CLSAGs.size() == num_inputs, false, "sanity check tx has correct num CLSAGs");
                    CHECK_AND_ASSERT_MES(jrvp.HasMember("CLSAGs"), false, "compare_tx_to_json_tx: rctsig_prunable missing CLSAGs");
                    CHECK_AND_ASSERT_MES(jrvp["CLSAGs"].IsArray(), false, "compare_tx_to_json_tx: rctsig_prunable CLSAGs not array");
                    CHECK_AND_ASSERT_MES(jrvp["CLSAGs"].Size() == num_inputs, false, "compare_tx_to_json_tx: rctsig_prunable CLSAGs wrong size");
                    for (size_t i = 0; i < num_inputs; ++i)
                    {
                        const rapidjson::Value &jclsag = jrvp["CLSAGs"][i];
                        const rct::clsag &clsag = rvp.CLSAGs[i];
                        CHECK_AND_ASSERT_MES(jclsag.IsObject(), false, "compare_tx_to_json_tx: rctsig_prunable CLSAG is not object");
                        CHECK_AND_ASSERT_MES(jclsag.HasMember("s") && jclsag.HasMember("c1") && jclsag.HasMember("D"),
                            false, "compare_tx_to_json_tx: rctsig_prunable CLSAG missing fields");
                        CHECK_AND_ASSERT_MES(jclsag["s"].IsArray() && jclsag["c1"].IsString() && jclsag["D"].IsString(),
                            false, "compare_tx_to_json_tx: rctsig_prunable CLSAG fields wrong type");
                        CHECK_AND_ASSERT_MES(jclsag["c1"].GetString() == pod_to_hex(clsag.c1),
                            false, "compare_tx_to_json_tx: rctsig_prunable CLSAG wrong c1");
                        CHECK_AND_ASSERT_MES(jclsag["D"].GetString() == pod_to_hex(clsag.D),
                            false, "compare_tx_to_json_tx: rctsig_prunable CLSAG wrong D");
                        CHECK_AND_ASSERT_MES(jclsag["s"].Size() == ring_size,
                            false, "compare_tx_to_json_tx: rctsig_prunable CLSAG.s wrong size");
                        CHECK_AND_ASSERT_MES(clsag.s.size() == ring_size, false, "sanity check CLSAG.s size");
                        for (size_t j = 0; j < ring_size; ++j)
                        {
                            CHECK_AND_ASSERT_MES(jclsag["s"][j].IsString(), false, "compare_tx_to_json_tx: rctsig_prunable CLSAG c1 not string");
                            CHECK_AND_ASSERT_MES(jclsag["s"][j].GetString() == pod_to_hex(clsag.s[j]),
                                false, "compare_tx_to_json_tx: rctsig_prunable CLSAG.s wrong key element");
                        }
                    }
                    break;
                default:
                    MERROR("Unrecognized RCT Type");
                    return false;
                }
            }
        }

        return true;
    }

    bool jsonify_tx_from_file_and_parse_compare(const std::string &tx_fname)
    {
        const boost::filesystem::path tx_path = unit_test::data_dir / "txs" / tx_fname;

        std::string tx_blob;
        CHECK_AND_ASSERT_THROW_MES(epee::file_io_utils::load_file_to_string(tx_path.string(), tx_blob),
            "failed to load tx from file: " << tx_path.string());

        cryptonote::transaction tx;
        CHECK_AND_ASSERT_THROW_MES(cryptonote::parse_and_validate_tx_from_blob(tx_blob, tx),
            "failed to parse tx from file: " << tx_path.string());

        // Same JSONification used in /get_transactions RPC endpoint
        const std::string tx_json{cryptonote::obj_to_json_str(tx)};

        return compare_tx_to_json_tx(tx, tx_json);
    }
} // anonymous

TEST(JsonSerialization, VectorBytes)
{
    EXPECT_EQ(std::vector<std::uint8_t>{}, test_json(std::vector<std::uint8_t>{}));
    EXPECT_EQ(std::vector<std::uint8_t>{0x00}, test_json(std::vector<std::uint8_t>{0x00}));
}

TEST(JsonSerialization, InvalidVectorBytes)
{
    rapidjson::Document doc;
    doc.SetString("1");

    std::vector<std::uint8_t> out;
    EXPECT_THROW(cryptonote::json::fromJsonValue(doc, out), cryptonote::json::BAD_INPUT);
}

TEST(JsonSerialization, DaemonInfo)
{
  cryptonote::rpc::DaemonInfo info{};
  info.height = 154544;
  info.target_height = 15345435;
  info.top_block_height = 2344;
  info.wide_difficulty = cryptonote::difficulty_type{"100000000000000000005443"};
  info.difficulty = 200376420520695107;
  info.target = 7657567;
  info.tx_count = 355;
  info.tx_pool_size = 45435;
  info.alt_blocks_count = 43535;
  info.outgoing_connections_count = 1444;
  info.incoming_connections_count = 1444;
  info.white_peerlist_size = 14550;
  info.grey_peerlist_size = 34324;
  info.mainnet = true;
  info.testnet = true;
  info.stagenet = true;
  info.nettype = "main";
  info.top_block_hash = crypto::hash{1};
  info.wide_cumulative_difficulty = cryptonote::difficulty_type{"200000000000000000005543"};
  info.cumulative_difficulty = 400752841041384871;
  info.block_size_limit = 4324234;
  info.block_weight_limit = 3434;
  info.block_size_median = 3434;
  info.adjusted_time = 4535;
  info.block_weight_median = 43535;
  info.start_time = 34535;
  info.version = "1.0";

  const auto info_copy = test_json(info);

  EXPECT_EQ(info.height, info_copy.height);
  EXPECT_EQ(info.target_height, info_copy.target_height);
  EXPECT_EQ(info.top_block_height, info_copy.top_block_height);
  EXPECT_EQ(info.wide_difficulty, info_copy.wide_difficulty);
  EXPECT_EQ(info.difficulty, info_copy.difficulty);
  EXPECT_EQ(info.target, info_copy.target);
  EXPECT_EQ(info.tx_count, info_copy.tx_count);
  EXPECT_EQ(info.tx_pool_size, info_copy.tx_pool_size);
  EXPECT_EQ(info.alt_blocks_count, info_copy.alt_blocks_count);
  EXPECT_EQ(info.outgoing_connections_count, info_copy.outgoing_connections_count);
  EXPECT_EQ(info.incoming_connections_count, info_copy.incoming_connections_count);
  EXPECT_EQ(info.white_peerlist_size, info_copy.white_peerlist_size);
  EXPECT_EQ(info.grey_peerlist_size, info_copy.grey_peerlist_size);
  EXPECT_EQ(info.mainnet, info_copy.mainnet);
  EXPECT_EQ(info.testnet, info_copy.testnet);
  EXPECT_EQ(info.stagenet, info_copy.stagenet);
  EXPECT_EQ(info.nettype, info_copy.nettype);
  EXPECT_EQ(info.top_block_hash, info_copy.top_block_hash);
  EXPECT_EQ(info.wide_cumulative_difficulty, info_copy.wide_cumulative_difficulty);
  EXPECT_EQ(info.cumulative_difficulty, info_copy.cumulative_difficulty);
  EXPECT_EQ(info.block_size_limit, info_copy.block_size_limit);
  EXPECT_EQ(info.block_weight_limit, info_copy.block_weight_limit);
  EXPECT_EQ(info.block_size_median, info_copy.block_size_median);
  EXPECT_EQ(info.adjusted_time, info_copy.adjusted_time);
  EXPECT_EQ(info.block_weight_median, info_copy.block_weight_median);
  EXPECT_EQ(info.start_time, info_copy.start_time);
  EXPECT_EQ(info.version, info_copy.version);
}

TEST(JsonSerialization, MinerTransaction)
{
    cryptonote::account_base acct;
    acct.generate();
    const auto miner_tx = test::make_miner_transaction(acct.get_keys().m_account_address);

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(miner_tx, tx_hash));

    cryptonote::transaction miner_tx_copy = test_json(miner_tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(miner_tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(miner_tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(miner_tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, RegularTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, false, false
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, RingctTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, true, false
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonSerialization, BulletproofTransaction)
{
    cryptonote::account_base acct1;
    acct1.generate();

    cryptonote::account_base acct2;
    acct2.generate();

    const auto miner_tx = test::make_miner_transaction(acct1.get_keys().m_account_address);
    const auto tx = test::make_transaction(
        acct1.get_keys(), {miner_tx}, {acct2.get_keys().m_account_address}, true, true
    );

    crypto::hash tx_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx, tx_hash));

    cryptonote::transaction tx_copy = test_json(tx);

    crypto::hash tx_copy_hash{};
    ASSERT_TRUE(cryptonote::get_transaction_hash(tx_copy, tx_copy_hash));
    EXPECT_EQ(tx_hash, tx_copy_hash);

    cryptonote::blobdata tx_bytes{};
    cryptonote::blobdata tx_copy_bytes{};

    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx, tx_bytes));
    ASSERT_TRUE(cryptonote::t_serializable_object_to_blob(tx_copy, tx_copy_bytes));

    EXPECT_EQ(tx_bytes, tx_copy_bytes);
}

TEST(JsonRpcSerialization, HandlerFromJson)
{
  cryptonote::rpc::FullMessage req_full("{\"jsonrpc\":\"2.0\",\"method\":\"get_hashes_fast\",\"params\":[1]}", true);
  cryptonote::rpc::GetHashesFast::Request request{};
  EXPECT_THROW(request.fromJson(req_full.getMessage()), cryptonote::json::WRONG_TYPE);
}

TEST(JsonSerialization, jsonify_tx_from_file_and_parse_compare)
{
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("v1_coinbase_tx_bf4c0300.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("v1_tx_hf3_effcceb9.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("v2_coinbase_tx_7f88a52a.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("rct_full_tx_14056427.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("rct_simple_tx_c69861bf.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("rct_bp_tx_a685d68e.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("rct_bp_compact_tx_10312fd4.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("rct_clsag_tx_200c3215.bin"));
    EXPECT_TRUE(jsonify_tx_from_file_and_parse_compare("bpp_tx_e89415.bin"));
}
