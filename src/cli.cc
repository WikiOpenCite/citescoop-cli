// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/detail/parsers.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "fmt/format.h"
#include "spdlog/common.h"
#include "spdlog/spdlog.h"

#include "citescoop/version.h"
#include "commands/base_command.h"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {
Cli::Cli() : global_options_("Global options") {
  global_options_.add_options()("help", "Show global help message")(
      "version", "Show citescoop version")(
      "log-level", options::value<std::string>()->default_value("off"),
      "Logging level")("command", options::value<std::string>(),
                       "command to execute")(
      "subargs", options::value<std::vector<std::string>>(),
      "Arguments for command");

  positional_options_.add("command", 1).add("subargs", -1);
}

void Cli::Register(std::unique_ptr<BaseCommand> command) {
  spdlog::trace("Registering command {}", command->name());
  this->commands_.insert({command->name(), std::move(command)});
}

int Cli::Run(int argc,
             char* argv[]) {  // NOLINT(modernize-avoid-c-arrays)
  spdlog::trace("Got request to run command");

  auto parsed_global_args = ParseGlobalArgs(argc, argv);
  auto global_args = GlobalArgsToStruct(parsed_global_args.first);
  spdlog::set_level(global_args.log_level);

  if (HandleImmediateExecutionFlags(parsed_global_args.first))
    return EXIT_SUCCESS;

  if (parsed_global_args.first["command"].empty()) {
    spdlog::critical("No command passed");
    std::cout << "Missing required argument command" << '\n';
    return static_cast<int>(ExitCode::kCliArgsError);
  }

  std::string cmd = parsed_global_args.first["command"].as<std::string>();
  if (!commands_.contains(cmd)) {
    spdlog::critical("Command {} not found", cmd);
    return static_cast<int>(ExitCode::kCliArgsError);
  }

  auto command_args = options::collect_unrecognized(
      parsed_global_args.second.options, options::include_positional);

  // BUG(computroniks): Potential for this to cause issues if the
  // positional argument isn't the first unrecognized command.
  command_args.erase(command_args.begin());

  spdlog::debug("Running command {}", cmd);
  auto r_code = commands_.at(cmd)->Run(command_args, global_args);
  spdlog::debug("Command exited with code {}", r_code);
  return r_code;
}

void Cli::PrintVersion() {
  namespace cmake = wikiopencite::citescoop::cmake;
  std::cout << fmt::format("{} v{} ({})", cmake::project_name,
                           cmake::project_version, cmake::git_sha)
            << '\n';
}

GlobalOptions Cli::GlobalArgsToStruct(
    const boost::program_options::variables_map& args) {

  GlobalOptions options;
  options.log_level =
      spdlog::level::from_str(args["log-level"].as<std::string>());
  return options;
}

std::pair<options::variables_map, options::parsed_options> Cli::ParseGlobalArgs(
    int argc, char* argv[]) {  //NOLINT (modernize-avoid-c-arrays)

  const options::parsed_options parsed =
      options::command_line_parser(argc, argv)
          .options(global_options_)
          .positional(positional_options_)
          .allow_unregistered()
          .run();

  options::variables_map cli_variables;
  options::store(parsed, cli_variables);
  options::notify(cli_variables);

  return std::pair<options::variables_map, options::parsed_options>(
      cli_variables, parsed);
}

void Cli::PrintGlobalHelp() {
  namespace cmake = wikiopencite::citescoop::cmake;
  std::cout << fmt::format("Usage: {} [global options] <command> [<args>]",
                           cmake::project_name)
            << '\n';

  // BUG(computroniks) This will also print out --command and --subargs
  // which may not be intentional.
  std::cout << global_options_;
  std::cout << '\n' << "Available commands: " << '\n';

  for (auto const& [_, command] : commands_) {
    std::cout << fmt::format("{}\t\t{}", command->name(),
                             command->description())
              << '\n';
  }
}

bool Cli::HandleImmediateExecutionFlags(const options::variables_map& args) {
  if (!args["help"].empty()) {
    PrintGlobalHelp();
    return true;
  }

  if (!args["version"].empty()) {
    PrintVersion();
    return true;
  }
  return false;
}

}  // namespace wikiopencite::citescoop::cli
