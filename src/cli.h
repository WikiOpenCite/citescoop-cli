// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_CLI_H_
#define SRC_CLI_H_

#include <arpa/inet.h>  // NOLINT(misc-include-cleaner)

#include <cstdint>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/variables_map.hpp"
#include "spdlog/common.h"

namespace wikiopencite::citescoop::cli {

struct GlobalOptions {
  spdlog::level::level_enum log_level;
};

enum class ExitCode : std::uint8_t {
  kOk = 0,
  kCliArgsError = 3,
};

class CommandException : public std::exception {
 public:
  explicit CommandException(std::string description)
      : error_description_(std::move(description)) {}

  [[nodiscard]] const char* what() const noexcept override {
    return error_description_.c_str();
  }

 protected:
  std::string error_description_;
};

class FilesystemException : public CommandException {
 public:
  explicit FilesystemException(const std::string& description)
      : CommandException(description) {}
};

class MissingArgumentException : public CommandException {
 public:
  explicit MissingArgumentException(const std::string& description)
      : CommandException(description) {}
};

/// A single command in the CLI
class Command {
 public:
  explicit Command(std::string name, std::string description);
  explicit Command(std::string name, std::string topic,
                   std::string description);
  virtual ~Command() = default;

  /// Execute this command.
  ///
  /// @param args Arguments provided at the command line.
  /// @param globals Global arguments passed to all commands.
  /// @return Return status. See ExitCode for meanings. Any non-zero
  /// value indicates failure.
  virtual ExitCode Run(std::vector<std::string> args,
                       struct GlobalOptions globals) = 0;

  /// Print the help text for this command.
  void PrintHelp();

  /// Brief description of this command.
  std::string description() { return description_; }

  /// Name of this command.
  std::string name() { return name_; }

  /// Name of the parent topic. May be an empty string
  std::string topic() { return topic_; }

 protected:
  /// @brief Parse the provided command line arguments according to the
  /// commands configured options.
  ///
  /// @param args List of arguments passed at the command line.
  /// @return The variable map of parsed arguments and parsed options
  /// for future use.
  std::pair<boost::program_options::variables_map,
            boost::program_options::parsed_options>
  ParseArgs(const std::vector<std::string>& args);

  /// Ensure that a given argument exists and if so return it, otherwise
  /// throw an exception.
  ///
  /// @tparam T Type to cast argument as.
  /// @param arg Name of argument to retrieve.
  /// @param args Variable map of command line arguments.
  /// @return The value of the argument as type T.
  template <typename T>
  T EnsureArgument(const std::string& arg,
                   // NOLINTNEXTLINE(whitespace/indent_namespace)
                   const boost::program_options::variables_map& args) {
    if (!args.contains(arg)) {
      throw MissingArgumentException("Missing required argument " + arg);
    }

    return args[arg].as<T>();
  }

  /// Name of parent topic
  std::string topic_;

  /// Command name
  std::string name_;

  /// Brief description of command
  std::string description_;

  /// Command flags and options
  boost::program_options::options_description cli_options_;

  /// Positional arguments
  boost::program_options::positional_options_description positional_options_;
};

/// A group of related commands.
class Topic {
 public:
  explicit Topic(std::string name, std::string description);
  virtual ~Topic() = default;

  /// Register a command in this topic
  /// @param command Command to register
  void Register(std::shared_ptr<Command> command);

  /// Execute the specified command.
  ///
  /// @param command Command to execute.
  /// @param args Arguments passed at the command line to parse.
  /// @param globals Global arguments that apply to all commands
  /// @return Return status. See ExitCode for meanings. Any non-zero
  /// value indicates failure.
  virtual ExitCode Run(std::string command, std::vector<std::string> args,
                       struct GlobalOptions globals);

  /// Print help for this topic.
  ///
  /// This is different from command help as this is topic wide.
  void PrintHelp();

  /// Print help for a specific command in this topic.
  ///
  /// @param command Command to print help for.
  void PrintHelp(std::string command);

  /// A brief description of this topic
  std::string description() { return description_; }

  /// The name of this topic
  std::string name() { return name_; }

 protected:
  /// Print a list of available commands
  void PrintAvailableCommands();

  /// Name of this topic
  std::string name_;

  /// Brief description of this topic
  std::string description_;

  /// Mapping of command names to commands
  std::map<std::string, std::shared_ptr<Command>> commands_;
};

class Cli {
 public:
  Cli();

  /// @brief Registers a topic within the CLI.
  ///
  /// @param topic Topic to register.
  void Register(std::shared_ptr<Topic> topic);

  /// @brief Run the specified command if it exists. If the command
  /// specified does not exist, an exception will be thrown.
  ///
  /// @param argc Number of command line arguments in array
  /// @param argv Array of command line argument values
  ExitCode Run(int argc, char* argv[]);  //NOLINT (modernize-avoid-c-arrays)

 private:
  /// @brief Print the program version
  static void PrintVersion();

  /// @brief Take the parsed variables map for global arguments and
  /// convert it to a struct ready to pass to the commands.
  ///
  /// @param args Arguments variables map for global args.
  /// @returns Parsed global options.
  static GlobalOptions GlobalArgsToStruct(
      const boost::program_options::variables_map& args);

  /// Get the topic from command line arguments
  ///
  /// Checks that the topic has been passed by the user and that it
  /// exists. If not, will output message to user and return an empty
  /// string.
  ///
  /// @param vm Parsed command line arguments
  /// @return Current topic or an empty string on error
  std::string GetTopic(boost::program_options::variables_map vm);

  /// Get the command from arguments
  ///
  /// Checks if the command has been passed by the user. Unlike
  /// GetTopic, no check is made to see if this command exists as this
  /// is handled by the topic runner. If the command isn't passed,
  /// prints a message to the user and returns an empty string.
  ///
  /// @param vm Parsed command line arguments.
  /// @return Current command or empty string on error.
  std::string GetCommand(boost::program_options::variables_map vm);

  /// Remove the command and topic from arguments to be passed to the
  /// @param args
  /// @param topic
  /// @param command
  static void RemoveUsedArgs(std::vector<std::string>* args, std::string topic,
                             std::string command);

  /// @brief Parse the global arguments
  ///
  /// @param argc Number of command line arguments in array
  /// @param argv Array of command line arguments
  /// @return Boost variables map and parsed options. Parsed options
  /// includes any unrecognized options for parsing by subcommands.
  std::pair<boost::program_options::variables_map,
            boost::program_options::parsed_options>
  ParseGlobalArgs(int argc, char* argv[]);  // NOLINT(modernize-avoid-c-arrays)

  // Print the global help
  void PrintGlobalHelp();

  /// @brief  Handle flags that result in an immediate execution. I.e.
  /// --help and --version.
  ///
  /// @param args Command line arguments variable map.
  /// @returns Were any flags executed? If they were we should probably
  ///  exit with success.
  bool HandleImmediateExecutionFlags(
      const boost::program_options::variables_map& args);

  // Mapping of command name to command
  std::map<std::string, std::shared_ptr<Topic>> topics_;

  boost::program_options::options_description global_options_;
  boost::program_options::positional_options_description positional_options_;
};
}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_CLI_H_
