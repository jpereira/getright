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
PACKAGE_BUNDLE := $(BUILD_DIR)/package/$(APP_NAME).app
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
	@if test -f "$(APP_BUNDLE)/Contents/Resources/qt.conf"; then \
	  rm -rf "$(APP_BUNDLE)/Contents/Frameworks" "$(APP_BUNDLE)/Contents/PlugIns"; \
	  rm -f "$(APP_BUNDLE)/Contents/Resources/qt.conf"; \
	fi
	cmake -S . -B $(BUILD_DIR) -DCMAKE_PREFIX_PATH="$(QT_PREFIX)" \
	  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64 \
	  -DCMAKE_OSX_DEPLOYMENT_TARGET="$(MACOS_DEPLOYMENT_TARGET)"
	cmake --build $(BUILD_DIR) --config Release

osx-dmg:
	$(MAKE) osx
	rm -rf "$(PACKAGE_BUNDLE)"
	mkdir -p "$(BUILD_DIR)/package"
	ditto "$(APP_BUNDLE)" "$(PACKAGE_BUNDLE)"
	@command -v hdiutil >/dev/null || \
	  { echo 'hdiutil is unavailable; this target requires macOS.' >&2; exit 1; }
	"$(QT_PREFIX)/bin/macdeployqt" "$(PACKAGE_BUNDLE)" -no-plugins -no-codesign
	@mkdir -p "$(PACKAGE_BUNDLE)/Contents/PlugIns/platforms" \
	  "$(PACKAGE_BUNDLE)/Contents/PlugIns/styles" "$(PACKAGE_BUNDLE)/Contents/PlugIns/tls"
	cp "$(QT_PLUGIN_DIR)/platforms/libqcocoa.dylib" \
	  "$(QT_PLUGIN_DIR)/platforms/libqoffscreen.dylib" \
	  "$(PACKAGE_BUNDLE)/Contents/PlugIns/platforms/"
	cp "$(QT_PLUGIN_DIR)/styles/libqmacstyle.dylib" \
	  "$(PACKAGE_BUNDLE)/Contents/PlugIns/styles/"
	cp "$(QT_PLUGIN_DIR)/tls/libqsecuretransportbackend.dylib" \
	  "$(QT_PLUGIN_DIR)/tls/libqcertonlybackend.dylib" \
	  "$(PACKAGE_BUNDLE)/Contents/PlugIns/tls/"
	@for plugin in "$(PACKAGE_BUNDLE)"/Contents/PlugIns/*/*.dylib; do \
	  install_name_tool -add_rpath @loader_path/../../Frameworks "$$plugin"; \
	done
	python3 scripts/deploy-macos.py "$(PACKAGE_BUNDLE)"
	cp LICENSE.md THIRD_PARTY_NOTICES.md "$(PACKAGE_BUNDLE)/Contents/Resources/"
	codesign --force --deep --sign - "$(PACKAGE_BUNDLE)"
	rm -f "$(DMG_PATH)"
	hdiutil create -volname "GetRight Port" -srcfolder "$(PACKAGE_BUNDLE)" \
	  -ov -format UDZO "$(DMG_PATH)"
	@echo "DMG created: $(DMG_PATH)"

screenshots: osx
	@command -v ffmpeg >/dev/null || \
	  { echo 'Install ffmpeg to encode the live download GIF.' >&2; exit 1; }
	git archive --format=zip -o "$(BUILD_DIR)/screenshot-source.zip" HEAD
	"$(BUILD_DIR)/capture-screenshots" "$(BUILD_DIR)/screenshot-source.zip" site/assets
	rm -rf "$(BUILD_DIR)/gif-frames"
	"$(BUILD_DIR)/capture-screenshots" "$(BUILD_DIR)/screenshot-source.zip" \
	  "$(BUILD_DIR)/gif-frames" --animation
	ffmpeg -y -loglevel error -framerate 10 -i "$(BUILD_DIR)/gif-frames/frame-%04d.png" \
	  -filter_complex 'split[a][b];[a]palettegen=max_colors=128[p];[b][p]paletteuse=dither=bayer' \
	  -loop 0 site/assets/port-download.gif

clean:
	rm -rf "$(BUILD_DIR)"
