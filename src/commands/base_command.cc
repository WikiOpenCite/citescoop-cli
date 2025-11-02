// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "base_command.h"

#include <iostream>
#include <string>
#include <utility>

#include "citescoop/version.h"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {
BaseCommand::BaseCommand(std::string name, std::string description)
    : cli_options_(name + " options") {
  name_ = std::move(name);
  description_ = std::move(description);
}

void BaseCommand::PrintHelp() {
  namespace cmake = wikiopencite::citescoop::cmake;
  std::cout << fmt::format("Usage: {} [global options] {} [<args>]",
                           cmake::project_name, name_)
            << '\n';

  std::cout << cli_options_;
}

std::pair<options::variables_map, options::parsed_options>
BaseCommand::ParseArgs(std::vector<std::string> args) {
  const options::parsed_options parsed =
      options::command_line_parser(args).options(cli_options_).run();

  options::variables_map cli_variables;
  options::store(parsed, cli_variables);
  options::notify(cli_variables);

  return std::pair<options::variables_map, options::parsed_options>(
      cli_variables, parsed);
}
}  // namespace wikiopencite::citescoop::cli
