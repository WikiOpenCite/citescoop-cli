// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_META_H_
#define SRC_DUMP_META_H_

#include <string>
#include <vector>

#include "cli.h"
#include "io.h"

namespace wikiopencite::citescoop::cli::pbf {

class Meta : public Command {
 public:
  Meta();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Args {
    std::string input;
    bool pretty;
  };

  void LoadArgs(std::vector<std::string> args);
  void OpenFile();
  void LoadHeader();
  std::pair<size_t, size_t> CalculateSize();
  std::string FormatAdditionalAttributes();
  std::string FormatSize(size_t size);

  Args args_;
  wikiopencite::proto::FileType file_type_;
  std::unique_ptr<wikiopencite::proto::FileHeader> header_;
  std::unique_ptr<wikiopencite::citescoop::cli::io::PbfFile> input_;
};

}  // namespace wikiopencite::citescoop::cli::pbf

#endif  // SRC_DUMP_META_H_
