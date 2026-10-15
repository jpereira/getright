cmake_minimum_required(VERSION 3.21)
project(GetRightPort VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

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

set_target_properties(getright-port PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER org.personal.getright-port
    MACOSX_BUNDLE_BUNDLE_NAME "GetRight Port"
    MACOSX_BUNDLE_ICON_FILE "GetRightPort.icns"
)
