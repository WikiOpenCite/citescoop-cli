// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_META_H_
#define SRC_DUMP_META_H_

#include <string>
#include <vector>

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {

class MetaCommand : public Command {
 public:
  MetaCommand();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;
};

}  // namespace wikiopencite::citescoop::cli::dump

#endif  // SRC_DUMP_META_H_
