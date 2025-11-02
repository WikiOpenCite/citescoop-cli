# SPDX-FileCopyrightText: 2025 The University of St Andrews
# SPDX-License-Identifier: GPL-3.0-or-later

execute_process(
    COMMAND git log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE GIT_SHA
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

configure_file(cmake/version.h.in configured/citescoop/version.h)
