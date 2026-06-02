// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_COMBINE_H_
#define SRC_DUMP_COMBINE_H_

#include <memory>
#include <string>
#include <vector>

#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli {

class CombineCommand : public Command {
 public:
  CombineCommand();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  std::vector<std::unique_ptr<wikiopencite::proto::Revision>> revisions_;
  std::vector<std::unique_ptr<wikiopencite::proto::Page>> pages_;
  wikiopencite::proto::Language language_;

  void ReadFiles(const std::vector<std::string>& files);
  void WriteOutput(const std::string& file);
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_DUMP_COMBINE_H_
