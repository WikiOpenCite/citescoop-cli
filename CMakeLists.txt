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

# ---- Declare library ----

add_library(
    citescoop-cli_lib OBJECT
    src/lib.cc
)

target_include_directories(
    citescoop-cli_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(citescoop-cli_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(citescoop-cli_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(citescoop-cli_exe src/main.cc)
add_executable(citescoop-cli::exe ALIAS citescoop-cli_exe)

set_property(TARGET citescoop-cli_exe PROPERTY OUTPUT_NAME citescoop-cli)

target_compile_features(citescoop-cli_exe PRIVATE cxx_std_20)

target_link_libraries(citescoop-cli_exe PRIVATE citescoop-cli_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

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
