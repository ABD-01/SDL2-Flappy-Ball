# Generic CMake Makefile
# Defaults (can be overridden on command line)
# Used from : https://github.com/ABD-01/.dotfiles/blob/master/utils/cmake-makefile.mk

BUILD_DIR ?= build
SRC_DIR   ?= .
CMAKE_TOOLCHAIN_FILE ?= D:/PROJECTS/ATP/toolchains/clang-msvc/clang_x86_64_msvc_toolchain.cmake

SDL2_INSTALL_DIR ?= D:/PROJECTS/compiler-shenannigans/SDL2-2.30.5/build-msvc/install

CONFIG_STAMP := $(BUILD_DIR)/.cmake_configured

.PHONY: all configure build install clean run

all: build

configure: $(CONFIG_STAMP)

$(CONFIG_STAMP):
	@echo "--- Configuring project with CMake ---"
	@mkdir -p $(BUILD_DIR)
	cmake -S $(SRC_DIR) -B $(BUILD_DIR) \
		-G "MinGW Makefiles" \
		$(if $(CMAKE_TOOLCHAIN_FILE),-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN_FILE)) \
		-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE \
		-DSDL2_DIR=$(SDL2_INSTALL_DIR)/cmake \
		-DCMAKE_INSTALL_PREFIX=$(BUILD_DIR)/install
	@touch $(CONFIG_STAMP)

build: $(CONFIG_STAMP)
	@echo "--- Building project using generated Makefiles ---"
	cmake --build $(BUILD_DIR) --parallel 4 -- --no-print-directory

install: build
	@echo "--- Installing ---"
	cmake --build $(BUILD_DIR) --target install -- --no-print-directory

clean:
	@echo "--- Cleaning build directory ---"
	rm -rf $(BUILD_DIR)
