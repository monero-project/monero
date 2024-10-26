# Intro

This directory contains tools, which can be used for checking the health of the project, like build/run time analyzers, lints, etc.

# Usage

Unless it's stated differently, these scripts should be called from a given source directory, where you want the checks to be performed, for instance:

`og@ghetto:~/dev/monero$ utils/health/clang-build-time-analyzer-run.sh`

## ClangBuildAnalyzer

`utils/health/clang-build-time-analyzer-run.sh`
The CBA helps in finding culprints of slow compilation.
On the first run, the script will complain about the missing ClangBuildAnalyzer binary and will point you to another script, which is able to clone and build the required binary.

## clang-tidy

`utils/health/clang-tidy-run-cc.sh`
`utils/health/clang-tidy-run-cpp.sh`
Performs Lint checks on the source code and stores the result in the build directory. More information on the [home page](https://clang.llvm.org/extra/clang-tidy/).

## include-what-you-use

`utils/health/clang-include-what-you-use-run.sh`
Analyses the header file hierarchy and delivers hints on how to reduce their complexity. More information on the [home page](https://include-what-you-use.org/).


## Valgrind checks

`utils/health/valgrind-tests.sh`
This script is able to run valgrind's callgrind, cachegrind and memcheck for a given set of executables.
It expects ONE PARAMETER, which points to a file with paths to executables and their arguments, written line by line. For example:

```
ls -l -h
build/tests/unit_tests/unit_tests
```

The `*.out` results can be interpreted with the `kcachegrind` tool. 
The memcheck output is just a readable text file with a summary at the end.

# Footer

Responsible: mj-xmr

