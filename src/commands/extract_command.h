// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_EXTRACT_COMMAND_H_
#define SRC_COMMANDS_EXTRACT_COMMAND_H_

#include <string>

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class ExtractCommand : public BaseCommand {
 public:
  ExtractCommand();
  int Run(std::vector<std::string> args, GlobalOptions globals) override;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_EXTRACT_COMMAND_H_
