// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_HELP_COMMAND_H_
#define SRC_COMMANDS_HELP_COMMAND_H_

#include <memory>
#include <string>
#include <vector>

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class HelpCommand : public BaseCommand {
 public:
  HelpCommand(
      std::shared_ptr<std::vector<std::shared_ptr<BaseCommand>>> commands);
  int Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  std::shared_ptr<std::vector<std::shared_ptr<BaseCommand>>> commands_;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_HELP_COMMAND_H_
