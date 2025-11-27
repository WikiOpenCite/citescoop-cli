// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_BASE_COMMAND_H_
#define SRC_COMMANDS_BASE_COMMAND_H_

#include <cstdint>
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
  kCliArgsError = 3,
};

// Base type for CLI commands
class BaseCommand {
 public:
  explicit BaseCommand(std::string name, std::string description);
  virtual ~BaseCommand() = default;

  virtual int Run(std::vector<std::string> args,
                  struct GlobalOptions globals) = 0;

  void PrintHelp();

  std::string description() { return description_; }

  std::string name() { return name_; }

 protected:
  std::pair<boost::program_options::variables_map,
            boost::program_options::parsed_options>
  ParseArgs(const std::vector<std::string>& args);
  std::string name_;
  std::string description_;
  boost::program_options::options_description cli_options_;
  boost::program_options::positional_options_description positional_options_;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_BASE_COMMAND_H_
