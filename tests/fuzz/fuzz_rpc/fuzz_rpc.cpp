#include "initialisation.h"
#include "rpc_endpoints.h"
#include <fuzzer/FuzzedDataProvider.h>

#include <iostream>
#include <vector>
#include <algorithm>
#include <utility> // Required for std::swap

// Define minimum data size required for a meaningful fuzzing iteration.
// This prevents the fuzzer from wasting cycles on tiny inputs.
constexpr size_t MIN_FUZZ_INPUT_SIZE = 512;
// An unsigned int is typically 4 bytes. We need enough bytes to consume a selector.
constexpr size_t MIN_BYTES_PER_SELECTOR = sizeof(unsigned);

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip this iteration if the input is too small for meaningful processing.
    if (size < MIN_FUZZ_INPUT_SIZE) {
        return 0;
    }

    // Determine if this iteration should run in safe mode based on the build flag.
#ifdef SAFE
    constexpr bool is_safe_mode = true;
#else
    constexpr bool is_safe_mode = false;
#endif

    // Retrieve the list of available fuzz targets (RPC endpoints).
    auto fuzz_targets = get_fuzz_targets(is_safe_mode);

    // Disable fatal exits for logging in the easyloggingpp library.
    // This allows the fuzzer to continue and check for other issues
    // even when a fatal log event occurs within the target code.
    el::Loggers::addFlag(el::LoggingFlag::DisableApplicationAbortOnFatalLog);

    // Prepare the primary data provider for fuzzing.
    FuzzedDataProvider provider(data, size);

    // RAII structure to ensure the database batch is stopped when exiting the function.
    // This cleans up the resource automatically regardless of the exit path.
    auto core_env = initialise_rpc_core();
    auto& dummy_core = core_env->core;

    // Use a lambda function or a simple object for RAII on the database operation.
    // This prevents duplicated batch_stop() calls.
    auto db_cleanup_guard = [](cryptonote::core& core) {
        core.get_blockchain_storage().get_db().batch_stop();
    };
    std::unique_ptr<void, decltype(db_cleanup_guard)> db_guard(
        (void*)1, // Dummy non-null pointer
        db_cleanup_guard);

    // Randomly choose the number of RPC messages to send (1 to 16).
    unsigned rpc_messages_to_send = provider.ConsumeIntegralInRange<unsigned>(1, 16);
    std::vector<unsigned> selectors;

    if (!is_safe_mode) {
        // In non-safe mode, include priority fuzz targets first.
        selectors.reserve(rpc_messages_to_send + priority_fuzz_targets.size());

        // Add priority target indices.
        for (unsigned i = 0; i < priority_fuzz_targets.size(); ++i) {
            selectors.push_back(i);
        }

        // Apply a random swap-based "shuffle" on the priority targets using fuzzer data.
        for (unsigned i = 0; i < priority_fuzz_targets.size(); i++) {
            // Check if enough bytes remain for the selector index consumption.
            if (provider.remaining_bytes() < MIN_BYTES_PER_SELECTOR) {
                break;
            }
            unsigned target = provider.ConsumeIntegralInRange<unsigned>(0, priority_fuzz_targets.size() - 1);
            std::swap(selectors[i], selectors[target]);
        }
    } else {
        // In safe mode, reserve only for the random messages.
        selectors.reserve(rpc_messages_to_send);
    }

    // Randomly select other RPC functions to call.
    for (unsigned i = 0; i < rpc_messages_to_send; ++i) {
        // CRITICAL FIX: Ensure enough bytes remain for consuming an unsigned selector.
        if (provider.remaining_bytes() < MIN_BYTES_PER_SELECTOR) {
            break;
        }
        unsigned selector = provider.ConsumeIntegralInRange<unsigned>(0, fuzz_targets.size() - 1);
        selectors.push_back(selector);
    }

    // Initialise the RPC server handler.
    auto rpc_handler = initialise_rpc_server(*dummy_core, provider, !is_safe_mode);

    // Generate random blocks, miners, and transactions, and push them to the core blockchains.
    if (!generate_random_blocks(*dummy_core, provider)) {
        // No randomized blocks have been successfully added, skipping this iteration.
        // The db_guard ensures batch_stop is called automatically.
        return 0;
    }

    // Disable the bootstrap daemon to prevent unnecessary network activity during fuzzing.
    disable_bootstrap_daemon(*rpc_handler->rpc);

    // Iterate through the selected RPC targets and fuzz each one.
    for (unsigned selector : selectors) {
        // Skip if the selector is out of bounds (should not happen, but safe check).
        if (selector >= fuzz_targets.size()) {
            continue;
        }

        try {
            // Execute the fuzz target function (RPC endpoint call).
            fuzz_targets[selector](*rpc_handler->rpc, provider);
        } catch (const std::runtime_error&) {
            // Known runtime_error thrown from monero; silently ignore.
        } catch (const cryptonote::DB_ERROR&) {
            // Known database error thrown on internal blockchain DB check.
        }
        // WARNING: The CATCH_ALL_EXCEPTIONS block is excluded here to allow the fuzzer 
        // to discover unexpected crashes, which is the primary goal of fuzzing.
    }

    // The db_guard automatically calls batch_stop() when the function returns.
    return 0;
}
