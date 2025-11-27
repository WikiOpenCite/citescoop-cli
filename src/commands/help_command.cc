// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "help_command.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"

#include "base_command.h"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {
HelpCommand::HelpCommand(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::shared_ptr<std::vector<std::shared_ptr<BaseCommand>>> commands)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("help", "Show help information for a given command") {

  commands_ = std::move(commands);
  cli_options_.add_options()("command",
                             options::value<std::string>()->required(),
                             "Command to show help text for");
  positional_options_.add("command", 1);
}

int HelpCommand::Run(
    std::vector<std::string> args,  // NOLINT(whitespace/indent_namespace)
    // NOLINTNEXTLINE(misc-unused-parameters,whitespace/indent_namespace)
    struct GlobalOptions globals) {
  auto parsed_args = ParseArgs(args);

  auto command_name = parsed_args.first["command"].as<std::string>();

  for (const auto& command : *commands_) {
    if (command->name() == command_name) {
      command->PrintHelp();
      return EXIT_SUCCESS;
    }
  }

  std::cout << "No command with name: " << command_name << '\n';

  return EXIT_FAILURE;
}

}  // namespace wikiopencite::citescoop::cli
