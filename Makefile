.PHONY: all configure build test clean run install uninstall

BUILD_DIR ?= build
CMAKE_BUILD_TYPE ?= Release

HOMEBREW_CLANG := $(firstword $(wildcard /opt/homebrew/opt/llvm/bin/clang++ /usr/local/opt/llvm/bin/clang++))

ifeq ($(origin CXX), default)
ifneq ($(HOMEBREW_CLANG),)
CXX := $(HOMEBREW_CLANG)
else
CXX := clang++
endif
endif

all: build

configure:
	cmake -S . -B $(BUILD_DIR) -G Ninja -DCMAKE_CXX_COMPILER=$(CXX) -DCMAKE_BUILD_TYPE=$(CMAKE_BUILD_TYPE)

build: configure
	cmake --build $(BUILD_DIR)

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

run: build
	$(BUILD_DIR)/md-archive $(ARGS)

install: build
	cmake --install $(BUILD_DIR)

uninstall: configure
	cmake --build $(BUILD_DIR) --target uninstall

clean:
	rm -rf $(BUILD_DIR)
