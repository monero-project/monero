#pragma once


#include "MoneroWallet.hh"

namespace Monero {

class WalletObserver
{
public:

    virtual void on_new_block(uint64_t height) = 0;

    virtual void on_money_received(uint64_t height, Monero::amount_mini_t amount_mini) = 0;

    virtual void on_money_spent(uint64_t height, Monero::amount_mini_t amount_mini) = 0;

    virtual void on_skip_transaction(uint64_t height, const std::string& transaction_id) = 0;

    virtual void on_payment_received(uint64_t height, const std::string& payment_id, const Payment& payment) = 0;
};

}