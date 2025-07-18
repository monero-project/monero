# Fuzzing Harness Explanation

The fuzzing harness skips the transport layer and directly initialises a `core_rpc_server` object. It then calls and fuzzes the RPC endpoint function handlers directly, removing the need to start a fake server while still allowing the handler logic for each RPC API to be fuzzed.


## `fuzz_rpc.cpp` / `fuzz_zmq.cpp`

These are the main fuzzing entry points, containing `LLVMFuzzerTestOneInput`, which manages each iteration of the fuzzing process.

## `initialisation.cpp` / `initialisation.h`

This set of source files provides initialisation helper functions to configure the `core_rpc_server` class with dummy protocols, P2P, payment modules, and other components. It also includes functions to generate fake random blocks, miners, and transactions for use in fuzzing.

## `rpc_endpoints.cpp` / `rpc_endpoints.h` / `zmq_endpoints.cpp` / `zmq_endpoints.h`

These source files handle the creation of the necessary request and response objects and the invocation of specific endpoint functions. These simulate real RPC requests / ZMQ requests for the purpose of fuzzing. There are three categories of RPC endpoint functions and only one for ZMQ endpoint functions:

* **Safe**: Considered stable and unlikely to fail.
* **Risky**: More prone to failure and early exits, especially if no valid blockchain is generated or no payment modules are configured.
* **Priority**: Most critical RPC endpoint functions that will at least run once per iteration.

# Building the Fuzzing Harnesses

The same `fuzz_rpc.cpp` is compiled into two versions of the fuzzer, one with the `SAFE` macro defined and one without. Meanwhile, `fuzz_zmq` is compiled into a single version.

* With `SAFE` defined: Only safe endpoint functions are fuzzed.
* Without `SAFE`: Both safe and risky functions are included in the fuzzing process, and payment modules are configured.

# Fuzzing Harness Flow

## Select a Random RPC Endpoint Function / ZMQ Endpoint Function

Random data is used to generate selectors that choose an RPC endpoint function / ZMQ endpoint function to fuzz, either from the safe or risky function maps (depending on whether the `SAFE` macro is defined). A function from the priority category is always included at least once.


## `initialise_rpc_core` / `initialise_rpc_server`

At the start of each fuzzing iteration, a `core_rpc_server` object is created and initialised with dummy protocols, P2P, payment modules, and more.
If the `SAFE` macro is not defined, the payment module will also be initialised.
This step is skipped for `fuzz_zmq`.

## `generate_random_blocks`

The provided random data is used to initialise a set of random blocks, which are added to a dummy blockchain (initialised using `FAKECHAIN`).
If blocks are successfully added, a list of random transactions is then generated and also pushed to the blockchain.
This step is skipped for `fuzz_zmq`.

## Real Fuzzing

The selected RPC endpoint helper function in `rpc_endpoints` or `zmq_endpoints` generates a random request and response object for the specific call, and then invokes the corresponding handler in the `core_rpc_server` object.
