# Dynamic Seed Corpus Generator

This directory contains source code to build a program which generates seed corpus data dynamically.
It should be used during the fuzzer build stage, before running the fuzzers. In the OSS-Fuzz
platform, the resultant data should be zipped into seed corpus files, one for each fuzzer. Outside
of OSS-Fuzz, these seed corpus paths can be passed directly to the individual fuzzing programs.
