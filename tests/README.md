# Crypto tests

## Running crypto Perl tests

Crypto tests require the Math::GMP Perl library, make sure it is installed on you system before running the tests.

Installing dependencies (using cpan):

```
cpan
cpan> install Math::BigInt::GMP
cpan> install Digest::Keccak
```

Running tests:

```
TESTPATH=/path/to/monero/tests
cd $TESTPATH
perl -I $TESTPATH cryptotest.pl
```

Important: must include test path for perl to import cryptolib.pl

## Writing new crypto tests

[TODO]

# Core tests

## Running core tests

Monero uses the Google C++ Testing Framework (`gtest`) to write unit, integration and functional tests for core and other features of the project.
`gtest` runs on top of cmake, and you can run all tests by:

```
cd /path/to/monero
make [-jn] debug-test # where n is number of compiler processes
```

To test a release build, replace `debug-test` with `release-test` in the previous command.

One can also run individual test suites by building monero, then running `ctest` in test suite folders.

Run only the hash tests:

```
cd /path/to/monero
make [-j#] debug
cd build/debug/tests/hash
ctest
```

To run the same tests on a release build, replace `debug` with `release` in previous commands.

## Writing new tests

Based on local tests and Google's guide on creating [simple tests with gtest](https://github.com/google/googletest/blob/master/googletest/docs/Primer.md#simple-tests)

Tests consist of a test harness (defined with the TEST() macro), and the test body consisting of gtest assertions.

Example of a test harness:

```
TEST(test_case_name, test_name) {
 ... test body ...

}
```

As an example in Monero's [crypto unit test](./unit_tests/crypto.cpp):

```
TEST(Crypto, Ostream)
{
  EXPECT_TRUE(is_formatted<crypto::hash8>());
  EXPECT_TRUE(is_formatted<crypto::hash>());
  EXPECT_TRUE(is_formatted<crypto::public_key>());
  EXPECT_TRUE(is_formatted<crypto::secret_key>());
  EXPECT_TRUE(is_formatted<crypto::signature>());
  EXPECT_TRUE(is_formatted<crypto::key_derivation>());
  EXPECT_TRUE(is_formatted<crypto::key_image>());
}

```

The assertions inside the test harness are a bit complex, but fairly straightforward.

- `is_formatted<T>()` is a polymorphic function that accepts the various types of structs defined in [crypto/hash.h](../src/crypto/hash.h).

Just above the test harness, we have the definition for `is_formatted`:

```
  template<typename T>
  bool is_formatted()
  {
    T value{};

    static_assert(alignof(T) == 1, "T must have 1 byte alignment");
    static_assert(sizeof(T) <= sizeof(source), "T is too large for source");
    static_assert(sizeof(T) * 2 <= sizeof(expected), "T is too large for destination");
    std::memcpy(std::addressof(value), source, sizeof(T));

    std::stringstream out;
    out << "BEGIN" << value << "END";
    return out.str() == "BEGIN<" + std::string{expected, sizeof(T) * 2} + ">END";
  }
```

`T value {}` produces the data member of the struct (`hash8` has `char data[8]`), which runs a number of tests to ensure well structured hash data.

Let's write a new test for the keccak function:

```
  bool keccak_harness()
  {
      size_t inlen = sizeof(source);
      int mdlen = (int)sizeof(md);
      int ret = keccak(source, inlen, md, mdlen);
      if (md[0] != 0x00)
      {
          return true;
      }
      else if (!ret)
      {
          return true;
      }
      else
      {
          return false;
      }
  }
```

This is a basic test that ensures `keccak()` returns successfully when given proper input. It reuses the `source` array for input, and a new byte array `md` for storing the hash digest. Full source is in the [crypto unit test](./unit_tests/crypto.cpp).

Now let's create a new test harness:

```
TEST(Crypto, Keccak)
{
  # ...
  EXPECT_TRUE(keccak_harness());
}

```

This creates a new test under the `Crypto` test case named `Keccak`. The harness includes one assertion `EXPECT_TRUE(keccak_harness())`, which invokes `keccak_harness()`. More complex logic can be added to test various functionality of the `Keccak` library.

To run the new test:

```
cd /path/to/monero
make -jn debug # if no debug build exists
cd build/debug/tests/unit_test
make -jn
make -jn test
```

# Fuzz tests

## Running fuzz tests

```
cd /path/to/monero
make [-jn] fuzz # where n is number of compiler processes
```

or

```
cd path/to/monero
./contrib/fuzz_testing/fuzz.sh
```

## Writing fuzz tests

[TODO]
