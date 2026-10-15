# SPDX-License-Identifier: LGPL-3.0-only
# Copyright (C) 2026 Jorge Pereira
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Build, package, and capture the macOS application.

.DEFAULT_GOAL := help
.PHONY: help osx osx-dmg screenshots clean

BUILD_DIR ?= build
APP_NAME := getright-port
APP_BUNDLE := $(BUILD_DIR)/$(APP_NAME).app
DMG_PATH := $(BUILD_DIR)/$(APP_NAME)-macos-arm64.dmg
MACOS_DEPLOYMENT_TARGET ?= 15.0
QT_PREFIX ?= $(shell brew --prefix qt 2>/dev/null)
QT_PLUGIN_DIR ?= $(shell "$(QT_PREFIX)/bin/qtpaths" --query QT_INSTALL_PLUGINS)

help:
	@printf '%s\n' \
	  'Targets:' \
	  '  make          Show this help' \
	  '  make osx      Build the macOS application' \
	  '  make osx-dmg  Create an installation DMG' \
	  '  make screenshots  Capture the interface locally' \
	  '  make clean    Remove build artifacts'

osx:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
	  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
	  -DCMAKE_OSX_DEPLOYMENT_TARGET="$(MACOS_DEPLOYMENT_TARGET)"
	cmake --build $(BUILD_DIR) --config Release

osx-dmg:
	rm -rf "$(APP_BUNDLE)"
	$(MAKE) osx
	@command -v hdiutil >/dev/null || \
	  { echo 'hdiutil is unavailable; this target requires macOS.' >&2; exit 1; }
	"$(QT_PREFIX)/bin/macdeployqt" "$(APP_BUNDLE)" -no-plugins -no-codesign
	@mkdir -p "$(APP_BUNDLE)/Contents/PlugIns/platforms" \
	  "$(APP_BUNDLE)/Contents/PlugIns/styles" "$(APP_BUNDLE)/Contents/PlugIns/tls"
	cp "$(QT_PLUGIN_DIR)/platforms/libqcocoa.dylib" \
	  "$(QT_PLUGIN_DIR)/platforms/libqoffscreen.dylib" \
	  "$(APP_BUNDLE)/Contents/PlugIns/platforms/"
	cp "$(QT_PLUGIN_DIR)/styles/libqmacstyle.dylib" \
	  "$(APP_BUNDLE)/Contents/PlugIns/styles/"
	cp "$(QT_PLUGIN_DIR)/tls/libqsecuretransportbackend.dylib" \
	  "$(QT_PLUGIN_DIR)/tls/libqcertonlybackend.dylib" \
	  "$(APP_BUNDLE)/Contents/PlugIns/tls/"
	@for plugin in "$(APP_BUNDLE)"/Contents/PlugIns/*/*.dylib; do \
	  install_name_tool -add_rpath @loader_path/../../Frameworks "$$plugin"; \
	done
	python3 scripts/deploy-macos.py "$(APP_BUNDLE)"
	cp LICENSE.md THIRD_PARTY_NOTICES.md "$(APP_BUNDLE)/Contents/Resources/"
	codesign --force --deep --sign - "$(APP_BUNDLE)"
	rm -f "$(DMG_PATH)"
	hdiutil create -volname "GetRight Port" -srcfolder "$(APP_BUNDLE)" \
	  -ov -format UDZO "$(DMG_PATH)"
	@echo "DMG created: $(DMG_PATH)"

screenshots: osx
	@mkdir -p screenshots
	QT_QPA_PLATFORM=offscreen \
	  "$(BUILD_DIR)/$(APP_NAME).app/Contents/MacOS/$(APP_NAME)" \
	  --screenshot screenshots/getright.png

clean:
	rm -rf "$(BUILD_DIR)"
