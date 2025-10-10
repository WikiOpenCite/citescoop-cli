// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "lib.h"

TEST_CASE("Name is citescoop-cli", "[library]") {
  auto const lib = library{};
  REQUIRE(lib.name == "citescoop-cli");
}
