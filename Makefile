.PHONY: all configure build test clean run install uninstall

BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release
CMAKE ?= cmake
CTEST ?= ctest
CMAKE_GENERATOR_ARGS :=

HOMEBREW_CLANG := $(firstword $(wildcard /opt/homebrew/opt/llvm/bin/clang++ /usr/local/opt/llvm/bin/clang++))
MSYS2_CLANG := $(firstword $(wildcard /clang64/bin/clang++.exe))

ifneq ($(MSYS2_CLANG),)
export PATH := /clang64/bin:$(PATH)
CMAKE := /usr/bin/cmake
CTEST := /usr/bin/ctest
CMAKE_GENERATOR_ARGS := -DCMAKE_MAKE_PROGRAM=/usr/bin/ninja -DCMAKE_CXX_STDLIB_MODULES_JSON=/clang64/lib/libc++.modules.json
INSTALL_PREFIX ?= /usr
else
INSTALL_PREFIX ?= /usr/local
endif

ifeq ($(origin CXX), default)
ifneq ($(HOMEBREW_CLANG),)
CXX := $(HOMEBREW_CLANG)
else ifneq ($(MSYS2_CLANG),)
CXX := /clang64/bin/clang++
else
CXX := clang++
endif
endif

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -G Ninja $(CMAKE_GENERATOR_ARGS) -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)"

build: configure
	$(CMAKE) --build $(BUILD_DIR)

test: build
	$(CTEST) --test-dir $(BUILD_DIR) --output-on-failure

run: build
	$(BUILD_DIR)/md-archive $(ARGS)

install: build
	$(CMAKE) --install $(BUILD_DIR)

uninstall: configure
	$(CMAKE) --build $(BUILD_DIR) --target uninstall

clean:
	rm -rf $(BUILD_DIR)
