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
//
// Parts of this file are originally copyright (c) 2012-2013 The Cryptonote developers

#include "pending_transaction.h"
#include "string_tools.h"
#include "wallet.h"
#include "common_defines.h"

#include "cryptonote_basic/cryptonote_format_utils.h"
#include "cryptonote_basic/cryptonote_basic_impl.h"
#include "common/base58.h"

#include <memory>
#include <vector>
#include <sstream>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>

using namespace std;

namespace Monero {

PendingTransaction::~PendingTransaction() {}


PendingTransactionImpl::PendingTransactionImpl(WalletImpl &wallet)
    : m_wallet(wallet)
{
  m_status = Status_Ok;
}

PendingTransactionImpl::~PendingTransactionImpl()
{

}

int PendingTransactionImpl::status() const
{
    return m_status;
}

string PendingTransactionImpl::errorString() const
{
    return m_errorString;
}

std::vector<std::string> PendingTransactionImpl::txid() const
{
    std::vector<std::string> txid;
    for (const auto &pt: m_pending_tx)
        txid.push_back(epee::string_tools::pod_to_hex(cryptonote::get_transaction_hash(pt.tx)));
    return txid;
}

bool PendingTransactionImpl::commit(const std::string &filename, bool overwrite)
{

    LOG_PRINT_L3("m_pending_tx size: " << m_pending_tx.size());

    bool warn_of_possible_attack = !m_wallet.trustedDaemon();
    try {
      // Save tx to file
      if (!filename.empty()) {
        boost::system::error_code ignore;
        bool tx_file_exists = boost::filesystem::exists(filename, ignore);
        if(tx_file_exists && !overwrite){
          m_errorString = string(tr("Attempting to save transaction to file, but specified file(s) exist. Exiting to not risk overwriting. File:")) + filename;
          m_status = Status_Error;
          LOG_ERROR(m_errorString);
          return false;
        }
        bool r = m_wallet.m_wallet->save_tx(m_pending_tx, filename);
        if (!r) {
          m_errorString = tr("Failed to write transaction(s) to file");
          m_status = Status_Error;
        } else {
          m_status = Status_Ok;
        }
      }
      // Commit tx
      else {
        auto multisigState = m_wallet.multisig();
        if (multisigState.isMultisig && m_signers.size() < multisigState.threshold) {
            throw runtime_error("Not enough signers to send multisig transaction");
        }

        m_wallet.pauseRefresh();

        const bool tx_cold_signed = m_wallet.m_wallet->get_account().get_device().has_tx_cold_sign();
        if (tx_cold_signed){
          std::unordered_set<size_t> selected_transfers;
          for(const tools::wallet2::pending_tx & ptx : m_pending_tx){
            for(size_t s : ptx.selected_transfers){
              selected_transfers.insert(s);
            }
          }

          m_wallet.m_wallet->cold_tx_aux_import(m_pending_tx, m_tx_device_aux);
          bool r = m_wallet.m_wallet->import_key_images(m_key_images, 0, selected_transfers);
          if (!r){
            throw runtime_error("Cold sign transaction submit failed - key image sync fail");
          }
        }

        while (!m_pending_tx.empty()) {
            auto & ptx = m_pending_tx.back();
            m_wallet.m_wallet->commit_tx(ptx);
            // if no exception, remove element from vector
            m_pending_tx.pop_back();
        } // TODO: extract method;
      }
    } catch (const tools::error::deprecated_rpc_access&) {
        m_errorString = tr("Daemon requires deprecated RPC payment. See https://github.com/monero-project/monero/issues/8722");
        m_status = Status_Error;
    } catch (const tools::error::no_connection_to_daemon&) {
        m_errorString = tr("no connection to daemon. Please make sure daemon is running.");
        m_status = Status_Error;
    } catch (const tools::error::daemon_busy&) {
        m_errorString = tr("daemon is busy. Please try again later.");
        m_status = Status_Error;
    } catch (const tools::error::wallet_rpc_error& e) {
        m_errorString = (boost::format(tr("RPC error: %s")) % e.what()).str();
        m_status = Status_Error;
        LOG_ERROR(tr("RPC error: ") << e.what());
    } catch (const tools::error::get_outs_error &e) {
        m_errorString = (boost::format(tr("failed to get random outputs to mix: %s")) % e.what()).str();
        m_status = Status_Error;
    } catch (const tools::error::not_enough_unlocked_money& e) {
        m_errorString = (boost::format("not enough unlocked money to transfer, available only %s, sent amount %s")
                            % cryptonote::print_money(e.available())
                            % cryptonote::print_money(e.tx_amount())).str();
        m_status = Status_Error;
        LOG_PRINT_L0(m_errorString);
        warn_of_possible_attack = false;
    } catch (const tools::error::not_enough_money& e) {
        m_errorString = (boost::format("not enough money to transfer, available only %s, sent amount %s")
                            % cryptonote::print_money(e.available())
                            % cryptonote::print_money(e.tx_amount())).str();
        m_status = Status_Error;
        LOG_PRINT_L0(m_errorString);
        warn_of_possible_attack = false;
    } catch (const tools::error::tx_not_possible& e) {
        m_errorString = (boost::format("Failed to find a way to create transactions. This is usually due to dust which is so small it cannot pay for itself in fees, or trying to send more money than the unlocked balance, or not leaving enough for fees, available only %s, transaction amount %s = %s + %s (fee)")
                            % cryptonote::print_money(e.available())
                            % cryptonote::print_money(e.tx_amount() + e.fee())
                            % cryptonote::print_money(e.tx_amount())
                            % cryptonote::print_money(e.fee())).str();
        m_status = Status_Error;
        LOG_PRINT_L0(m_errorString);
        warn_of_possible_attack = false;
    } catch (const tools::error::not_enough_outs_to_mix& e) {
        std::ostringstream writer(m_errorString);
        writer << tr("not enough outputs for specified ring size") << " = " << (e.mixin_count() + 1) << ":";
        for (std::pair<uint64_t, uint64_t> outs_for_amount : e.scanty_outs())
            writer << "\n" << tr("output amount") << " = " << cryptonote::print_money(outs_for_amount.first) << ", " << tr("found outputs to use") << " = " << outs_for_amount.second;
        m_errorString = writer.str();
        m_status = Status_Error;
        LOG_PRINT_L0(m_errorString);
    } catch (const tools::error::tx_not_constructed&) {
        m_errorString = tr("transaction was not constructed");
        m_status = Status_Error;
        warn_of_possible_attack = false;
    } catch (const tools::error::tx_rejected& e) {
        std::ostringstream writer(m_errorString);
        writer << (boost::format(tr("transaction %s was rejected by daemon with status: ")) % get_transaction_hash(e.tx())) <<  e.status();
        std::string reason = e.reason();
        m_status = Status_Error;
        m_errorString = writer.str();
        if (!reason.empty())
          m_errorString  += string(tr(". Reason: ")) + reason;
    } catch (const tools::error::tx_sum_overflow& e) {
        m_errorString = e.what();
        m_status = Status_Error;
        warn_of_possible_attack = false;
    } catch (const tools::error::zero_amount&) {
        m_errorString = tr("destination amount is zero");
        m_status = Status_Error;
        warn_of_possible_attack = false;
    } catch (const tools::error::zero_destination&) {
        m_errorString = tr("transaction has no destination");
        m_status = Status_Error;
        warn_of_possible_attack = false;
    } catch (const tools::error::tx_too_big& e) {
        m_errorString = tr("failed to find a suitable way to split transactions");
        m_status = Status_Error;
        warn_of_possible_attack = false;
    } catch (const tools::error::transfer_error& e) {
        m_errorString = (boost::format(tr("unknown transfer error: %s")) % e.what()).str();
        m_status = Status_Error;
        LOG_ERROR("unknown transfer error: " << e.to_string());
    } catch (const tools::error::multisig_export_needed& e) {
        m_errorString = (boost::format(tr("Multisig error: ")) % e.what()).str();
        m_status = Status_Error;
        LOG_ERROR("Multisig error: " << e.to_string());
        warn_of_possible_attack = false;
    } catch (const tools::error::wallet_internal_error& e) {
        m_errorString = (boost::format(tr("internal error: ")) % e.what()).str();
        m_status = Status_Error;
        LOG_ERROR("internal error: " << e.to_string());
    } catch (const std::exception &e) {
        m_errorString = string(tr("Unknown exception: ")) + e.what();
        m_status = Status_Error;
    } catch (...) {
        m_errorString = tr("Unhandled exception");
        LOG_ERROR(m_errorString);
        m_status = Status_Error;
    }
    if (warn_of_possible_attack && m_status != Status_Ok)
      m_errorString += tr("\nThere was an error, which could mean the node may be trying to get you to retry creating a transaction, and zero in on which outputs you own. Or it could be a bona fide error. It may be prudent to disconnect from this node, and not try to send a transaction immediately. Alternatively, connect to another node so the original node cannot correlate information.");

    m_wallet.startRefresh();
    return m_status == Status_Ok;
}

uint64_t PendingTransactionImpl::amount() const
{
    uint64_t result = 0;
    for (const auto &ptx : m_pending_tx)   {
        for (const auto &dest : ptx.dests) {
            result += dest.amount;
        }
    }
    return result;
}

uint64_t PendingTransactionImpl::dust() const
{
    uint64_t result = 0;
    for (const auto & ptx : m_pending_tx) {
        result += ptx.dust;
    }
    return result;
}

uint64_t PendingTransactionImpl::dustInFee() const
{
    uint64_t result = 0;
    for (const auto & ptx : m_pending_tx) {
        if (ptx.dust_added_to_fee)
            result += ptx.dust;
    }
    return result;
}

uint64_t PendingTransactionImpl::fee() const
{
    uint64_t result = 0;
    for (const auto &ptx : m_pending_tx) {
        result += ptx.fee;
    }
    return result;
}

uint64_t PendingTransactionImpl::change() const
{
    uint64_t result = 0;
    for (const auto &ptx : m_pending_tx) {
        result += ptx.change_dts.amount;
    }
    return result;
}

uint64_t PendingTransactionImpl::txCount() const
{
    return m_pending_tx.size();
}

std::vector<uint32_t> PendingTransactionImpl::subaddrAccount() const
{
    std::vector<uint32_t> result;
    for (const auto& ptx : m_pending_tx)
        result.push_back(ptx.construction_data.subaddr_account);
    return result;
}

std::vector<std::set<uint32_t>> PendingTransactionImpl::subaddrIndices() const
{
    std::vector<std::set<uint32_t>> result;
    for (const auto& ptx : m_pending_tx)
        result.push_back(ptx.construction_data.subaddr_indices);
    return result;
}

std::string PendingTransactionImpl::convertTxToStr()
{
    std::string tx;
    LOG_PRINT_L3("m_pending_tx size: " << m_pending_tx.size());

    try {
        tx = m_wallet.m_wallet->dump_tx_to_str(m_pending_tx);
        m_status = Status_Ok;
        return tx;
    } catch (const tools::error::wallet_internal_error &e) {
        m_errorString = std::string(tr("wallet internal error: ")) + e.what();
        m_status = Status_Error;
        LOG_ERROR(std::string(tr("wallet internal error: ")) + e.to_string());
    } catch (const std::exception &e) {
        m_errorString = string(tr("Unknown exception: ")) + e.what();
        m_status = Status_Error;
    } catch (...) {
        m_errorString = tr("Unhandled exception");
        m_status = Status_Error;
    }
    return ""; // m_status != Status_Ok
}

std::vector<std::string> PendingTransactionImpl::convertTxToRawBlobStr()
{
    std::vector<std::string> tx_blobs{};
    m_status = Status_Ok;
    m_errorString = "";
    for (const auto &ptx : m_pending_tx)
    {
        std::string tx_blob;
        if (cryptonote::tx_to_blob(ptx.tx, tx_blob))
            tx_blobs.push_back(tx_blob);
        else
        {
            m_status = Status_Error;
            m_errorString = tr("failed to serialize tx");
            return {};
        }
    }
    return tx_blobs;
}

double PendingTransactionImpl::getWorstFeePerByte() const
{
    double worst_fee_per_byte = std::numeric_limits<double>::max();
    for (size_t n = 0; n < m_pending_tx.size(); ++n)
    {
        const uint64_t blob_size = cryptonote::tx_to_blob(m_pending_tx[n].tx).size();
        const double fee_per_byte = m_pending_tx[n].fee / (double)blob_size;
        if (fee_per_byte < worst_fee_per_byte)
            worst_fee_per_byte = fee_per_byte;
    }
    return worst_fee_per_byte;
}

std::vector<std::vector<std::vector<std::uint64_t>>> PendingTransactionImpl::vinOffsets() const
{
    std::vector<std::vector<std::vector<std::uint64_t>>> vin_offsets;
    for (size_t n = 0; n < m_pending_tx.size(); ++n)
    {
        const cryptonote::transaction &tx = m_pending_tx[n].tx;
        vin_offsets.push_back({});
        for (size_t i = 0; i < tx.vin.size(); ++i)
        {
            if (tx.vin[i].type() != typeid(cryptonote::txin_to_key))
                continue;
            const cryptonote::txin_to_key& in_key = boost::get<cryptonote::txin_to_key>(tx.vin[i]);
            vin_offsets[n].push_back(in_key.key_offsets);
        }
    }
    return vin_offsets;
}

std::vector<std::vector<std::uint64_t>> PendingTransactionImpl::constructionDataRealOutputIndices() const
{
    std::vector<std::vector<std::uint64_t>> vin_real_output_indices;
    for (size_t n = 0; n < m_pending_tx.size(); ++n)
    {
        const cryptonote::transaction &tx = m_pending_tx[n].tx;
        const tools::wallet2::tx_construction_data &construction_data = m_pending_tx[n].construction_data;
        vin_real_output_indices.push_back({});
        for (size_t i = 0; i < tx.vin.size(); ++i)
        {
            if (tx.vin[i].type() != typeid(cryptonote::txin_to_key))
                continue;
            const tools::wallet2::transfer_details &td = m_wallet.m_wallet->get_transfer_details(construction_data.selected_transfers[i]);
            const cryptonote::tx_source_entry *sptr = NULL;
            for (const auto &src: construction_data.sources)
                if (src.outputs[src.real_output].second.dest == td.get_public_key())
                    sptr = &src;
            if (!sptr)
                return {};
            vin_real_output_indices[n].push_back(sptr->real_output);
        }
    }
    return vin_real_output_indices;
}

std::vector<std::vector<std::uint64_t>> PendingTransactionImpl::vinAmounts() const
{
    std::vector<std::vector<std::uint64_t>> vin_amounts;
    for (size_t n = 0; n < m_pending_tx.size(); ++n)
    {
        cryptonote::transaction tx = m_pending_tx[n].tx;
        vin_amounts.push_back({});
        for (size_t i = 0; i < tx.vin.size(); ++i)
        {
            if (tx.vin[i].type() != typeid(cryptonote::txin_to_key))
                continue;
            const cryptonote::txin_to_key& in_key = boost::get<cryptonote::txin_to_key>(tx.vin[i]);
            vin_amounts[n].push_back(in_key.amount);
        }
    }
    return vin_amounts;
}

std::vector<std::vector<std::unique_ptr<EnoteDetails>>> PendingTransactionImpl::getEnoteDetailsIn() const
{
    std::vector<std::unique_ptr<EnoteDetails>> eds = m_wallet.getEnoteDetails();
    std::vector<std::vector<std::unique_ptr<EnoteDetails>>> eds_out;
    for (const auto &ptx: m_pending_tx)
    {
        eds_out.push_back({});
        for (const auto i: ptx.selected_transfers)
            eds_out.back().push_back(std::move(eds[i]));
    }
    return eds_out;
}

bool PendingTransactionImpl::finishParsingTx()
{
    // import key images
    bool r = m_wallet.m_wallet->import_key_images(m_key_images);
    if (!r) return false;

    // remember key images for this tx, for when we get those txes from the blockchain
    m_wallet.m_wallet->insert_cold_key_images(m_tx_key_images);
    return true;
}

std::string PendingTransactionImpl::multisigSignData() {
    try {
        if (!m_wallet.multisig().isMultisig) {
            throw std::runtime_error("wallet is not multisig");
        }

        tools::wallet2::multisig_tx_set txSet;
        txSet.m_ptx = m_pending_tx;
        txSet.m_signers = m_signers;
        auto cipher = m_wallet.m_wallet->save_multisig_tx(txSet);

        return epee::string_tools::buff_to_hex_nodelimer(cipher);
    } catch (const std::exception& e) {
        m_status = Status_Error;
        m_errorString = std::string(tr("Couldn't multisig sign data: ")) + e.what();
    }

    return std::string();
}

void PendingTransactionImpl::signMultisigTx(std::vector<std::string> *txids /* = nullptr */) {
    try {
        std::vector<crypto::hash> ignore;

        tools::wallet2::multisig_tx_set txSet;
        txSet.m_ptx = m_pending_tx;
        txSet.m_signers = m_signers;

        if (!m_wallet.m_wallet->sign_multisig_tx(txSet, ignore)) {
            throw std::runtime_error("couldn't sign multisig transaction");
        }

        if (txids && !ignore.empty())
        {
            for (const auto &txid : ignore)
                txids->emplace_back(epee::string_tools::pod_to_hex(txid));
        }
        std::swap(m_pending_tx, txSet.m_ptx);
        std::swap(m_signers, txSet.m_signers);
    } catch (const std::exception& e) {
        m_status = Status_Error;
        m_errorString = std::string(tr("Couldn't sign multisig transaction: ")) + e.what();
    }
}

std::vector<std::string> PendingTransactionImpl::signersKeys() const {
    std::vector<std::string> keys;
    keys.reserve(m_signers.size());

    for (const auto& signer: m_signers) {
        keys.emplace_back(tools::base58::encode(cryptonote::t_serializable_object_to_blob(signer)));
    }

    return keys;
}

void PendingTransactionImpl::finishRestoringMultisigTransaction() {
    tools::wallet2::multisig_tx_set exported_txs;
    exported_txs.m_ptx = m_pending_tx;
    exported_txs.m_signers = m_signers;
    m_wallet.m_wallet->finish_loading_accepted_multisig_tx(exported_txs);
}

//----------------------------------------------------------------------------------------------------
bool PendingTransactionImpl::checkLoadedTx(const std::function<size_t()> get_num_txes, const std::function<const tools::wallet2::tx_construction_data&(size_t)> &get_tx, const std::string &extra_message)
{
  // gather info to ask the user
  uint64_t amount = 0, amount_to_dests = 0, change = 0;
  size_t min_ring_size = ~0;
  std::unordered_map<cryptonote::account_public_address, std::pair<std::string, uint64_t>> dests;
  int first_known_non_zero_change_index = -1;
  std::string payment_id_string = "";
  for (size_t n = 0; n < get_num_txes(); ++n)
  {
    const tools::wallet2::tx_construction_data &cd = get_tx(n);

    std::vector<cryptonote::tx_extra_field> tx_extra_fields;
    bool has_encrypted_payment_id = false;
    crypto::hash8 payment_id8 = crypto::null_hash8;
    if (cryptonote::parse_tx_extra(cd.extra, tx_extra_fields))
    {
      cryptonote::tx_extra_nonce extra_nonce;
      if (find_tx_extra_field_by_type(tx_extra_fields, extra_nonce))
      {
        crypto::hash payment_id;
        if(cryptonote::get_encrypted_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id8))
        {
          if (!payment_id_string.empty())
            payment_id_string += ", ";

          // if none of the addresses are integrated addresses, it's a dummy one
          bool is_dummy = true;
          for (const auto &e: cd.dests)
            if (e.is_integrated)
              is_dummy = false;

          if (is_dummy)
            payment_id_string += std::string("dummy encrypted payment ID");
          else
          {
            payment_id_string = std::string("encrypted payment ID ") + epee::string_tools::pod_to_hex(payment_id8);
            has_encrypted_payment_id = true;
          }
        }
        else if (cryptonote::get_payment_id_from_tx_extra_nonce(extra_nonce.nonce, payment_id))
        {
          if (!payment_id_string.empty())
            payment_id_string += ", ";
          payment_id_string = std::string("unencrypted payment ID ") + epee::string_tools::pod_to_hex(payment_id);
          payment_id_string += " (OBSOLETE)";
        }
      }
    }

    for (size_t s = 0; s < cd.sources.size(); ++s)
    {
      amount += cd.sources[s].amount;
      size_t ring_size = cd.sources[s].outputs.size();
      if (ring_size < min_ring_size)
        min_ring_size = ring_size;
    }
    for (size_t d = 0; d < cd.splitted_dsts.size(); ++d)
    {
      const cryptonote::tx_destination_entry &entry = cd.splitted_dsts[d];
      std::string address, standard_address = get_account_address_as_str(m_wallet.m_wallet->nettype(), entry.is_subaddress, entry.addr);
      if (has_encrypted_payment_id && !entry.is_subaddress)
      {
        address = get_account_integrated_address_as_str(m_wallet.m_wallet->nettype(), entry.addr, payment_id8);
        address += std::string(" (" + standard_address + " with encrypted payment id " + epee::string_tools::pod_to_hex(payment_id8) + ")");
      }
      else
        address = standard_address;
      auto i = dests.find(entry.addr);
      if (i == dests.end())
        dests.insert(std::make_pair(entry.addr, std::make_pair(address, entry.amount)));
      else
        i->second.second += entry.amount;
      amount_to_dests += entry.amount;
    }
    if (cd.change_dts.amount > 0)
    {
      auto it = dests.find(cd.change_dts.addr);
      if (it == dests.end())
      {
        m_status = Status_Error;
        m_errorString = tr("Claimed change does not go to a paid address");
        return false;
      }
      if (it->second.second < cd.change_dts.amount)
      {
        m_status = Status_Error;
        m_errorString = tr("Claimed change is larger than payment to the change address");
        return  false;
      }
      if (cd.change_dts.amount > 0)
      {
        if (first_known_non_zero_change_index == -1)
          first_known_non_zero_change_index = n;
        if (memcmp(&cd.change_dts.addr, &get_tx(first_known_non_zero_change_index).change_dts.addr, sizeof(cd.change_dts.addr)))
        {
          m_status = Status_Error;
          m_errorString = tr("Change goes to more than one address");
          return false;
        }
      }
      change += cd.change_dts.amount;
      it->second.second -= cd.change_dts.amount;
      if (it->second.second == 0)
        dests.erase(cd.change_dts.addr);
    }
  }

  if (payment_id_string.empty())
    payment_id_string = "no payment ID";

  std::string dest_string;
  size_t n_dummy_outputs = 0;
  for (auto i = dests.begin(); i != dests.end(); )
  {
    if (i->second.second > 0)
    {
      if (!dest_string.empty())
        dest_string += ", ";
      dest_string += (boost::format(tr("sending %s to %s")) % cryptonote::print_money(i->second.second) % i->second.first).str();
    }
    else
      ++n_dummy_outputs;
    ++i;
  }
  if (n_dummy_outputs > 0)
  {
    if (!dest_string.empty())
      dest_string += ", ";
    dest_string += std::to_string(n_dummy_outputs) + tr(" dummy output(s)");
  }
  if (dest_string.empty())
    dest_string = tr("with no destinations");

  std::string change_string;
  if (change > 0)
  {
    std::string address = get_account_address_as_str(m_wallet.m_wallet->nettype(), get_tx(0).subaddr_account > 0, get_tx(0).change_dts.addr);
    change_string += (boost::format(tr("%s change to %s")) % cryptonote::print_money(change) % address).str();
  }
  else
    change_string += tr("no change");
  uint64_t fee = amount - amount_to_dests;
  m_confirmationMessage = (boost::format(tr("Loaded %lu transactions, for %s, fee %s, %s, %s, with min ring size %lu, %s. %s. Is this okay?")) % (unsigned long)get_num_txes() % cryptonote::print_money(amount) % cryptonote::print_money(fee) % dest_string % change_string % (unsigned long)min_ring_size % payment_id_string % extra_message).str();
  return true;
}


}
