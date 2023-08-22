#ifndef CAKE_COINS_INFO_H
#define CAKE_COINS_INFO_H

#include "wallet/api/wallet2_api.h"
#include <string>
#include <ctime>

namespace Monero {

class CoinsImpl;

class CoinsInfoImpl : public CoinsInfo
{
public:
    CoinsInfoImpl();
    ~CoinsInfoImpl();

    virtual uint64_t blockHeight() const override;
    virtual std::string hash() const override;
    virtual size_t internalOutputIndex() const override;
    virtual uint64_t globalOutputIndex() const override;
    virtual bool spent() const override;
    virtual bool frozen() const override;
    virtual uint64_t spentHeight() const override;
    virtual uint64_t amount() const override;
    virtual bool rct() const override;
    virtual bool keyImageKnown() const override;
    virtual size_t pkIndex() const override;
    virtual uint32_t subaddrIndex() const override;
    virtual uint32_t subaddrAccount() const override;
    virtual std::string address() const override;
    virtual std::string addressLabel() const override;
    virtual std::string keyImage() const override;
    virtual uint64_t unlockTime() const override;
    virtual bool unlocked() const override;
    virtual std::string pubKey() const override;
    virtual bool coinbase() const override;
    virtual std::string description() const override;

private:
    uint64_t    m_blockHeight;
    std::string m_hash;
    size_t      m_internalOutputIndex;
    uint64_t    m_globalOutputIndex;
    bool        m_spent;
    bool        m_frozen;
    uint64_t    m_spentHeight;
    uint64_t    m_amount;
    bool        m_rct;
    bool        m_keyImageKnown;
    size_t      m_pkIndex;
    uint32_t    m_subaddrIndex;
    uint32_t    m_subaddrAccount;
    std::string m_address;
    std::string m_addressLabel;
    std::string m_keyImage;
    uint64_t    m_unlockTime;
    bool        m_unlocked;
    std::string m_pubKey;
    bool        m_coinbase;
    std::string m_description;

    friend class CoinsImpl;

};

} // namespace

namespace Bitmonero = Monero;

#endif // CAKE_COINS_INFO_H