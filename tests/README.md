# Running all tests

To run all tests, run:

```bash
cd /path/to/monero
make [-jn] debug-test # where n is number of compiler processes
```

To test a release build, replace `debug-test` with `release-test` in the previous command.

# Core tests

Core tests take longer than any other Monero tests, due to the high amount of computational work involved in validating core components.

Tests are located in `tests/core_tests/`, and follow a straightforward naming convention. Most cases cover core functionality (`block_reward.cpp`, `chaingen.cpp`, `rct.cpp`, etc.), while some cover basic security tests (`double_spend.cpp` & `integer_overflow.cpp`).

To run only Monero's core tests (after building):

```bash
cd build/debug/tests/core_tests
ctest
```

To run the same tests on a release build, replace `debug` with `release`.


# Crypto Tests

Crypto tests are located under the `tests/crypto` directory.

- `crypto-tests.h` contains test harness headers
- `main.cpp` implements the driver for the crypto tests

Tests correspond to components under `src/crypto/`. A quick comparison reveals the pattern, and new tests should continue the naming convention.

To run only Monero's crypto tests (after building):

```bash
cd build/debug/tests/crypto
ctest
```

To run the same tests on a release build, replace `debug` with `release`.

# Functional tests

[TODO]
Functional tests are located under the `tests/functional_tests` directory.

Building all the tests requires installing the following dependencies:
```bash
pip install requests psutil monotonic zmq deepdiff
```

First, run a regtest daemon in the offline mode and with a fixed difficulty:
```bash
monerod --regtest --offline --fixed-difficulty 1
```
Alternatively, you can run multiple daemons and let them connect with each other by using `--add-exclusive-node`. In this case, make sure that the same fixed difficulty is given to all the daemons.

Next, restore a mainnet wallet with the following seed and restore height 0 (the file path doesn't matter):
```bash
velvet lymph giddy number token physics poetry unquoted nibs useful sabotage limits benches lifestyle eden nitrogen anvil fewest avoid batch vials washing fences goat unquoted
```

Open the wallet file with `monero-wallet-rpc` with RPC port 18083. Finally, start tests by invoking ./blockchain.py or ./speed.py

## Parameters

Configuration of individual tests.

### Mining test

The following environment variables may be set to control the mining test:

- `MINING_NO_MEASUREMENT` - set to anything to use large enough and fixed mining timeouts (use case: very slow PCs and no intention to change the mining code)
- `MINING_SILENT`         - set to anything to disable mining logging

For example, to customize the run of the functional tests, you may run the following commands from the build directory:

```bash
export MINING_NO_MEASUREMENT=1
ctest -V -R functional_tests_rpc
unset MINING_NO_MEASUREMENT
```

# Fuzz tests

Fuzz tests are written using American Fuzzy Lop (AFL), and located under the `tests/fuzz` directory.

An additional helper utility is provided `contrib/fuzz_testing/fuzz.sh`. AFL must be installed, and some additional setup may be necessary for the script to run properly.

## OSS-Fuzz

Monero is integrated into [OSS-Fuzz](https://github.com/google/oss-fuzz) and the project integration
is available [here](https://github.com/google/oss-fuzz/tree/master/projects/monero). OSS-Fuzz builds
and runs the fuzzers continuously, so long as Monero's OSS-Fuzz [build script](https://github.com/google/oss-fuzz/blob/master/projects/monero/build.sh) builds them.

Issues found by OSS-Fuzz are publicly available (following a disclosure deadline) on the OSS-Fuzz issue tracker [here](https://issues.oss-fuzz.com/issues?q=project%3Dmonero).
The issue tracker only displays limited information, and only maintainers with emails listed in the [project.yaml](https://github.com/google/oss-fuzz/blob/master/projects/monero/project.yaml) have access to full details.

Coverage reports are built on a daily basis and data about this can be found at [introspector.oss-fuzz.com](https://introspector.oss-fuzz.com) [here](https://introspector.oss-fuzz.com/project-profile?project=monero).

### Build and run fuzzers by way of OSS-Fuzz

**Building Monero's fuzzers with OSS-Fuzz**

```sh
$ git clone https://github.com/google/oss-fuzz
$ cd oss-fuzz
$ python3 infra/helper.py build_fuzzers monero

# Display what was build
$ ls build/out/monero/
base58_fuzz_tests                       cold-outputs_fuzz_tests_seed_corpus.zip      llvm-symbolizer                              signature_fuzz_tests
base58_fuzz_tests_seed_corpus.zip       cold-transaction_fuzz_tests                  load-from-binary_fuzz_tests                  signature_fuzz_tests_seed_corpus.zip
block_fuzz_tests                        cold-transaction_fuzz_tests_seed_corpus.zip  load-from-binary_fuzz_tests_seed_corpus.zip  transaction_fuzz_tests
block_fuzz_tests_seed_corpus.zip        http-client_fuzz_tests                       load-from-json_fuzz_tests                    transaction_fuzz_tests_seed_corpus.zip
bulletproof_fuzz_tests                  http-client_fuzz_tests_seed_corpus.zip       load-from-json_fuzz_tests_seed_corpus.zip    tx-extra_fuzz_tests
bulletproof_fuzz_tests_seed_corpus.zip  levin_fuzz_tests                             parse-url_fuzz_tests                         tx-extra_fuzz_tests_seed_corpus.zip
cold-outputs_fuzz_tests                 levin_fuzz_tests_seed_corpus.zip             parse-url_fuzz_tests_seed_corpus.zip
```

**Run fuzzing harness with OSS-Fuzz**

Assuming you performed the above steps for building the fuzzers and are in the OSS-Fuzz root directory:

```sh
$ python3 infra/helper.py run_fuzzer monero base58_fuzz_tests
...
...
INFO: Loaded 1 modules   (9075 inline 8-bit counters): 9075 [0x55d1c3d6cfd8, 0x55d1c3d6f34b),
INFO: Loaded 1 PC tables (9075 PCs): 9075 [0x55d1c3d6f350,0x55d1c3d92a80),
INFO:        1 files found in /tmp/base58_fuzz_tests_corpus
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: seed corpus: files: 1 min: 95b max: 95b total: 95b rss: 33Mb
#2      INITED cov: 18 ft: 19 corp: 1/95b exec/s: 0 rss: 33Mb
#3      NEW    cov: 19 ft: 23 corp: 2/190b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 1 ChangeByte-
#4      NEW    cov: 20 ft: 24 corp: 3/285b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 1 ChangeByte-
#5      NEW    cov: 22 ft: 26 corp: 4/359b lim: 95 exec/s: 0 rss: 34Mb L: 74/95 MS: 1 EraseBytes-
#6      NEW    cov: 23 ft: 29 corp: 5/454b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 1 ChangeByte-
#8      NEW    cov: 24 ft: 30 corp: 6/549b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 2 CrossOver-ChangeBit-
#12     NEW    cov: 25 ft: 35 corp: 7/606b lim: 95 exec/s: 0 rss: 34Mb L: 57/95 MS: 4 ChangeBinInt-ShuffleBytes-ShuffleBytes-EraseBytes-
#14     NEW    cov: 26 ft: 38 corp: 8/655b lim: 95 exec/s: 0 rss: 34Mb L: 49/95 MS: 2 ChangeBinInt-EraseBytes-
#17     NEW    cov: 27 ft: 40 corp: 9/708b lim: 95 exec/s: 0 rss: 34Mb L: 53/95 MS: 3 ChangeASCIIInt-ChangeBit-EraseBytes-
#18     NEW    cov: 28 ft: 41 corp: 10/803b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 1 ChangeByte-
#20     NEW    cov: 28 ft: 42 corp: 11/852b lim: 95 exec/s: 0 rss: 34Mb L: 49/95 MS: 2 ChangeASCIIInt-ShuffleBytes-
#22     REDUCE cov: 28 ft: 42 corp: 11/847b lim: 95 exec/s: 0 rss: 34Mb L: 90/95 MS: 2 ChangeBinInt-CrossOver-
#25     NEW    cov: 29 ft: 47 corp: 12/942b lim: 95 exec/s: 0 rss: 34Mb L: 95/95 MS: 3 ChangeBit-ChangeBit-CopyPart-
#39     REDUCE cov: 29 ft: 47 corp: 12/941b lim: 95 exec/s: 0 rss: 34Mb L: 94/95 MS: 4 ChangeByte-CopyPart-ChangeASCIIInt-EraseBytes-
#41     NEW    cov: 30 ft: 48 corp: 13/991b lim: 95 exec/s: 0 rss: 34Mb L: 50/95 MS: 2 CopyPart-CrossOver-
#57     NEW    cov: 31 ft: 49 corp: 14/1068b lim: 95 exec/s: 0 rss: 34Mb L: 77/95 MS: 1 InsertRepeatedBytes-
#63     NEW    cov: 32 ft: 50 corp: 15/1147b lim: 95 exec/s: 0 rss: 34Mb L: 79/95 MS: 1 CrossOver-
...
```


# Hash tests

Hash tests exist under `tests/hash`, and include a set of target hashes in text files.

To run only Monero's hash tests (after building):

```bash
cd build/debug/tests/hash
ctest
```

To run the same tests on a release build, replace `debug` with `release`.

To run specific hash test, you can use `ctest` `-R` parameter. For example to run only `blake2b` hash tests:

```
ctest -R hash-blake2b
```

# Libwallet API tests

[TODO]

# Net Load tests

[TODO]

# Performance tests

Performance tests are located in `tests/performance_tests`, and test features for performance metrics on the host machine.

To run only Monero's performance tests (after building):

```bash
cd build/debug/tests/performance_tests
./performance_tests
```

The path may be build/Linux/master/debug (adapt as necessary for your platform).

If the `performance_tests` binary does not exist, try running `make` in the `build/debug/tests/performance_tests` directory.

To run the same tests on a release build, replace `debug` with `release`.

# Unit tests

Unit tests are defined under the `tests/unit_tests` directory. Independent components are tested individually to ensure they work properly on their own.

To run only Monero's unit tests (after building):

```bash
cd build/debug/tests/unit_tests
ctest
```

To run the same tests on a release build, replace `debug` with `release`.

# Writing new tests

## Test hygiene

When writing new tests, please implement all functions in `.cpp` or `.c` files, and only put function headers in `.h` files. This will help keep the fairly complex test suites somewhat sane going forward.

## Writing fuzz tests

[TODO]
hash
