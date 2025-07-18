#include <fuzzer/FuzzedDataProvider.h>
#include "zmq_endpoints.h"
#include <vector>
#include <cstring>

using namespace cryptonote;
using namespace cryptonote::listener;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 64) {
    return 0;
  }

  FuzzedDataProvider provider(data, size);

  void* ctx = zmq_ctx_new();
  if (!ctx) {
    return 0;
  }

  // Randomly choose multiple zmq_targets to fuzz
  int to_sent = provider.ConsumeIntegralInRange<int>(1, 8);
  std::vector<int> selectors;
  selectors.reserve(to_sent);
  for (int i = 0; i < to_sent && provider.remaining_bytes() >= 2; ++i) {
    uint16_t raw = provider.ConsumeIntegral<uint16_t>();
    selectors.push_back(raw % zmq_targets.size());
  }

  try {
    zmq_pub pub(ctx);
    for (int selector : selectors) {
      zmq_targets[selector](pub, provider);
    }
  } catch (const std::runtime_error& e) {
    // Ignore known runtime_error from checking
  }

  zmq_ctx_shutdown(ctx);
  zmq_ctx_term(ctx);

  return 0;
}
