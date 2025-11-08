// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_META_COMMAND_H_
#define SRC_COMMANDS_META_COMMAND_H_

#include <string>
#include <vector>

#include "google/protobuf/message.h"

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class MetaCommand : public BaseCommand {
 public:
  MetaCommand();
  int Run(std::vector<std::string> args, GlobalOptions globals) override;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_META_COMMAND_H_
