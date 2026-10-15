# SPDX-License-Identifier: LGPL-3.0-only
# Copyright (C) 2026 Jorge Pereira
# Author: Jorge Pereira <jpereiran@gmail.com>
# Date: 2026-10-14
# Description: Qt application build and macOS bundle configuration.

cmake_minimum_required(VERSION 3.21)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" GETRIGHT_VERSION LIMIT_COUNT 1)
string(REGEX REPLACE "^v" "" GETRIGHT_VERSION "${GETRIGHT_VERSION}")
project(GetRightPort VERSION "${GETRIGHT_VERSION}" LANGUAGES CXX)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS VERSION)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Network)

set(APP_ICON "${CMAKE_CURRENT_BINARY_DIR}/GetRightPort.icns")
add_custom_command(
    OUTPUT "${APP_ICON}"
    COMMAND /bin/sh "${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-icns.sh"
            "${CMAKE_CURRENT_SOURCE_DIR}/resources/getright-logo.png"
            "${APP_ICON}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/getright-logo.png"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/make-icns.sh"
        "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pack-icns.py"
    VERBATIM
)

qt_add_executable(getright-port
    src/main.cpp
    src/main_window.cpp
    src/main_window.h
    resources/getright.qrc
    "${APP_ICON}"
)

set_source_files_properties("${APP_ICON}" PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources
)

target_link_libraries(getright-port PRIVATE Qt6::Widgets Qt6::Network)
target_compile_definitions(getright-port PRIVATE GETRIGHT_VERSION="${PROJECT_VERSION}")

if(APPLE)
    enable_language(OBJCXX)
    target_sources(getright-port PRIVATE src/macos_application.mm)
    target_link_libraries(getright-port PRIVATE "-framework AppKit")
endif()

set_target_properties(getright-port PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER org.personal.getright-port
    MACOSX_BUNDLE_BUNDLE_NAME "GetRight Port"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE "GetRightPort.icns"
)
