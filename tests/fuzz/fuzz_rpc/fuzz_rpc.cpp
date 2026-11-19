#include "initialisation.h"
#include "rpc_endpoints.h"
#include <fuzzer/FuzzedDataProvider.h>

#include <csignal>
#include <csetjmp>
#include <iostream>
#include <vector>
#include <algorithm>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // In general an iteration needs a fair amount of data so ensure we have enough
  // to work with, otherwise return 0 to skip this iteration.
  if (size < 512) {
    return 0;
  }

  // Determine if this iteration should run in safe mode
#ifdef SAFE
  constexpr bool is_safe_mode = true;
#else
  constexpr bool is_safe_mode = false;
#endif

  const auto fuzz_targets = get_fuzz_targets(is_safe_mode);
  // The handler map can have gaps in its keys when endpoints are removed.
  std::vector<int> available_selectors;
  available_selectors.reserve(fuzz_targets.size());
  for (const auto& target : fuzz_targets) {
    available_selectors.push_back(target.first);
  }

  // Disable fatal exits for logging.
  el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);

  // Prepare base FuzzedDataProvider
  FuzzedDataProvider provider(data, size);

  // Randomly choose multiple fuzz_targets to fuzz
  unsigned rpc_messages_to_send = provider.ConsumeIntegralInRange<unsigned>(1, 16);
  std::vector<int> selectors;
  if (is_safe_mode) {
    selectors.reserve(rpc_messages_to_send);
  } else {
    selectors.reserve(rpc_messages_to_send + priority_fuzz_targets.size());
    for (const auto& target : priority_fuzz_targets) {
      selectors.push_back(target.first);
    }

    // Randomly shuffle the selectors for priority fuzz targets
    for (unsigned i = 0; i < priority_fuzz_targets.size(); i++) {
      unsigned target = provider.ConsumeIntegralInRange<unsigned>(0, priority_fuzz_targets.size() - 1);
      std::swap(selectors[i], selectors[target]);
    }
  }

  // Randomly select rpc functions to call
  for (unsigned i = 0; i < rpc_messages_to_send && provider.remaining_bytes() >= 2; ++i) {
    const auto index = provider.ConsumeIntegralInRange<unsigned>(0, available_selectors.size() - 1);
    selectors.push_back(available_selectors[index]);
  }

  // Initialise core and core_rpc_server
  auto core_env = initialise_rpc_core();
  if (!core_env) {
    return 0;
  }
  auto& dummy_core = core_env->core;
  auto rpc_handler = initialise_rpc_server(*dummy_core, provider.ConsumeBool());

  // Generate random blocks/miners/transactions and push to the core blockchains
  if (!generate_random_blocks(*dummy_core, provider)) {
    // No randomised blocks have been successfully added, skipping this iteration
    dummy_core->get_blockchain_storage().get_db().batch_stop();
    return 0;
  }

  for (int selector : selectors) {
    try {
      // Fuzz the target function
      fuzz_targets.at(selector)(*rpc_handler->rpc, provider);
    } catch (const std::runtime_error&) {
      // Known runtime_error thrown from monero
    } catch (const cryptonote::DB_ERROR& e) {
      // Known error thrown from monero on internal blockchain DB check
      // when fuzzing with random values
#ifdef CATCH_ALL_EXCEPTIONS
    } catch (...) {
      // Silent all exceptions
#endif
    }
  }

  dummy_core->get_blockchain_storage().get_db().batch_stop();
  return 0;
}
