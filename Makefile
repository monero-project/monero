all: all-release

cmake-debug:
	mkdir -p build/debug
	cd build/debug && cmake -D CMAKE_BUILD_TYPE=Debug ../..

build-debug: cmake-debug
	cd build/debug && $(MAKE)

test-debug: build-debug
	mkdir -p build/debug
	cd build/debug && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Debug ../.. && $(MAKE) test

all-debug:
	mkdir -p build/debug
	cd build/debug && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=Debug ../.. && $(MAKE)

cmake-release:
	mkdir -p build/release
	cd build/release && cmake -D CMAKE_BUILD_TYPE=Release ../..

build-release: cmake-release
	cd build/release && $(MAKE)

test-release: build-release
	mkdir -p build/release
	cd build/release && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=release ../.. && $(MAKE) test

all-release:
	mkdir -p build/release
	cd build/release && cmake -D BUILD_TESTS=ON -D CMAKE_BUILD_TYPE=release ../.. && $(MAKE)

release-static:
	mkdir -p build/release
	cd build/release && cmake -D STATIC=ON -D ARCH="x86-64" -D CMAKE_BUILD_TYPE=release ../.. && $(MAKE)

clean:
	@echo "WARNING: Back-up your wallet if it exists within ./build!" ; \
        read -r -p "This will destroy the build directory, continue (y/N)?: " CONTINUE; \
	[ $$CONTINUE = "y" ] || [ $$CONTINUE = "Y" ] || (echo "Exiting."; exit 1;)
	rm -rf build

tags:
	ctags -R --sort=1 --c++-kinds=+p --fields=+iaS --extra=+q --language-force=C++ src contrib tests/gtest

.PHONY: all cmake-debug build-debug test-debug all-debug cmake-release build-release test-release all-release clean tags
