// Copyright (c) 2019, The Monero Project
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

#pragma once

#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <memory>
#include <vector>

#include "byte_slice.h"
#include "cryptonote_basic/blobdatatype.h"
#include "cryptonote_protocol/fwd.h"
#include "net/enums.h"
#include "span.h"

namespace epee
{
namespace levin
{
    template<typename> class async_protocol_handler_config;
}
}

namespace nodetool
{
  template<typename> struct p2p_connection_context_t;
}

namespace cryptonote
{
namespace levin
{
  namespace detail
  {
    using p2p_context = nodetool::p2p_connection_context_t<cryptonote::cryptonote_connection_context>;
    struct zone; //!< Internal data needed for zone notifications
  } // detail

  using connections = epee::levin::async_protocol_handler_config<detail::p2p_context>;

  //! Provides tx notification privacy
  class notify
  {
    std::shared_ptr<detail::zone> zone_;

  public:
    struct status
    {
      bool has_noise;
      bool connections_filled;
    };

    //! Construct an instance that cannot notify.
    notify() noexcept
      : zone_(nullptr)
    {}

    //! Construct an instance with available notification `zones`.
    explicit notify(boost::asio::io_service& service, std::shared_ptr<connections> p2p, epee::byte_slice noise, bool is_public, bool pad_txs);

    notify(const notify&) = delete;
    notify(notify&&) = default;

    ~notify() noexcept;

    notify& operator=(const notify&) = delete;
    notify& operator=(notify&&) = default;

    //! \return Status information for zone selection.
    status get_status() const noexcept;

    //! Probe for new outbound connection - skips if not needed.
    void new_out_connection();

    //! Run the logic for the next epoch immediately. Only use in testing.
    void run_epoch();

    //! Run the logic for the next stem timeout imemdiately. Only use in  testing.
    void run_stems();

    //! Run the logic for flushing all Dandelion++ fluff queued txs. Only use in testing.
    void run_fluff();

    /*! Send txs using `cryptonote_protocol_defs.h` payload format wrapped in a
        levin header. The message will be sent in a "discreet" manner if
        enabled - if `!noise.empty()` then the `command`/`payload` will be
        queued to send at the next available noise interval. Otherwise, a
        Dandelion++ fluff algorithm will be used.

        \note Eventually Dandelion++ stem sending will be used here when
          enabled.

        \param txs The transactions that need to be serialized and relayed.
        \param source The source of the notification. `is_nil()` indicates this
          node is the source. Dandelion++ will use this to map a source to a
          particular stem.

      \return True iff the notification is queued for sending. */
    bool send_txs(std::vector<blobdata> txs, const boost::uuids::uuid& source);
  };
} // levin
} // net
