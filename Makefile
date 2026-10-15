.DEFAULT_GOAL := help
.PHONY: help osx osx-dmg screenshots clean

BUILD_DIR ?= build
APP_NAME := getright-port
APP_BUNDLE := $(BUILD_DIR)/$(APP_NAME).app
DMG_PATH := $(BUILD_DIR)/$(APP_NAME)-macos-arm64.dmg
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
	cmake -S . -B $(BUILD_DIR) -DCMAKE_PREFIX_PATH="$(QT_PREFIX)"
	cmake --build $(BUILD_DIR) --config Release

osx-dmg:
	rm -rf "$(APP_BUNDLE)"
	$(MAKE) osx
	@command -v hdiutil >/dev/null || \
	  { echo 'hdiutil is unavailable; this target requires macOS.' >&2; exit 1; }
	macdeployqt "$(APP_BUNDLE)" -no-plugins -no-codesign
	@mkdir -p "$(APP_BUNDLE)/Contents/PlugIns/platforms"
	cp "$(QT_PLUGIN_DIR)/platforms/libqcocoa.dylib" \
	  "$(QT_PLUGIN_DIR)/platforms/libqoffscreen.dylib" \
	  "$(APP_BUNDLE)/Contents/PlugIns/platforms/"
	install_name_tool -add_rpath @executable_path/../../Frameworks \
	  "$(APP_BUNDLE)/Contents/PlugIns/platforms/libqcocoa.dylib"
	install_name_tool -add_rpath @executable_path/../../Frameworks \
	  "$(APP_BUNDLE)/Contents/PlugIns/platforms/libqoffscreen.dylib"
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
