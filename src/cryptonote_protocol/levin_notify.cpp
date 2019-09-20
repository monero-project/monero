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

#include "levin_notify.h"

#include <boost/asio/steady_timer.hpp>
#include <boost/system/system_error.hpp>
#include <chrono>
#include <deque>
#include <stdexcept>

#include "common/expect.h"
#include "common/varint.h"
#include "cryptonote_config.h"
#include "crypto/random.h"
#include "cryptonote_basic/connection_context.h"
#include "cryptonote_protocol/cryptonote_protocol_defs.h"
#include "net/dandelionpp.h"
#include "p2p/net_node.h"

namespace cryptonote
{
namespace levin
{
  namespace
  {
    constexpr std::size_t connection_id_reserve_size = 100;

    constexpr const std::chrono::minutes noise_min_epoch{CRYPTONOTE_NOISE_MIN_EPOCH};
    constexpr const std::chrono::seconds noise_epoch_range{CRYPTONOTE_NOISE_EPOCH_RANGE};

    constexpr const std::chrono::seconds noise_min_delay{CRYPTONOTE_NOISE_MIN_DELAY};
    constexpr const std::chrono::seconds noise_delay_range{CRYPTONOTE_NOISE_DELAY_RANGE};

    /*! Select a randomized duration from 0 to `range`. The precision will be to
        the systems `steady_clock`. As an example, supplying 3 seconds to this
        function will select a duration from [0, 3] seconds, and the increments
        for the selection will be determined by the `steady_clock` precision
        (typically nanoseconds).

        \return A randomized duration from 0 to `range`. */
    std::chrono::steady_clock::duration random_duration(std::chrono::steady_clock::duration range)
    {
      using rep = std::chrono::steady_clock::rep;
      return std::chrono::steady_clock::duration{crypto::rand_range(rep(0), range.count())};
    }

    //! \return All outgoing connections supporting fragments in `connections`.
    std::vector<boost::uuids::uuid> get_out_connections(connections& p2p)
    {
      std::vector<boost::uuids::uuid> outs;
      outs.reserve(connection_id_reserve_size);

      /* The foreach call is serialized with a lock, but should be quick due to
         the reserve call so a strand is not used. Investigate if there is lots
         of waiting in here. */

      p2p.foreach_connection([&outs] (detail::p2p_context& context) {
        if (!context.m_is_income)
          outs.emplace_back(context.m_connection_id);
        return true;
      });

      return outs;
    }

    std::string make_tx_payload(std::vector<blobdata>&& txs, const bool pad)
    {
      NOTIFY_NEW_TRANSACTIONS::request request{};
      request.txs = std::move(txs);

      if (pad)
      {
        size_t bytes = 9 /* header */ + 4 /* 1 + 'txs' */ + tools::get_varint_data(request.txs.size()).size();
        for(auto tx_blob_it = request.txs.begin(); tx_blob_it!=request.txs.end(); ++tx_blob_it)
          bytes += tools::get_varint_data(tx_blob_it->size()).size() + tx_blob_it->size();

        // stuff some dummy bytes in to stay safe from traffic volume analysis
        static constexpr const size_t granularity = 1024;
        size_t padding = granularity - bytes % granularity;
        const size_t overhead = 2 /* 1 + '_' */ + tools::get_varint_data(padding).size();
        if (overhead > padding)
          padding = 0;
        else
          padding -= overhead;
        request._ = std::string(padding, ' ');

        std::string arg_buff;
        epee::serialization::store_t_to_binary(request, arg_buff);

        // we probably lowballed the payload size a bit, so added a but too much. Fix this now.
        size_t remove = arg_buff.size() % granularity;
        if (remove > request._.size())
          request._.clear();
        else
          request._.resize(request._.size() - remove);
        // if the size of _ moved enough, we might lose byte in size encoding, we don't care
      }

      std::string fullBlob;
      if (!epee::serialization::store_t_to_binary(request, fullBlob))
        throw std::runtime_error{"Failed to serialize to epee binary format"};

      return fullBlob;
    }

    /* The current design uses `asio::strand`s. The documentation isn't as clear
       as it should be - a `strand` has an internal `mutex` and `bool`. The
       `mutex` synchronizes thread access and the `bool` is set when a thread is
       executing something "in the strand". Therefore, if a callback has lots of
       work to do in a `strand`, asio can switch to some other task instead of
       blocking 1+ threads to wait for the original thread to complete the task
       (as is the case when client code has a `mutex` inside the callback). The
       downside is that asio _always_ allocates for the callback, even if it can
       be immediately executed. So if all work in a strand is minimal, a lock
       may be better.

       This code uses a strand per "zone" and a strand per "channel in a zone".
       `dispatch` is used heavily, which means "execute immediately in _this_
       thread if the strand is not in use, otherwise queue the callback to be
       executed immediately after the strand completes its current task".
       `post` is used where deferred execution to an `asio::io_service::run`
       thread is preferred.

       The strand per "zone" is useful because the levin
       `foreach_connection` is blocked with a mutex anyway. So this primarily
       helps with reducing blocking of a thread attempting a "flood"
       notification. Updating/merging the outgoing connections in the
       Dandelion++ map is also somewhat expensive.

       The strand per "channel" may need a re-visit. The most "expensive" code
       is figuring out the noise/notification to send. If levin code is
       optimized further, it might be better to just use standard locks per
       channel. */

    //! A queue of levin messages for a noise i2p/tor link
    struct noise_channel
    {
      explicit noise_channel(boost::asio::io_service& io_service)
        : active(nullptr),
          queue(),
          strand(io_service),
          next_noise(io_service),
          connection(boost::uuids::nil_uuid())
      {}

      // `asio::io_service::strand` cannot be copied or moved
      noise_channel(const noise_channel&) = delete;
      noise_channel& operator=(const noise_channel&) = delete;

      // Only read/write these values "inside the strand"

      epee::byte_slice active;
      std::deque<epee::byte_slice> queue;
      boost::asio::io_service::strand strand;
      boost::asio::steady_timer next_noise;
      boost::uuids::uuid connection;
    };
  } // anonymous

  namespace detail
  {
    struct zone
    {
      explicit zone(boost::asio::io_service& io_service, std::shared_ptr<connections> p2p, epee::byte_slice noise_in, bool is_public)
        : p2p(std::move(p2p)),
          noise(std::move(noise_in)),
          next_epoch(io_service),
          strand(io_service),
          map(),
          channels(),
          connection_count(0),
          is_public(is_public)
      {
        for (std::size_t count = 0; !noise.empty() && count < CRYPTONOTE_NOISE_CHANNELS; ++count)
          channels.emplace_back(io_service);
      }

      const std::shared_ptr<connections> p2p;
      const epee::byte_slice noise; //!< `!empty()` means zone is using noise channels
      boost::asio::steady_timer next_epoch;
      boost::asio::io_service::strand strand;
      net::dandelionpp::connection_map map;//!< Tracks outgoing uuid's for noise channels or Dandelion++ stems
      std::deque<noise_channel> channels;  //!< Never touch after init; only update elements on `noise_channel.strand`
      std::atomic<std::size_t> connection_count; //!< Only update in strand, can be read at any time
      const bool is_public;                      //!< Zone is public ipv4/ipv6 connections
    };
  } // detail

  namespace
  {
    //! Adds a message to the sending queue of the channel.
    class queue_covert_notify
    {
      std::shared_ptr<detail::zone> zone_;
      epee::byte_slice message_; // Requires manual copy constructor
      const std::size_t destination_;

    public:
      queue_covert_notify(std::shared_ptr<detail::zone> zone, epee::byte_slice message, std::size_t destination)
        : zone_(std::move(zone)), message_(std::move(message)), destination_(destination)
      {}

      queue_covert_notify(queue_covert_notify&&) = default;
      queue_covert_notify(const queue_covert_notify& source)
        : zone_(source.zone_), message_(source.message_.clone()), destination_(source.destination_)
      {}

      //! \pre Called within `zone_->channels[destionation_].strand`.
      void operator()()
      {
        if (!zone_)
          return;

        noise_channel& channel = zone_->channels.at(destination_);
        assert(channel.strand.running_in_this_thread());

        if (!channel.connection.is_nil())
          channel.queue.push_back(std::move(message_));
      }
    };

    //! Sends a message to every active connection
    class flood_notify
    {
      std::shared_ptr<detail::zone> zone_;
      epee::byte_slice message_; // Requires manual copy
      boost::uuids::uuid source_;

    public:
      explicit flood_notify(std::shared_ptr<detail::zone> zone, epee::byte_slice message, const boost::uuids::uuid& source)
        : zone_(std::move(zone)), message_(message.clone()), source_(source)
      {}

      flood_notify(flood_notify&&) = default;
      flood_notify(const flood_notify& source)
        : zone_(source.zone_), message_(source.message_.clone()), source_(source.source_)
      {}

      void operator()() const
      {
        if (!zone_ || !zone_->p2p)
          return;

        assert(zone_->strand.running_in_this_thread());

        /* The foreach should be quick, but then it iterates and acquires the
           same lock for every connection. So do in a strand because two threads
           will ping-pong each other with cacheline invalidations. Revisit if
           algorithm changes or the locking strategy within the levin config
           class changes. */

        std::vector<boost::uuids::uuid> connections;
        connections.reserve(connection_id_reserve_size);
        zone_->p2p->foreach_connection([this, &connections] (detail::p2p_context& context) {
          /* Only send to outgoing connections when "flooding" over i2p/tor.
             Otherwise this makes the tx linkable to a hidden service address,
             making things linkable across connections. */
          if (this->source_ != context.m_connection_id && (this->zone_->is_public || !context.m_is_income))
            connections.emplace_back(context.m_connection_id);
          return true;
        });

        for (const boost::uuids::uuid& connection : connections)
          zone_->p2p->send(message_.clone(), connection);
      }
    };

    //! Updates the connection for a channel.
    struct update_channel
    {
      std::shared_ptr<detail::zone> zone_;
      const std::size_t channel_;
      const boost::uuids::uuid connection_;

      //! \pre Called within `stem_.strand`.
      void operator()() const
      {
        if (!zone_)
          return;

        noise_channel& channel = zone_->channels.at(channel_);
        assert(channel.strand.running_in_this_thread());
        static_assert(
          CRYPTONOTE_MAX_FRAGMENTS <= (noise_min_epoch / (noise_min_delay + noise_delay_range)),
          "Max fragments more than the max that can be sent in an epoch"
        );

        /* This clears the active message so that a message "in-flight" is
           restarted. DO NOT try to send the remainder of the fragments, this
           additional send time can leak that this node was sending out a real
           notify (tx) instead of dummy noise. */

        channel.connection = connection_;
        channel.active = nullptr;

        if (connection_.is_nil())
          channel.queue.clear();
      }
    };

    //! Merges `out_connections_` into the existing `zone_->map`.
    struct update_channels
    {
      std::shared_ptr<detail::zone> zone_;
      std::vector<boost::uuids::uuid> out_connections_;

      //! \pre Called within `zone->strand`.
      static void post(std::shared_ptr<detail::zone> zone)
      {
        if (!zone)
          return;

        assert(zone->strand.running_in_this_thread());

        zone->connection_count = zone->map.size();
        for (auto id = zone->map.begin(); id != zone->map.end(); ++id)
        {
          const std::size_t i = id - zone->map.begin();
          zone->channels[i].strand.post(update_channel{zone, i, *id});
        }
      }

      //! \pre Called within `zone_->strand`.
      void operator()()
      {
        if (!zone_)
          return;

        assert(zone_->strand.running_in_this_thread());
        if (zone_->map.update(std::move(out_connections_)))
          post(std::move(zone_));
      }
    };

    //! Swaps out noise channels entirely; new epoch start.
    class change_channels
    {
      std::shared_ptr<detail::zone> zone_;
      net::dandelionpp::connection_map map_; // Requires manual copy constructor

    public:
      explicit change_channels(std::shared_ptr<detail::zone> zone, net::dandelionpp::connection_map map)
        : zone_(std::move(zone)), map_(std::move(map))
      {}

      change_channels(change_channels&&) = default;
      change_channels(const change_channels& source)
        : zone_(source.zone_), map_(source.map_.clone())
      {}

      //! \pre Called within `zone_->strand`.
      void operator()()
      {
        if (!zone_)
          return

        assert(zone_->strand.running_in_this_thread());

        zone_->map = std::move(map_);
        update_channels::post(std::move(zone_));
      }
    };

    //! Sends a noise packet or real notification and sets timer for next call.
    struct send_noise
    {
      std::shared_ptr<detail::zone> zone_;
      const std::size_t channel_;

      static void wait(const std::chrono::steady_clock::time_point start, std::shared_ptr<detail::zone> zone, const std::size_t index)
      {
        if (!zone)
          return;

        noise_channel& channel = zone->channels.at(index);
        channel.next_noise.expires_at(start + noise_min_delay + random_duration(noise_delay_range));
        channel.next_noise.async_wait(
          channel.strand.wrap(send_noise{std::move(zone), index})
        );
      }

      //! \pre Called within `zone_->channels[channel_].strand`.
      void operator()(boost::system::error_code error)
      {
        if (!zone_ || !zone_->p2p || zone_->noise.empty())
          return;

        if (error && error != boost::system::errc::operation_canceled)
          throw boost::system::system_error{error, "send_noise timer failed"};

        assert(zone_->channels.at(channel_).strand.running_in_this_thread());

        const auto start = std::chrono::steady_clock::now();
        noise_channel& channel = zone_->channels.at(channel_);

        if (!channel.connection.is_nil())
        {
          epee::byte_slice message = nullptr;
          if (!channel.active.empty())
            message = channel.active.take_slice(zone_->noise.size());
          else if (!channel.queue.empty())
          {
            channel.active = channel.queue.front().clone();
            message = channel.active.take_slice(zone_->noise.size());
          }
          else
            message = zone_->noise.clone();

          if (zone_->p2p->send(std::move(message), channel.connection))
          {
            if (!channel.queue.empty() && channel.active.empty())
              channel.queue.pop_front();
          }
          else
          {
            channel.active = nullptr;
            channel.connection = boost::uuids::nil_uuid();
            zone_->strand.post(
              update_channels{zone_, get_out_connections(*zone_->p2p)}
            );
          }
        }

        wait(start, std::move(zone_), channel_);
      }
    };

    //! Prepares connections for new channel epoch and sets timer for next epoch
    struct start_epoch
    {
      // Variables allow for Dandelion++ extension
      std::shared_ptr<detail::zone> zone_;
      std::chrono::seconds min_epoch_;
      std::chrono::seconds epoch_range_;
      std::size_t count_;

      //! \pre Should not be invoked within any strand to prevent blocking.
      void operator()(const boost::system::error_code error = {})
      {
        if (!zone_ || !zone_->p2p)
          return;

        if (error && error != boost::system::errc::operation_canceled)
          throw boost::system::system_error{error, "start_epoch timer failed"};

        const auto start = std::chrono::steady_clock::now();
        zone_->strand.dispatch(
          change_channels{zone_, net::dandelionpp::connection_map{get_out_connections(*(zone_->p2p)), count_}}
        );

        detail::zone& alias = *zone_;
        alias.next_epoch.expires_at(start + min_epoch_ + random_duration(epoch_range_));
        alias.next_epoch.async_wait(start_epoch{std::move(*this)});
      }
    };
  } // anonymous

  notify::notify(boost::asio::io_service& service, std::shared_ptr<connections> p2p, epee::byte_slice noise, bool is_public)
    : zone_(std::make_shared<detail::zone>(service, std::move(p2p), std::move(noise), is_public))
  {
    if (!zone_->p2p)
      throw std::logic_error{"cryptonote::levin::notify cannot have nullptr p2p argument"};

    if (!zone_->noise.empty())
    {
      const auto now = std::chrono::steady_clock::now();
      start_epoch{zone_, noise_min_epoch, noise_epoch_range, CRYPTONOTE_NOISE_CHANNELS}();
      for (std::size_t channel = 0; channel < zone_->channels.size(); ++channel)
        send_noise::wait(now, zone_, channel);
    }
  }

  notify::~notify() noexcept
  {}

  notify::status notify::get_status() const noexcept
  {
    if (!zone_)
      return {false, false};

    return {!zone_->noise.empty(), CRYPTONOTE_NOISE_CHANNELS <= zone_->connection_count};
  }

  void notify::new_out_connection()
  {
    if (!zone_ || zone_->noise.empty() || CRYPTONOTE_NOISE_CHANNELS <= zone_->connection_count)
      return;

    zone_->strand.dispatch(
      update_channels{zone_, get_out_connections(*(zone_->p2p))}
    );
  }

  void notify::run_epoch()
  {
    if (!zone_)
      return;
    zone_->next_epoch.cancel();
  }

  void notify::run_stems()
  {
    if (!zone_)
      return;

    for (noise_channel& channel : zone_->channels)
      channel.next_noise.cancel();
  }

  bool notify::send_txs(std::vector<blobdata> txs, const boost::uuids::uuid& source, const bool pad_txs)
  {
    if (!zone_)
      return false;

    if (!zone_->noise.empty() && !zone_->channels.empty())
    {
      // covert send in "noise" channel
      static_assert(
        CRYPTONOTE_MAX_FRAGMENTS * CRYPTONOTE_NOISE_BYTES <= LEVIN_DEFAULT_MAX_PACKET_SIZE, "most nodes will reject this fragment setting"
      );

      // padding is not useful when using noise mode
      const std::string payload = make_tx_payload(std::move(txs), false);
      epee::byte_slice message = epee::levin::make_fragmented_notify(
        zone_->noise, NOTIFY_NEW_TRANSACTIONS::ID, epee::strspan<std::uint8_t>(payload)
      );
      if (CRYPTONOTE_MAX_FRAGMENTS * zone_->noise.size() < message.size())
      {
        MERROR("notify::send_txs provided message exceeding covert fragment size");
        return false;
      }

      for (std::size_t channel = 0; channel < zone_->channels.size(); ++channel)
      {
        zone_->channels[channel].strand.dispatch(
          queue_covert_notify{zone_, message.clone(), channel}
        );
      }
    }
    else
    {
      const std::string payload = make_tx_payload(std::move(txs), pad_txs);
      epee::byte_slice message =
        epee::levin::make_notify(NOTIFY_NEW_TRANSACTIONS::ID, epee::strspan<std::uint8_t>(payload));

      // traditional monero send technique
      zone_->strand.dispatch(flood_notify{zone_, std::move(message), source});
    }

    return true;
  }
} // levin
} // net
