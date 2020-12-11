
#include "byte_slice.h"
#include "byte_stream.h"
#include "misc_log_ex.h"
#include "span.h"
#include "storages/portable_storage.h"
#include "storages/portable_storage_to_bin.h"

namespace epee
{
namespace serialization
{
  bool portable_storage::store_to_binary(byte_slice& target, const std::size_t initial_buffer_size)
  {
    TRY_ENTRY();
    byte_stream ss;
    ss.reserve(initial_buffer_size);
    storage_block_header sbh{};
    sbh.m_signature_a = SWAP32LE(PORTABLE_STORAGE_SIGNATUREA);
    sbh.m_signature_b = SWAP32LE(PORTABLE_STORAGE_SIGNATUREB);
    sbh.m_ver = PORTABLE_STORAGE_FORMAT_VER;
    ss.write(epee::as_byte_span(sbh));
    pack_entry_to_buff(ss, m_root);
    target = epee::byte_slice{std::move(ss)};
    return true;
    CATCH_ENTRY("portable_storage::store_to_binary", false)
  }
}
}
