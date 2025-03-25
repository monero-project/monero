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

Building all the tests requires the dependencies listed in the `requirements.txt` file.

To install said dependencies, it is recommended that you use a virtual environment (`virtualenv`). 

To learn more about how to install `virtualenv`, please read [this](https://virtualenv.pypa.io/en/latest/installation.html) short guide.

```bash
python3 -m pip install --user virtualenv
```

After `virtualenv` is installed, create your own virtual environment with:

```bash
virtualenv venv
```

**Note**: You can name your virtual environment anything you want; but, this repository is only configured to ignore the contents of `venv` and `.venv`.

After activating your virtual environment, as detailed in the aformentioned guide, install the required dependencies:
```bash
pip install -r requirements.txt
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
