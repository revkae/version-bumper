cmake_minimum_required(VERSION 3.16)
project(VersionBumper)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK_PATH OUTPUT_STRIP_TRAILING_WHITESPACE)
  include_directories(SYSTEM "${MACOS_SDK_PATH}/usr/include/c++/v1")
endif()

add_executable(VersionBumper main.cpp mainwindow.cpp savesystem.cpp)
target_link_libraries(VersionBumper Qt6::Core Qt6::Widgets)
