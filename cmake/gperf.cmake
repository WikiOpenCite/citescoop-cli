# SPDX-FileCopyrightText: 2025 The University of St Andrews
# SPDX-License-Identifier: GPL-3.0-or-later

# Find gperf from vcpkg
find_program(GPERF_EXECUTABLE
    NAMES gperf
    PATHS ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES tools/gperf bin
)
if(NOT GPERF_EXECUTABLE)
    message(FATAL_ERROR "gperf not found. Please install via vcpkg: vcpkg install gperf")
endif()
message(STATUS "Found gperf: ${GPERF_EXECUTABLE}")

# Define the gperf input and output files
set(GPERF_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/languages.gperf)
set(GPERF_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/configured/citescoop/languages.h)

# Custom command to generate the perfect hash header
add_custom_command(
    OUTPUT ${GPERF_OUTPUT}
    COMMAND ${GPERF_EXECUTABLE} ${GPERF_INPUT} > ${GPERF_OUTPUT}
    DEPENDS ${GPERF_INPUT}
    COMMENT "Generating perfect hash for Wikipedia language codes"
    VERBATIM
)

# Add the generated file as a source dependency
add_custom_target(generate_language_hash DEPENDS ${GPERF_OUTPUT})
