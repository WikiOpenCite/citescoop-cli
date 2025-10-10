# SPDX-FileCopyrightText: 2025 https://github.com/friendlyanon
# SPDX-License-Identifier: Unlicense

# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(citescoop-cliTests LANGUAGES CXX)

# ---- Dependencies ----

find_package(Catch2 REQUIRED)
include(Catch)

# ---- Tests ----

add_executable(citescoop-cli_test src/citescoop-cli_test.cc)
target_link_libraries(
    citescoop-cli_test PRIVATE
    citescoop-cli_lib
    Catch2::Catch2WithMain
)
target_compile_features(citescoop-cli_test PRIVATE cxx_std_20)

catch_discover_tests(citescoop-cli_test)

# ---- End-of-file commands ----

add_folders(Test)
