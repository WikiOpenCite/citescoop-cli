// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_COMBINE_H_
#define SRC_DUMP_COMBINE_H_

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {

class CombineCommand : public Command {
 public:
  CombineCommand();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Args {
    std::vector<std::string> inputs;
    std::string output;
  };

  struct Streams {
    std::vector<std::ifstream> inputs;
    std::ofstream output;
  };

  void LoadArgs(std::vector<std::string> args);
  void OpenStreams();
  void CloseStreams();
  void ReadHeaders();
  void ValidateFileType(wikiopencite::proto::FileType type);
  void ValidateLanguage(wikiopencite::proto::Language language);
  void CopyData();

  wikiopencite::proto::Language language_;
  Args args_;
  Streams streams_;
  uint64_t total_messages_;
  wikiopencite::proto::FileType file_type_;
};

}  // namespace wikiopencite::citescoop::cli::dump

#endif  // SRC_DUMP_COMBINE_H_
