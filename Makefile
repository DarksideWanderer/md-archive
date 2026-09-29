.PHONY: all configure build test clean run install uninstall

CMAKE_BUILD_TYPE ?= Release
CMAKE ?= cmake
CTEST ?= ctest
CMAKE_GENERATOR_ARGS :=

HOMEBREW_CLANG := $(firstword $(wildcard /opt/homebrew/opt/llvm/bin/clang++ /usr/local/opt/llvm/bin/clang++))
MSYS2_UCRT64_CLANG := $(firstword $(wildcard /ucrt64/bin/clang++.exe))

ifneq ($(MSYS2_UCRT64_CLANG),)
MSYS2_UCRT64_PREFIX := $(shell cygpath -m /ucrt64)
export PATH := /ucrt64/bin:$(PATH)
CMAKE := $(MSYS2_UCRT64_PREFIX)/bin/cmake.exe
CTEST := $(MSYS2_UCRT64_PREFIX)/bin/ctest.exe
CMAKE_GENERATOR_ARGS := -DCMAKE_MAKE_PROGRAM="$(MSYS2_UCRT64_PREFIX)/bin/ninja.exe" -DCMAKE_CXX_STDLIB_MODULES_JSON="$(MSYS2_UCRT64_PREFIX)/lib/libc++.modules.json" -DCMAKE_CXX_FLAGS="-stdlib=libc++"
BUILD_DIR ?= build/ucrt64
INSTALL_PREFIX ?= $(MSYS2_UCRT64_PREFIX)
else
BUILD_DIR ?= build
INSTALL_PREFIX ?= /usr/local
endif

ifeq ($(origin CXX), default)
ifneq ($(MSYS2_UCRT64_CLANG),)
CXX := $(MSYS2_UCRT64_PREFIX)/bin/clang++.exe
else ifneq ($(HOMEBREW_CLANG),)
CXX := $(HOMEBREW_CLANG)
else
CXX := clang++
endif
endif

all: build

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" -G Ninja $(CMAKE_GENERATOR_ARGS) -DCMAKE_CXX_COMPILER="$(CXX)" -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE) -DCMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)"
	$(CMAKE) -E copy_if_different "$(BUILD_DIR)/compile_commands.json" compile_commands.json

build: configure
	$(CMAKE) --build "$(BUILD_DIR)"

test: build
	$(CTEST) --test-dir "$(BUILD_DIR)" --output-on-failure

run: build
	"$(BUILD_DIR)/md-archive" $(ARGS)

install: build
	$(CMAKE) --install "$(BUILD_DIR)"

uninstall: configure
	$(CMAKE) --build "$(BUILD_DIR)" --target uninstall

clean:
	rm -rf "$(BUILD_DIR)"
