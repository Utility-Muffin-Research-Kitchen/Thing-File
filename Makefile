SHELL := /bin/bash

CXX ?= c++
BUILD ?= build
WORKSPACE_ROOT ?= $(abspath ..)
CATASTROPHE_DIR ?= $(WORKSPACE_ROOT)/Catastrophe
MLP1_TOOLCHAIN_IMAGE ?= ghcr.io/utility-muffin-research-kitchen/mlp1-toolchain:local
JAWAKA_SDCARD_ROOT ?= $(WORKSPACE_ROOT)/Jawaka/mock-sdcard
SDCARD_PATH ?= $(JAWAKA_SDCARD_ROOT)
UMRK_PLATFORM_PATH ?= $(SDCARD_PATH)/.system/leaf/platforms/mac
APPS_PATH ?= $(SDCARD_PATH)/Apps
MLP1_REMOTE_SDCARD_PATH ?= /mnt/sdcard
MLP1_SYSTEM_PATH ?= $(MLP1_REMOTE_SDCARD_PATH)/.system/leaf/platforms/mlp1
MLP1_APPS_PATH ?= $(MLP1_REMOTE_SDCARD_PATH)/Apps

APP_NAME := Thing-File
APP_BIN_NAME := thing-file
PACKAGE_NAME := Thing-File.pak
PACKAGE_ROOT := $(BUILD)/package
PACKAGE_DIR := $(PACKAGE_ROOT)/$(PACKAGE_NAME)
OUTDIR := $(BUILD)/obj
EXECUTABLE := $(BUILD)/bin/$(APP_BIN_NAME)

CXXSTD := -std=c++17
CWARN := -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare
CXXOPT ?= -Os
CXXFLAGS_PLATFORM ?=

SDL := SDL2
SDL_CFLAGS := $(shell pkg-config --cflags sdl2 SDL2_image SDL2_ttf)
SDL_LDFLAGS := $(shell pkg-config --libs sdl2 SDL2_image SDL2_ttf)

CXXFLAGS_COMMON := $(CXXSTD) $(CWARN) $(CXXOPT) $(CXXFLAGS_PLATFORM) $(SDL_CFLAGS) -DUSE_SDL2 -DAPP_NAME=\"$(APP_NAME)\" -DRES_DIR=\"res/\"
LINKFLAGS_COMMON := $(SDL_LDFLAGS)
ifeq ($(shell uname -s),Darwin)
LINKFLAGS_COMMON += -framework CoreFoundation
endif

FONTS_NATIVE := {"$(CATASTROPHE_DIR)/res/font.ttf",8},{"SourceCodePro-Semibold.ttf",8},{"SourceCodePro-Regular.ttf",8}
FONTS_MLP1 := {"$(MLP1_APPS_PATH)/$(PACKAGE_NAME)/res/font.ttf",8},{"SourceCodePro-Semibold.ttf",8},{"SourceCodePro-Regular.ttf",8}

ifeq ($(PLATFORM),mlp1)
PLATFORM_ID := mlp1
PATH_DEFAULT := $(MLP1_REMOTE_SDCARD_PATH)
PATH_DEFAULT_RIGHT := $(MLP1_SYSTEM_PATH)
FILE_SYSTEM := /dev/mmcblk1p1
SCREEN_WIDTH := 480
SCREEN_HEIGHT := 360
PPU_X := 2
PPU_Y := 2
CXXFLAGS_COMMON += -DPLATFORM_MLP1 -DFONTS='$(FONTS_MLP1)'
else
PLATFORM_ID := mac
PATH_DEFAULT := $(SDCARD_PATH)
PATH_DEFAULT_RIGHT := $(UMRK_PLATFORM_PATH)
FILE_SYSTEM := /
SCREEN_WIDTH := 640
SCREEN_HEIGHT := 480
PPU_X := 1
PPU_Y := 1
CXXFLAGS_COMMON += -DFONTS='$(FONTS_NATIVE)'
endif

CXXFLAGS_COMMON += \
	-DPATH_DEFAULT=\"$(PATH_DEFAULT)\" \
	-DPATH_DEFAULT_RIGHT=\"$(PATH_DEFAULT_RIGHT)\" \
	-DFILE_SYSTEM=\"$(FILE_SYSTEM)\" \
	-DSCREEN_WIDTH=$(SCREEN_WIDTH) \
	-DSCREEN_HEIGHT=$(SCREEN_HEIGHT) \
	-DPPU_X=$(PPU_X) \
	-DPPU_Y=$(PPU_Y) \
	-DSCREEN_BPP=32 \
	-DAUTOSCALE=1 \
	-DAUTOSCALE_DPI=0

OBJS := \
	main.o commander.o config.o dialog.o fileLister.o fileutils.o keyboard.o panel.o resourceManager.o \
	screen.o sdl_ttf_multifont.o sdlutils.o text_edit.o utf8.o text_viewer.o image_viewer.o window.o \
	umrk_input.o \
	sdl_gfx/SDL2_rotozoom.o

DEPFILES := $(patsubst %.o,$(OUTDIR)/%.d,$(OBJS))

.PHONY: all native run-native package package-native package-build package-mlp1 mlp1 install-jawaka-app adb-stage-pak-mlp1 clean check-sdl

all: native

native: $(EXECUTABLE)

check-sdl:
	@pkg-config --exists sdl2 SDL2_image SDL2_ttf 2>/dev/null || \
		( echo "SDL2 libraries not found. Install with: brew install sdl2 sdl2_image sdl2_ttf" && exit 1 )

$(BUILD)/bin:
	@mkdir -p "$@"

$(EXECUTABLE): $(addprefix $(OUTDIR)/,$(OBJS)) | $(BUILD)/bin
	$(CXX) -o "$@" $^ $(LINKFLAGS_COMMON)

$(OUTDIR)/%.o: src/%.cpp | check-sdl
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS_COMMON) -MP -MMD -MF "$(@:%.o=%.d)" -c "$<" -o "$@"

$(OUTDIR)/%.o: src/%.c | check-sdl
	@mkdir -p "$(@D)"
	$(CXX) $(CXXFLAGS_COMMON) -MP -MMD -MF "$(@:%.o=%.d)" -c "$<" -o "$@"

run-native: native
	SDCARD_PATH="$(SDCARD_PATH)" \
	UMRK_PLATFORM_PATH="$(UMRK_PLATFORM_PATH)" \
	APPS_PATH="$(APPS_PATH)" \
	JAWAKA_SDCARD_ROOT="$(JAWAKA_SDCARD_ROOT)" \
	"$(EXECUTABLE)" --res-dir "$(CURDIR)/res"

package package-native: native
	$(MAKE) BUILD="$(BUILD)" PLATFORM="$(PLATFORM)" package-build

package-build:
	@rm -rf "$(PACKAGE_ROOT)"
	@mkdir -p "$(PACKAGE_DIR)/bin" "$(PACKAGE_DIR)/res"
	@cp -f "$(EXECUTABLE)" "$(PACKAGE_DIR)/bin/$(APP_BIN_NAME)"
	@cp -Rf res/. "$(PACKAGE_DIR)/res/"
	@if [ -f "$(CATASTROPHE_DIR)/res/font.ttf" ]; then cp -f "$(CATASTROPHE_DIR)/res/font.ttf" "$(PACKAGE_DIR)/res/font.ttf"; fi
	@cp -f "pak/launch.sh" "$(PACKAGE_DIR)/launch.sh"
	@printf '{ "name": "Thing-File", "icon": "", "platform": "$(PLATFORM_ID)", "pak_version": "0.1.0", "min_jawaka_version": "0.0.1" }\n' > "$(PACKAGE_DIR)/pak.json"
	@chmod 755 "$(PACKAGE_DIR)/launch.sh" "$(PACKAGE_DIR)/bin/$(APP_BIN_NAME)"
	@find "$(PACKAGE_DIR)" -maxdepth 3 -type f -print | sort

mlp1:
	docker run --rm \
		-v "$(WORKSPACE_ROOT)":/workspace \
		-w /workspace/Thing-File \
		"$(MLP1_TOOLCHAIN_IMAGE)" \
		make PLATFORM=mlp1 BUILD=build/mlp1 native

package-mlp1: mlp1
	$(MAKE) PLATFORM=mlp1 BUILD=build/mlp1 package-build

install-jawaka-app: package-native
	@mkdir -p "$(APPS_PATH)"
	@rm -rf "$(APPS_PATH)/$(PACKAGE_NAME)"
	@cp -R "$(PACKAGE_DIR)" "$(APPS_PATH)/$(PACKAGE_NAME)"
	@echo "Installed $(PACKAGE_NAME) to $(APPS_PATH)"

adb-stage-pak-mlp1: package-mlp1
	scripts/adb-stage-pak.sh

clean:
	rm -rf "$(BUILD)"

-include $(DEPFILES)
