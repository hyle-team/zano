# Define CMake generator
system := $(shell uname)
ifneq (, $(findstring MINGW, $(system)))
  cmake_gen = -G 'MSYS Makefiles'
endif

cmake = cmake $(cmake_gen)

cmake_debug = $(cmake) -D CMAKE_BUILD_TYPE=Debug
cmake_release = $(cmake) -D CMAKE_BUILD_TYPE=Release
cmake_tests = $(cmake) -D BUILD_TESTS=ON

# Helper macro
define CMAKE
  mkdir -p $1 && cd $1 && $2 ../../
endef

build = build
dir_debug = $(build)/debug
dir_release = $(build)/release

all: release

release:
	$(eval command += $(cmake_release))
	$(call CMAKE,$(dir_release),$(command)) && $(MAKE)

test-release:
	$(eval command += $(cmake_release) $(cmake_tests))
	$(call CMAKE,$(dir_release),$(command)) && $(MAKE) && $(MAKE) test

test: test-release

debug:
	$(eval command += $(cmake_debug))
	$(call CMAKE,$(dir_debug),$(command)) && $(MAKE)

test-debug:
	$(eval command += $(cmake_debug) $(cmake_tests))
	$(call CMAKE,$(dir_debug),$(command)) && $(MAKE) && $(MAKE) test

clean:
	rm -rf build

tags:
	ctags -R --sort=1 --c++-kinds=+p --fields=+iaS --extra=+q --language-force=C++ src contrib tests/gtest

.PHONY: all release test-release test all-debug debug test-debug clean tags
