# Compiling, debugging and testing efficiently

This document describes ways of compiling, debugging and testing efficiently for various use cases.
The intended audience are developers, who want to leverage newly added tricks to Monero via `CMake`. The document will lower the entry point for these developers.
Before reading this document, please consult section "Build instructions" in the main README.md. 
Some information from README.md will be repeated here, but the aim is to go beyond it.

## Basic compilation

Monero can be compiled via the main `Makefile`, using one of several targets listed there.
The targets are actually presets for `CMake` calls with various options, plus `make` commands for building or in some cases `make test` for testing.
It is possible to extract these `CMake` calls and modify them for your specific needs. For example, a minimal external cmake command to compile Monero, executed from within a newly created build directory could look like:

`cmake -S "$DIR_SRC" -DCMAKE_BUILD_TYPE=Release && make`

where the variable `DIR_SRC` is expected to store the path to the Monero source code.

## Toolchain requirements

This section is the authoritative statement of the language standard, the
build-system floor, the compiler floors and the verified
compiler/standard-library pairings for this repository. `README.md` repeats the
floors in prose and refers here for the matrix, and the compiler-floor guard in
the root `CMakeLists.txt` points its `FATAL_ERROR` messages at this section, so
the three are kept in agreement with each other.

### Language standard

Monero is compiled as C++23. The root build sets `CMAKE_CXX_STANDARD 23`
together with `CMAKE_CXX_STANDARD_REQUIRED ON` and `CMAKE_CXX_EXTENSIONS OFF`,
so no target silently falls back to an older dialect and no compiler extension
is enabled. C sources are compiled as C11, declared the same way with
`CMAKE_C_STANDARD 11`, `CMAKE_C_STANDARD_REQUIRED ON` and
`CMAKE_C_EXTENSIONS OFF`.

CMake spells the dialect in one of two equivalent ways, and which one it picks
depends on the CMake version and the target platform as well as on the
compiler. For GCC it emits `-std=c++23`, from GCC 11.1 onwards and therefore
for every GCC this repository accepts. For Clang the mapping changed inside the
CMake range this repository supports: 3.25 and 3.26 emit `-std=c++2b` for every
Clang they accept at this dialect, whatever its version; 3.27.0 and newer emit
`-std=c++23` for Clang 17 or newer and keep `-std=c++2b` for Clang 12 through
16; and from 3.27.7 that threshold is Clang 18 when the target system is
Android, so an Android build with Clang 17 gets `-std=c++2b` as well. The Clang
16 row of the matrix below is annotated with `-std=c++2b` for that reason. Both
spellings select the same language, so a build log carrying either one is
building C++23.

### CMake

CMake 3.25 or newer is required, and `cmake_minimum_required(VERSION 3.25)` is
declared both in the root build and in the small embedded project it configures
with `try_compile`. The `CXX_STANDARD` value `23` on its own would need only
CMake 3.20; 3.25 is the floor because `cmake_minimum_required` also raises the
policy version, and the tree relies on the behaviour that comes with it. In
particular, CMP0119 makes CMake pass an explicit `-x <language>` for sources
whose `LANGUAGE` property is set, which is why
`src/crypto/CryptonightR_template.S` is declared `LANGUAGE ASM` in
`src/crypto/CMakeLists.txt`: under `LANGUAGE C` the policy would pass `-x c`
and the assembler source would fail to compile.

The versions exercised for this dialect are 3.25.3 (configure) and 3.28.3 (full
builds).

### Compiler floors and verified pairings

The floors are GCC 13, Clang 16, Apple Clang 15 (Xcode 15) and MinGW-w64 GCC
13. They are enforced at configure time by the guard in the root
`CMakeLists.txt`, and they are floors of compiler *families*: what has actually
been built and tested is the matrix below, which is the only statement of that.
A pairing the matrix does not list is untested rather than endorsed, and the
Status column names the evidence for each row, so that a verified pairing is
never read as one that is only declared, only enforced by CI, or still awaiting
its gate.

| Compiler | Standard library / Boost | Status |
| --- | --- | --- |
| GCC ≥ 13 (floor) — 13.3.0 | libstdc++ 13 / Boost 1.83 | **Verified**; the Ubuntu 24.04 CI image and the reference Debian-13-class configuration |
| GCC 14.2.0 | libstdc++ 14 / Boost 1.83 | **Verified**; the Debian 13 CI image |
| Clang ≥ 16 (floor) — 16.0.6 | libstdc++ 13 / Boost 1.83 | **Verified** (`-std=c++2b`); the only verified Clang 16 pairing |
| Clang 16.0.6 | libstdc++ 14 | **Fails** at C++23 in five objects — a compiler/standard-library pairing defect, not first-party code; documented as unsupported |
| Clang 18.1.3 | libstdc++ 14 / Boost 1.91.0 | **Verified** and warning-clean |
| Clang 18.1.3 | libstdc++ 14 / Boost 1.83 | Builds and passes tests; emits one Boost-internal Beast deprecation — **conditional support requirement**: Clang ≥ 18 with libstdc++ ≥ 14 needs Boost ≥ 1.84 to be warning-clean |
| Clang 18.1.3, Debug, `STACK_TRACE` via libunwind 1.6.2 | libstdc++ 14 | **Verified** for `obj_common` only: 17 objects, including `src/common/stack_trace.cpp`, at C++23 with zero warnings; the only exercise of the libunwind path, which no CI job builds |
| Clang (any) | libc++ | **Not verified**; no CI or release path on Linux uses it |
| Apple Clang ≥ 15 (floor, Xcode 15 = LLVM 16 base) | Xcode libc++ / Homebrew Boost | **Declared, guard-enforced, not demonstrated**; the only continuous evidence is **CI-enforced**, on a newer Xcode (see below) |
| MinGW-w64 GCC ≥ 13 (floor) | libstdc++ 13 / MSYS2 or depends Boost | **CI-enforced only**: the Windows `ucrt64` job and the `x86_64-w64-mingw32` depends job |
| depends cross hosts and Guix (`gcc-15` Linux, `clang-toolchain-22` Darwin) | Boost 1.91.0, OpenSSL 3.5.7, ZeroMQ 4.3.5, protobuf 3.21.12 (all pinned) | **Not built here; pre-acceptance gate**: pinned, not verified (see below) |

The Apple Clang row is the one floor in this matrix that has not been
demonstrated, and it is published as exactly that: declared, and enforced by
the configure-time guard, but not verified. Xcode 15 ships a compiler based on
LLVM 16, which makes it the Apple equivalent of the Clang 16 floor, but no
Apple compiler was available when the matrix was measured, so Xcode 15 itself
has never been configured, built or tested. The only continuous evidence is
the `macOS (brew)` job, which builds and runs the reduced test tier on the
newer Xcode that `macOS-latest` ships. One pinned Xcode 15 configure, build
and test run is required before this floor is published, and recording in this
row the exact Xcode and Apple Clang version it passed with is what turns the
row from declared into verified. Should that run not pass, the floor is raised
to the oldest Xcode that does pass — in this matrix, in `README.md` and in the
guard together.

The last row is a gate rather than a result. No depends cross host and no Guix
triple has been built at this dialect, so the Boost, OpenSSL, ZeroMQ and
protobuf versions in that row are the versions `contrib/depends` pins and not
versions this dialect has been compiled against. The gates are a `depends.yml`
run across all ten cross hosts — among them the three whose pinned libc++
predates C++23, the two Apple Darwin targets and FreeBSD — and a `guix.yml` run
on the candidate, repeated to confirm that the release binaries still
reproduce. Those runs are the pre-acceptance gates themselves, and no
in-repository remediation for them is recorded: until both pass, this row
claims nothing beyond the pins.

One property of those release paths is known in advance and is recorded here
rather than resolved in the tree, because resolving it would change a pinned
recipe. Every *target* C++ recipe receives the host `CXXFLAGS`, so under this
dialect the pinned protobuf-cpp 3.21.12 is compiled as C++23 for the first
time, and its own headers and generated code combine two enumeration types
with `|` — the `field_layout` constants in
`google/protobuf/generated_message_tctable_impl.h`, such as
`kBool = kFkVarint | kRep8Bits`. C++20 deprecated that operation, so GCC and
Clang emit `-Wdeprecated-enum-enum-conversion` where C++17 emitted nothing.
The measured scope, from the release-path acceptance run, is about 105 new
diagnostics per host on each of the ten depends cross hosts and each of the
eight Guix triples, with zero at C++17; a syntax-only compile of the pinned
protobuf-cpp 3.21.12 library sources with GCC 14.3.0 reproduces that count from
35 distinct lines of that single upstream header.

The diagnostics are third-party by origin — they are emitted from protobuf's
own headers and generated code, not from any first-party translation unit — so
they add no first-party warning origin and no first-party trace location, and
the no-new-warnings criterion for the acceptance configurations above is
unaffected. That is the same treatment the Boost 1.83 Beast deprecation gets
under "Libraries", and nothing is suppressed for either: no `-Wno-*` flag and
no diagnostic pragma is added anywhere, and the pinned protobuf version stays
as it is, because a recipe version is a reproducible-build input. Two
remediations are available to the maintainers, and choosing either one is a
decision taken outside a language-standard migration: upgrade the protobuf
recipe to an upstream release whose generated code and headers are C++20-clean,
which changes the depends inputs and invalidates every depends cache; or give
that one recipe a dialect exception through its
`$(package)_cxxflags_$(host_os)` hook — the mechanism
`contrib/depends/packages/zeromq.mk` already uses for `mingw32` — leaving the
first-party dialect at 23. An exception applied that way is recorded in this
section, so that the dialect a release path actually compiles a recipe with is
never implicit.

Unknown compiler IDs and the `clang-cl` frontend are rejected at configure
time, because this repository has no MSVC build path: Windows is built with
MinGW-w64 GCC through MSYS2 UCRT64. The guard branches on
`CMAKE_CXX_COMPILER_ID` and, for `clang-cl`, on
`CMAKE_CXX_COMPILER_FRONTEND_VARIANT`.

The guard checks compiler family, frontend variant and version, but it does
not inspect the standard library. It therefore cannot detect the Clang 16 with
libstdc++ 14 pairing defect, which is a header-library mismatch rather than a
compiler version, and that is exactly why the matrix carries that row:
such a failure surfaces only when the affected translation units are compiled,
so this table is the only place it is documented.

### Libraries

Boost 1.69 is the declared floor, unchanged by the move to C++23; 1.83 and
1.91.0 are the versions verified under C++23. Clang 18 or newer with libstdc++
14 or newer must use Boost 1.84 or newer for a warning-clean build. With Boost
1.83 that combination builds and passes its tests, but Beast's use of the
deprecated `std::aligned_storage` produces one Boost-internal warning origin
(eight diagnostics in the measured full build), reached
through the tree's single Beast consumer,
`tests/unit_tests/epee_http_server.cpp`; Boost 1.84.0 replaced that use
upstream, so releases from 1.84 onwards do not carry it. Boost 1.69 through
1.82 were not built or tested at this dialect in any pairing, so nothing is
claimed for them here beyond the declared floor. Nothing is suppressed for any
of this — no `-Wno-*` flag and no diagnostic pragma is added anywhere — the
resolution is the newer Boost.

OpenSSL 1.1.1 is the declared floor and is likewise unchanged: it is a C API
consumed through `extern "C"`, so the C++ dialect cannot move it. 3.0.13 is the
version verified against, and 3.5.7 is the version pinned in `contrib/depends`.

Raising either declared floor is a separate maintainers' decision, not a
consequence of the move to C++23.

### Rust

Rust and `cargo` are mandatory on `master` — `src/fcmp_pp/fcmp_pp_rust` is
built unconditionally — so both must be on `PATH` before CMake is configured.
`src/fcmp_pp/fcmp_pp_rust/Cargo.toml` declares no `rust-version`, so the
repository states no minimum supported Rust version; 1.93, installed through
rustup, is the toolchain CI tests with.

## Use cases

### Test Driven Development (TDD) - shared libraries for release builds

Building shared libraries spares a lot of disk space and linkage time. By default only the debug builds produce shared libraries. If you'd like to produce dynamic libraries for the release build for the same reasons as it's being done for the debug version, then you need to add the `BUILD_SHARED_LIBS=ON` flag to the `CMake` call, like the following:

`cmake -S "$DIR_SRC" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON && make`

A perfect use case for the above call is following the Test Driven Development (TDD) principles. In a nutshell, you'd first write a couple of tests, which describe the (new) requirements of the class/method that you're about to write or modify. The tests will typically compile for quite a long time, so ideally write them once. After you're done with the tests, the only thing left to do is to keep modifying the implementation for as long as the tests are failing. If the implementation is contained properly within a .cpp file, then the only time cost to be paid will be compiling the single source file and generating the implementation's shared library. The test itself will not have to be touched and will pick up the new version of the implementation (via the shared library) upon the next execution of the test.

### Project generation for IDEs

CMake allows to generate project files for many IDEs. The list of supported project files can be obtained by writing in the console:

`cmake -G`

For instance, in order to generate Makefiles and project files for the Code::Blocks IDE, this part of the call would look like the following:

`cmake -G "CodeBlocks - Unix Makefiles" (...)`

The additional artifact of the above call is the `monero.cbp` Code::Blocks project file in the build directory.

### Debugging in Code::Blocks (CB)

First prepare the build directory for debugging using the following example command, assuming, that the path to the source dir is being held in the DIR_SRC variable, and using 2 cores:

`cmake -S "$DIR_SRC" -G "CodeBlocks - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON && make -j 2`

After a successful build, open the `monero.cbp` with CB. From the CB's menu bar select the target, that you want debug. Assuming these are unit tests:

`Build -> Select target -> Select target -> unit_tests`

In order to lower the turnaround times, we will run a specific portion of code of interest, without having to go through all the time costly initialization and execution of unrelated parts. For this we'll use GTest's capabilities of test filtering. From the build directory run the following command to learn all the registered tests:

`tests/unit_tests/unit_tests --gtest_list_tests`

For example, if you're only interested in logging, you'd find in the list the label `logging.` and its subtests. To execute all the logging tests, you'd write in the console:

`tests/unit_tests/unit_tests --gtest_filter="logging.*"`

This parameter is what we need to transfer to CB, in order to reflect the same behaviour in the CB's debugger. From the main menu select:

`Project -> Set program's arguments...`

Then in the `Program's arguments` textbox you'd write in this case: 

`--gtest_filter="logging.*"`

Verify if the expected UTs are being properly executed with `F9` or select:

`Build -> Build and run`

If everything looks fine, then after setting some breakpoints of your choice, the target is ready for debugging in CB via:

`Debug -> Start/Continue`

## To be done (and merged):
### Multihost parallel compilation
https://github.com/monero-project/monero/pull/7160

### Unity builds
https://github.com/monero-project/monero/pull/7217

