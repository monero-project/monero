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

  // Retrieve a list of all fuzz targets
  auto fuzz_targets = get_fuzz_targets(is_safe_mode);

  // Disable fatal exits for logging.
  el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);

  // Prepare base FuzzedDataProvider
  FuzzedDataProvider provider(data, size);

  // Randomly choose multiple fuzz_targets to fuzz
  int rpc_messages_to_send = provider.ConsumeIntegralInRange<int>(1, 16);
  std::vector<int> selectors;
  if (is_safe_mode) {
    selectors.reserve(rpc_messages_to_send);
  } else {
    selectors.reserve(rpc_messages_to_send + priority_fuzz_targets.size());
    for (int i = 0; i < priority_fuzz_targets.size(); ++i) {
        selectors.push_back(i);
    }

    // Randomly shuffle the selectors for priority fuzz targets
    for (int i = 0; i < priority_fuzz_targets.size(); i++) {
      int target = provider.ConsumeIntegralInRange<int>(0, priority_fuzz_targets.size() - 1);
      std::swap(selectors[i], selectors[target]);
    }
  }

  // Randomly select rpc functions to call
  for (int i = 0; i < rpc_messages_to_send && provider.remaining_bytes() >= 2; ++i) {
    int selector = provider.ConsumeIntegralInRange<int>(0, fuzz_targets.size() - 1);
    selectors.push_back(selector);
  }

  // Initialise core and core_rpc_server
  auto core_env = initialise_rpc_core();
  auto& dummy_core = core_env->core;
  auto rpc_handler = initialise_rpc_server(*dummy_core, provider, !is_safe_mode);

  // Generate random blocks/miners/transactions and push to the core blockchains
  if (!generate_random_blocks(*dummy_core, provider)) {
    // No randomised blocks have been successfully added, skipping this iteration
    dummy_core->get_blockchain_storage().get_db().batch_stop();
    return 0;
  }

  // Disable bootstrap daemon
  disable_bootstrap_daemon(*rpc_handler->rpc);

  for (int selector : selectors) {
    try {
      // Fuzz the target function
      fuzz_targets[selector](*rpc_handler->rpc, provider);
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
