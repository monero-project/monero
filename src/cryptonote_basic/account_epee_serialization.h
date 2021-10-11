#pragma once

#include "account.h"
#include "p2p/portable_scheme/scheme.h"

namespace portable_scheme {

namespace scheme_space {

template<>
struct scheme<cryptonote::account_public_address> {
  using T = cryptonote::account_public_address;
  using tags = map_tag<
    field_tag<KEY("m_spend_public_key"), span_tag<>>,
    field_tag<KEY("m_view_public_key"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD_POD(m_spend_public_key)
    READ_FIELD_POD(m_view_public_key)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD_POD(m_spend_public_key)
    WRITE_FIELD_POD(m_view_public_key)
  END_WRITE()
};

template<>
struct scheme<cryptonote::account_keys> {
  using T = cryptonote::account_keys;
  using tags = map_tag<
    field_tag<KEY("m_account_address"), scheme<decltype(T::m_account_address)>::tags>,
    field_tag<KEY("m_encryption_iv"), span_tag<>>,
    field_tag<KEY("m_multisig_keys"), span_tag<>>,
    field_tag<KEY("m_spend_secret_key"), span_tag<>>,
    field_tag<KEY("m_view_secret_key"), span_tag<>>
  >;
  BEGIN_READ(T)
    READ_FIELD(m_account_address)
    READ_FIELD_CUSTOM(m_spend_secret_key, scheme<pod_tag<crypto::ec_scalar>>)
    READ_FIELD_CUSTOM(m_view_secret_key, scheme<pod_tag<crypto::ec_scalar>>)
    READ_FIELD_LIST_CUSTOM(m_multisig_keys, scheme<container_pod_tag<decltype(T::m_multisig_keys), false, crypto::ec_scalar>>)
    READ_FIELD_POD(m_encryption_iv)
  END_READ()
  BEGIN_WRITE(T)
    WRITE_FIELD(m_account_address)
    WRITE_FIELD_CUSTOM(m_spend_secret_key, scheme<pod_tag<crypto::ec_scalar>>)
    WRITE_FIELD_CUSTOM(m_view_secret_key, scheme<pod_tag<crypto::ec_scalar>>)
    WRITE_FIELD_LIST_CUSTOM(m_multisig_keys, scheme<container_pod_tag<decltype(T::m_multisig_keys), false, crypto::ec_scalar>>)
    WRITE_FIELD_POD_WITH_DEFAULT(m_encryption_iv, crypto::chacha_iv{{}})
  END_WRITE()
};

template<>
struct scheme<cryptonote::account_base> {
  using T = cryptonote::account_base;
  using tags = map_tag<
    field_tag<KEY("m_creation_timestamp"), base_tag<std::uint64_t>>,
    field_tag<KEY("m_keys"), scheme<cryptonote::account_keys>::tags>
  >;
  BEGIN_READ_RAW(
    private:
      const T &t;
      uint64_t m_creation_timestamp;
    public:
      read_t(const T &t): t(t), m_creation_timestamp(t.get_createtime()) {}
  )
    READ_FIELD_NAME("m_keys", t.get_keys())
    READ_FIELD_NAME("m_creation_timestamp", m_creation_timestamp)
  END_READ()
  BEGIN_WRITE_RAW(
    private:
      T &t;
      cryptonote::account_keys m_keys{};
      uint64_t m_creation_timestamp{};
    public:
      write_t(T &t): t(t) {}
  )
    WRITE_FIELD_NAME("m_keys", m_keys);
    WRITE_FIELD_NAME("m_creation_timestamp", m_creation_timestamp);
    WRITE_CHECK(
      struct archive_t {
        const cryptonote::account_keys &m_keys;
        const uint64_t &m_creation_timestamp;
        void operator&(cryptonote::account_keys& m_keys) const {
          m_keys = this->m_keys;
        }
        void operator&(std::uint64_t &m_creation_timestamp) const {
          m_creation_timestamp = this->m_creation_timestamp;
        }
      };
      archive_t archive{m_keys, m_creation_timestamp};
      t.serialize(archive, 0);
      return true;
    )
  END_WRITE()
};

}

}
