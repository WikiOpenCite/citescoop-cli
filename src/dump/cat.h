// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_CAT_H_
#define SRC_DUMP_CAT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "google/protobuf/message.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli {

class CatCommand : public Command {
 public:
  CatCommand();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  static void PrintMessage(const google::protobuf::Message& message,
                           uint32_t size);
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_DUMP_CAT_H_
