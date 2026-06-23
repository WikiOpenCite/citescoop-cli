// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli.h"

#include <algorithm>
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

#include "help.h"

namespace options = boost::program_options;

namespace wikiopencite::citescoop::cli {

Command::Command(std::string name, std::string topic, std::string description)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : cli_options_(topic + " " + name + " options") {
  name_ = std::move(name);
  topic_ = std::move(topic);
  description_ = std::move(description);
}

Command::Command(std::string name, std::string description)
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command(std::move(name), "", std::move(description)) {}

void Command::PrintHelp() {
  std::cout << fmt::format("Usage: {} [global options] {} [<args>]",
                           cmake::kProjectName, name_)
            << '\n';

  std::cout << cli_options_;
}

std::pair<options::variables_map, options::parsed_options> Command::ParseArgs(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const std::vector<std::string>& args) {
  // Changes each call.
  // NOLINTNEXTLINE(readability-identifier-naming)
  const options::parsed_options parsed = options::command_line_parser(args)
                                             .options(cli_options_)
                                             .positional(positional_options_)
                                             .run();

  options::variables_map cli_variables;
  options::store(parsed, cli_variables);
  options::notify(cli_variables);

  return std::pair<options::variables_map, options::parsed_options>(
      cli_variables, parsed);
}

Topic::Topic(std::string name, std::string description) {
  name_ = std::move(name);
  description_ = std::move(description);
}

void Topic::Register(std::shared_ptr<Command> command) {
  spdlog::trace("Registering command {} in topic {}", command->name(), name_);
  commands_.insert({command->name(), std::move(command)});
}

ExitCode Topic::Run(std::string command, std::vector<std::string> args,
                    // NOLINTNEXTLINE(whitespace/indent_namespace)
                    GlobalOptions globals) {
  if (!commands_.contains(command)) {
    spdlog::critical("Command {} not found", command);
    return ExitCode::kCliArgsError;
  }

  return commands_.at(command)->Run(std::move(args), globals);
}

void Topic::PrintAvailableCommands() {
  std::cout << '\n' << "Available commands: " << '\n';
  for (auto const& [unused, command] : commands_) {
    std::cout << fmt::format("{}\t\t{}", command->name(),
                             command->description())
              << '\n';
  }
}

void Topic::PrintHelp() {
  std::cout << fmt::format("Usage: {} [global options] {} <command> [<args>]",
                           cmake::kProjectName, name_)
            << '\n';

  PrintAvailableCommands();
}

void Topic::PrintHelp(const std::string& command) {
  if (commands_.contains(command)) {
    commands_.at(command)->PrintHelp();
  } else {
    std::cout << "Command not found" << '\n';
    PrintAvailableCommands();
  }
}

Cli::Cli() : global_options_("Global options") {
  global_options_.add_options()("help", "Show global help message")(
      "version", "Show citescoop version")(
      "log-level", options::value<std::string>()->default_value("warn"),
      "Logging level")("topic", options::value<std::string>(),
                       "topic of commands to use")(
      "command", options::value<std::string>(), "command to execute")(
      "subargs", options::value<std::vector<std::string>>(),
      "Arguments for command");

  positional_options_.add("topic", 1).add("command", 1).add("subargs", -1);

  Register(std::make_unique<Help>(topics_));
}

void Cli::Register(std::shared_ptr<Topic> topic) {
  spdlog::trace("Registering topic {}", topic->name());
  this->topics_.insert({topic->name(), std::move(topic)});
}

ExitCode Cli::Run(int argc, char* argv[]) {  // NOLINT(modernize-avoid-c-arrays)
  spdlog::trace("Got request to run command");

  auto parsed_global_args = ParseGlobalArgs(argc, argv);
  auto global_args = GlobalArgsToStruct(parsed_global_args.first);
  spdlog::set_level(global_args.log_level);

  if (HandleImmediateExecutionFlags(parsed_global_args.first))
    return ExitCode::kOk;

  auto topic = GetTopic(parsed_global_args.first);
  if (topic.empty())
    return ExitCode::kCliArgsError;

  auto command = GetCommand(parsed_global_args.first);
  if (command.empty())
    return ExitCode::kCliArgsError;

  auto command_args = options::collect_unrecognized(
      parsed_global_args.second.options, options::include_positional);

  RemoveUsedArgs(&command_args, topic, command);

  spdlog::debug("Running command {} {}", topic, command);
  ExitCode r_code = topics_.at(topic)->Run(command, command_args, global_args);
  spdlog::debug("Command exited with code {}", static_cast<int>(r_code));

  return r_code;
}

void Cli::PrintVersion() {
  std::cout << fmt::format("{} v{} ({})", cmake::kProjectName,
                           cmake::kProjectVersion, cmake::kGitSha)
            << '\n';
}

GlobalOptions Cli::GlobalArgsToStruct(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const boost::program_options::variables_map& args) {

  GlobalOptions options;
  options.log_level =
      spdlog::level::from_str(args["log-level"].as<std::string>());
  return options;
}

std::string Cli::GetTopic(const boost::program_options::variables_map& args) {
  if (args["topic"].empty()) {
    spdlog::critical("No topic passed");
    std::cout << "Missing required argument topic" << '\n';
    return "";
  }

  std::string topic = args["topic"].as<std::string>();
  if (!topics_.contains(topic)) {
    spdlog::critical("Topic {} not found", topic);
    return "";
  }
  return topic;
}

std::string Cli::GetCommand(const boost::program_options::variables_map& args) {
  if (args["command"].empty()) {
    spdlog::critical("No command passed");
    std::cout << "Missing required argument topic" << '\n';
    return "";
  }

  std::string command = args["command"].as<std::string>();
  return command;
}

void Cli::RemoveUsedArgs(std::vector<std::string>* args,
                         // NOLINTNEXTLINE(whitespace/indent_namespace)
                         const std::string& topic,
                         // NOLINTNEXTLINE(whitespace/indent_namespace)
                         const std::string& command) {
  auto index = std::ranges::find(args->begin(), args->end(), topic);
  if (index != args->end()) {
    args->erase(index);
  }

  index = std::ranges::find(args->begin(), args->end(), command);
  if (index != args->end()) {
    args->erase(index);
  }
}

std::pair<options::variables_map, options::parsed_options> Cli::ParseGlobalArgs(
    int argc,  // NOLINT(whitespace/indent_namespace)
    // NOLINTNEXTLINE (modernize-avoid-c-arrays, whitespace/indent_namespace)
    char* argv[]) {

  // Changes each run
  // NOLINTNEXTLINE(readability-identifier-naming)
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
  std::cout << fmt::format(
                   "Usage: {} [global options] <topic> <command> [<args>]",
                   cmake::kProjectName)
            << '\n';

  // BUG(computroniks) This will also print out --command and --subargs
  // which may not be intentional.
  std::cout << global_options_;
  std::cout << '\n' << "Available topics: " << '\n';

  for (auto const& [unused, topic] : topics_) {
    std::cout << fmt::format("{}\t\t{}", topic->name(), topic->description())
              << '\n';
  }

  std::cout << '\n' << "To get topic and command specific help, use:" << '\n';
  std::cout << "citescoop-cli help <topic> <command>" << '\n';
}

bool Cli::HandleImmediateExecutionFlags(const options::variables_map& args) {
  if (!args["help"].empty()) {
    PrintGlobalHelp();
    return true;
  }

  if (!args["topic"].empty() && args["command"].empty() &&
      args["topic"].as<std::string>() == "help") {
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
