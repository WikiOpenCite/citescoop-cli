// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include "cli.h"
#include "commands/base_command.h"
#include "commands/extract_command.h"
#include "commands/help_command.h"

namespace cli = wikiopencite::citescoop::cli;

namespace {
void SetDebug() {
  spdlog::set_level(spdlog::level::off);

  auto* debug_env = std::getenv("DEBUG");
  if (debug_env != nullptr) {
    auto debug_str = std::string(debug_env);
    std::ranges::transform(debug_str, debug_str.begin(),
                           [](unsigned char chr) { return std::tolower(chr); });
    std::istringstream iss(debug_env);
    bool debug;
    iss >> std::boolalpha >> debug;

    if (debug) {
      spdlog::set_level(spdlog::level::trace);
    }
  }
}
}  // namespace

auto main(int argc, char** argv) -> int {
  SetDebug();

  cli::Cli cli = cli::Cli();

  auto commands =
      std::make_shared<std::vector<std::shared_ptr<cli::BaseCommand>>>();

  auto command = std::shared_ptr<cli::BaseCommand>(new cli::ExtractCommand());
  commands->push_back(command);
  cli.Register(command);

  command = std::shared_ptr<cli::BaseCommand>(new cli::HelpCommand(commands));
  commands->push_back(command);
  cli.Register(command);

  return cli.Run(argc, argv);
}
