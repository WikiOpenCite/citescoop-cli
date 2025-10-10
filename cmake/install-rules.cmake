# SPDX-FileCopyrightText: 2025 https://github.com/friendlyanon
# SPDX-License-Identifier: Unlicense

install(
    TARGETS citescoop-cli_exe
    RUNTIME COMPONENT citescoop-cli_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
