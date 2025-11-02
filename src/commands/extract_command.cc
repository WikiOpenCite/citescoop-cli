// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extract_command.h"

#include <iostream>
#include <string>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {
ExtractCommand::ExtractCommand()
    : BaseCommand("extract", "Extract citations from") {
  cli_options_.add_options()("dump-root",
                             options::value<std::string>()->required(),
                             "Root directory for latest data dump");
}

int ExtractCommand::Run(std::vector<std::string> args,
                        struct GlobalOptions globals) {
  auto parsed_args = ParseArgs(args);

  std::cout << parsed_args.first["dump-root"].as<std::string>() << '\n';
  return EXIT_SUCCESS;
}

}  // namespace wikiopencite::citescoop::cli
