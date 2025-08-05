#include "initialisation.h"
#include "rpc_endpoints.h"
#include <fuzzer/FuzzedDataProvider.h>

#include <csignal>
#include <csetjmp>
#include <iostream>
#include <vector>
#include <algorithm>

std::vector<size_t> historical_counts;

// Helper definition for timeout recovery
static sigjmp_buf jump_env;
void timeout_handler(int sig) {
  if (tools::performance_timers) {
    tools::performance_timers->clear();
    delete tools::performance_timers;
    tools::performance_timers = nullptr;
  }

  siglongjmp(jump_env, 1);
}

// Helper function to generate a biased selector targeting rarely-fuzzed RPC endpoints
int biased_selector(FuzzedDataProvider& provider) {
  size_t total = std::accumulate(historical_counts.begin(), historical_counts.end(), size_t{0}) + 1;

  // Calculate the probabilites for each RPC endpoint.
  // The more frequantly it has been selected, the less likely it becomes next time.
  std::vector<double> probabilities;
  for (size_t c : historical_counts) {
    probabilities.push_back((total - c + 1.0) / total);
  }

  // Compute the total probabilty weight and consume a random double from the provider
  double sum = std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
  double r = provider.ConsumeFloatingPoint<double>() * sum;

  // Loop over each weighted probability. The higher the weight, the more likely
  // the selector will match as the value of 'r' decreases below zero.
  // Also, there is a second bias on the order of the rpc endpoints, which
  // higher priority endpoints with smaller selector indx has slighly more chance
  // to be selected.
  for (size_t i = 0; i < probabilities.size(); ++i) {
    if (r < probabilities[i]) {
      return static_cast<int>(i);
    }
    r -= probabilities[i];
  }

  // Eeturn the final index in case no match was found earlly.
  return static_cast<int>(probabilities.size() - 1);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // In general an iteration need a fair amount of data so ensure we have enough
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

  // Initialise historical counts for bias selector generation
  if (historical_counts.empty()) {
    historical_counts.resize(fuzz_targets.size(), 0);
  }

  // Disable fatal exits for logging.
  el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);

  // Register a handler for timeout alarm signal
  signal(SIGALRM, timeout_handler);

  // Prepare base FuzzedDataProvider
  FuzzedDataProvider provider(data, size);

  // Randomly choose multiple fuzz_targets to fuzz
  int rpc_messages_to_sent = provider.ConsumeIntegralInRange<int>(1, 16);
  std::vector<int> selectors;
  if (is_safe_mode) {
    selectors.reserve(rpc_messages_to_sent);
  } else {
    selectors.reserve(rpc_messages_to_sent + priority_fuzz_targets.size());
    for (int i = 0; i < priority_fuzz_targets.size(); ++i) {
        selectors.push_back(i);
    }
  }

  // Randomly
  for (int i = 0; i < rpc_messages_to_sent && provider.remaining_bytes() >= 2; ++i) {
    int selector;
    if (provider.ConsumeBool()) {
      // Using general FuzzedDataProvider approach
      selector = provider.ConsumeIntegralInRange<int>(0, fuzz_targets.size() - 1);
    } else {
      // Using FuzzedDataProvider with bias on unfuzzed/low-fuzzed rpc endpoints
      selector = biased_selector(provider);
    }
    selectors.push_back(selector);
    historical_counts[selector]++;
  }

  // Initialise core and core_rpc_server
  auto dummy_core = initialise_rpc_core();
  auto rpc_handler = initialise_rpc_server(*dummy_core, provider, !is_safe_mode);

  // Generalise random blocks/miners/transactions and push to the core blockchains
  if (!generate_random_blocks(*dummy_core, provider)) {
    // No randomised blocks has been successfully added, skipping this iteration
    dummy_core->get_blockchain_storage().get_db().batch_stop();
    return 0;
  }

  // Disable bootstrap daemon
  disable_bootstrap_daemon(*rpc_handler->rpc);

  for (int selector : selectors) {
    // Configure returning point for timeout
    if (sigsetjmp(jump_env, 1) == 0) {
      try {
        // Start a timeout alarm for 120 secs
        // SIGALARM will be sent once 120 is up
        // timeout_handler catches the SIGALARM and return to the
        // last sigsetjmp call with matching value to allow the code
        // skipping to the next iteration
        alarm(120);
        // Fuzz the target function
        fuzz_targets[selector](*rpc_handler->rpc, provider);
      } catch (const std::runtime_error&) {
        // Known runtime_error thrown from monero
      } catch (const cryptonote::DB_ERROR& e) {
        // Known error thrown from monero on internal blockchain DB check
        // when fuzzing with random values
      }

      // Clear the alarm before next iteration
      alarm(0);
    }
  }

  dummy_core->get_blockchain_storage().get_db().batch_stop();
  return 0;
}
