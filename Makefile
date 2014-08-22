all: _warn_fast all-fast _warn_fast2

# WARNING: to really build "all" - the all modules in release version, use:
# make all-release

# this is the default thing to run for developers for now. added to be friendly for IDEs/setups that bind "make run"
run: build-fast
	bash start-devel.sh "fast"

# fast: the build that gives fast recompile for core developer (skips modules etc; includes debug still)
cmake-fast:
	mkdir -p build/fast
	( cd build/fast && cmake -D CMAKE_BUILD_TYPE=Debug  -D BUILD_CONN_TOOL=OFF  -D BUILD_SIMPLE_MINER=OFF  -D BUILD_SIMPLE_WALLET=OFF  -D BUILD_TESTS=OFF   -D ENABLE_COTIRE=ON  -D COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES=1  -D COTIRE_VERBOSE=ON  ../.. )

build-fast: cmake-fast
	( cd build/fast && $(MAKE) )

all-fast: build-fast

fast: all-fast


# debug: the debug version
cmake-debug:
	mkdir -p build/debug
	( cd build/debug && cmake -D CMAKE_BUILD_TYPE=Debug ../.. )

build-debug: cmake-debug
	( cd build/debug && $(MAKE) )

test-debug: build-debug
	( cd build/debug && $(MAKE) test )

all-debug: build-debug

# release: the main release:
cmake-release:
	mkdir -p build/release
	( cd build/release && cmake -D CMAKE_BUILD_TYPE=Release ../.. )

build-release: cmake-release
	( cd build/release && $(MAKE) )

test-release: build-release
	( cd build/release && $(MAKE) test )

all-release: build-release


clean:
	@echo "WARNING: Back-up your wallet if it exists within ./build!" ; \
        read -r -p "This will destroy the build directory, continue (y/N)?: " CONTINUE; \
	[ $$CONTINUE = "y" ] || [ $$CONTINUE = "Y" ] || (echo "Exiting."; exit 1;)
	rm -rf build

tags:
	ctags -R --sort=1 --c++-kinds=+p --fields=+iaS --extra=+q --language-force=C++ src contrib tests/gtest


# utils:

_warn_fast:
	@echo ""
	@echo ""
	@echo "==============================================================="
	@echo "This is the FAST build, for developers, and NOT for the release!"
	@echo "Not building most of the module."
	@echo "For the full RELEASE, instead please use: make all-release"
	@echo "==============================================================="
	@echo ""
	@echo ""

_warn_fast2: _warn_fast


.PHONY: run all cmake-debug build-debug test-debug all-debug cmake-release build-release test-release all-release clean tags cmake-fast build-fast all-fast fast _warn_fast _warn_fast2


