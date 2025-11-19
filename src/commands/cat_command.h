// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_CAT_COMMAND_H_
#define SRC_COMMANDS_CAT_COMMAND_H_

#include <string>
#include <vector>

#include "google/protobuf/message.h"

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class CatCommand : public BaseCommand {
 public:
  CatCommand();
  int Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  void PrintMessage(std::shared_ptr<google::protobuf::Message> message,
                    uint32_t size);
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_CAT_COMMAND_H_
