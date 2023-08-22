#include "coins.h"
#include "coins_info.h"
#include "wallet.h"
#include "crypto/hash.h"
#include "wallet/wallet2.h"
#include "common_defines.h"

#include <string>
#include <vector>

using namespace epee;

namespace Monero {

Coins::~Coins() = default;

CoinsImpl::CoinsImpl(WalletImpl *wallet)
        : m_wallet(wallet) {}

CoinsImpl::~CoinsImpl()
{
    for (auto t : m_rows)
        delete t;
}

int CoinsImpl::count() const
{
    boost::shared_lock<boost::shared_mutex> lock(m_rowsMutex);
    int result = m_rows.size();
    return result;
}

CoinsInfo *CoinsImpl::coin(int index) const
{
    boost::shared_lock<boost::shared_mutex> lock(m_rowsMutex);
    // sanity check
    if (index < 0)
        return nullptr;
    auto index_ = static_cast<unsigned>(index);
    return index_ < m_rows.size() ? m_rows[index_] : nullptr;
}

std::vector<CoinsInfo *> CoinsImpl::getAll() const
{
    boost::shared_lock<boost::shared_mutex> lock(m_rowsMutex);
    return m_rows;
}


void CoinsImpl::refresh()
{
    LOG_PRINT_L2("Refreshing coins");

    boost::unique_lock<boost::shared_mutex> lock(m_rowsMutex);
    boost::shared_lock<boost::shared_mutex> transfers_lock(m_wallet->m_wallet->m_transfers_mutex);

    // delete old outputs;
    for (auto t : m_rows)
        delete t;
    m_rows.clear();

    for (size_t i = 0; i < m_wallet->m_wallet->get_num_transfer_details(); ++i)
    {
        const tools::wallet2::transfer_details &td = m_wallet->m_wallet->get_transfer_details(i);

        auto ci = new CoinsInfoImpl();
        ci->m_blockHeight = td.m_block_height;
        ci->m_hash = string_tools::pod_to_hex(td.m_txid);
        ci->m_internalOutputIndex = td.m_internal_output_index;
        ci->m_globalOutputIndex = td.m_global_output_index;
        ci->m_spent = td.m_spent;
        ci->m_frozen = td.m_frozen;
        ci->m_spentHeight = td.m_spent_height;
        ci->m_amount = td.m_amount;
        ci->m_rct = td.m_rct;
        ci->m_keyImageKnown = td.m_key_image_known;
        ci->m_pkIndex = td.m_pk_index;
        ci->m_subaddrIndex = td.m_subaddr_index.minor;
        ci->m_subaddrAccount = td.m_subaddr_index.major;
        ci->m_address = m_wallet->m_wallet->get_subaddress_as_str(td.m_subaddr_index); // todo: this is expensive, cache maybe?
        ci->m_addressLabel = m_wallet->m_wallet->get_subaddress_label(td.m_subaddr_index);
        ci->m_keyImage = string_tools::pod_to_hex(td.m_key_image);
        ci->m_unlockTime = td.m_tx.unlock_time;
        ci->m_unlocked = m_wallet->m_wallet->is_transfer_unlocked(td);
        ci->m_pubKey = string_tools::pod_to_hex(td.get_public_key());
        ci->m_coinbase = td.m_tx.vin.size() == 1 && td.m_tx.vin[0].type() == typeid(cryptonote::txin_gen);
        ci->m_description = m_wallet->m_wallet->get_tx_note(td.m_txid);

        m_rows.push_back(ci);
    }
}

void CoinsImpl::setFrozen(int index)
{
    try
    {
        m_wallet->m_wallet->freeze(index);
        refresh();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("setLabel: " << e.what());
    }
}

void CoinsImpl::thaw(int index)
{
    try
    {
        m_wallet->m_wallet->thaw(index);
        refresh();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("thaw: " << e.what());
    }
}

bool CoinsImpl::isTransferUnlocked(uint64_t unlockTime, uint64_t blockHeight) {
    return m_wallet->m_wallet->is_transfer_unlocked(unlockTime, blockHeight);
}

void CoinsImpl::setDescription(int index, const std::string &description)
{
    try
    {
        const tools::wallet2::transfer_details &td = m_wallet->m_wallet->get_transfer_details(index);
        m_wallet->m_wallet->set_tx_note(td.m_txid, description);
        refresh();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("setDescription: " << e.what());
    }
}

} // namespace