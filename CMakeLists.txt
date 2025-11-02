# SPDX-FileCopyrightText: 2025 https://github.com/friendlyanon
# SPDX-License-Identifier: Unlicense

cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    citescoop-cli
    VERSION 0.1.0
    DESCRIPTION "A command line interface to interact with the citescoop library."
    HOMEPAGE_URL "https://github.com/WikiOpenCite/citescoop-cli"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)
include(cmake/configured-files.cmake)

# ---- Declare executable ----

add_executable(citescoop-cli_exe
  src/commands/base_command.cc
  src/commands/extract_command.cc
  src/cli.cc
  src/main.cc
)
add_executable(citescoop-cli::exe ALIAS citescoop-cli_exe)

target_include_directories(
    citescoop-cli_exe
    PRIVATE
    "\$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/configured>"
)

set_property(TARGET citescoop-cli_exe PROPERTY OUTPUT_NAME citescoop-cli)

target_compile_features(citescoop-cli_exe PRIVATE cxx_std_20)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
target_link_libraries(citescoop-cli_exe PRIVATE spdlog::spdlog_header_only Boost::program_options fmt::fmt)

# ---- Developer mode ----

if(NOT citescoop-cli_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of citescoop-cli"
  )
endif()

include(cmake/dev-mode.cmake)
