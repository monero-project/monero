#include "coins_info.h"

using namespace std;

namespace Monero {

CoinsInfo::~CoinsInfo() = default;

CoinsInfoImpl::CoinsInfoImpl()
        : m_blockHeight(0)
        , m_internalOutputIndex(0)
        , m_globalOutputIndex(0)
        , m_spent(false)
        , m_frozen(false)
        , m_spentHeight(0)
        , m_amount(0)
        , m_rct(false)
        , m_keyImageKnown(false)
        , m_pkIndex(0)
        , m_subaddrAccount(0)
        , m_subaddrIndex(0)
        , m_unlockTime(0)
        , m_unlocked(false)
{

}

CoinsInfoImpl::~CoinsInfoImpl() = default;

uint64_t CoinsInfoImpl::blockHeight() const
{
    return m_blockHeight;
}

string CoinsInfoImpl::hash() const
{
    return m_hash;
}

size_t CoinsInfoImpl::internalOutputIndex() const {
    return m_internalOutputIndex;
}

uint64_t CoinsInfoImpl::globalOutputIndex() const
{
    return m_globalOutputIndex;
}

bool CoinsInfoImpl::spent() const
{
    return m_spent;
}

bool CoinsInfoImpl::frozen() const
{
    return m_frozen;
}

uint64_t CoinsInfoImpl::spentHeight() const
{
    return m_spentHeight;
}

uint64_t CoinsInfoImpl::amount() const
{
    return m_amount;
}

bool CoinsInfoImpl::rct() const {
    return m_rct;
}

bool CoinsInfoImpl::keyImageKnown() const {
    return m_keyImageKnown;
}

size_t CoinsInfoImpl::pkIndex() const {
    return m_pkIndex;
}

uint32_t CoinsInfoImpl::subaddrIndex() const {
    return m_subaddrIndex;
}

uint32_t CoinsInfoImpl::subaddrAccount() const {
    return m_subaddrAccount;
}

string CoinsInfoImpl::address() const {
    return m_address;
}

string CoinsInfoImpl::addressLabel() const {
    return m_addressLabel;
}

string CoinsInfoImpl::keyImage() const {
    return m_keyImage;
}

uint64_t CoinsInfoImpl::unlockTime() const {
    return m_unlockTime;
}

bool CoinsInfoImpl::unlocked() const {
    return m_unlocked;
}

string CoinsInfoImpl::pubKey() const {
    return m_pubKey;
}

bool CoinsInfoImpl::coinbase() const {
    return m_coinbase;
}

string CoinsInfoImpl::description() const {
    return m_description;
}
} // namespace

namespace Bitmonero = Monero;