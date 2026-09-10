# Monero License Library

## API usage

There are two functions exposed:
* `monero_license_num()`
* `monero_license_get()`

`monero_license_num()` returns the number of license entries, and
`monero_license_get(N)` is used to fetch the `N`-th license entry.

A license entry contains the following info:
* Source Location: a URL or other string to access the source coe
* Copyright year range
* Copyright holder's name
* License text

## Adding / removing entries

The license entries are entered into the file `sublicenses.txt.in`, which gets
compiled into the `monero_license` library target. The format for
`sublicenses.txt.in` is simply multiple license entries concatenated together.
A single license entry follows the format:

```
%%%%%% .... %%%%
<Source location URL>

Copyright (c) <Year> <Holder>

<License text
More...
...
...
Last line>
```

A line composed entirely of `%` symbols denotes the start of a new entry. `Year`
can be a single year, a range of years, `present`, etc as long as it doesn't
contain a space. `Holder` may contain spaces.

You can verify your changes to `sublicenses.txt.in` by:
1. Compiling the unit tests with `CMAKE_BUILD_TYPE=Debug`
2. Running `unit_tests --gtest_filter=license.dump_all_short --log-level=1`
3. Observing the printed output and checking that it matches the intentions
