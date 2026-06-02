// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "help.h"

#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "spdlog/spdlog.h"

#include "cli.h"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {
Help::Help(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const std::map<std::string, std::shared_ptr<Topic>>& topics)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Topic("help", "Show help information for a given command"),
      // NOLINTNEXTLINE(whitespace/indent_namespace)
      Command("help", "Show help information for a given command"),
      // NOLINTNEXTLINE(whitespace/indent_namespace)
      topics_(topics) {
  cli_options_.add_options()("command", options::value<std::string>(),
                             "Command to show help text for");
  positional_options_.add("command", 1);
}

ExitCode Help::Run(
    std::string topic,              // NOLINT(whitespace/indent_namespace)
    std::vector<std::string> args,  // NOLINT(whitespace/indent_namespace)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    struct GlobalOptions /*globals*/) {
  auto parsed_args = ParseArgs(args);

  if (!topics_.contains(topic)) {
    spdlog::critical("Topic {} not found", topic);
    return ExitCode::kCliArgsError;
  }

  if (parsed_args.first["command"].empty()) {
    topics_.at(topic)->PrintHelp();
  } else {
    auto command = parsed_args.first["command"].as<std::string>();
    topics_.at(topic)->PrintHelp(command);
  }

  return ExitCode::kOk;
}

}  // namespace wikiopencite::citescoop::cli
