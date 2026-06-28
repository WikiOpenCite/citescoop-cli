// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_DUMP_EXTRACT_H_
#define SRC_DUMP_EXTRACT_H_

#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "boost/program_options/variables_map.hpp"
#include "citescoop/extract.h"
#include "citescoop/parser.h"
#include "citescoop/proto/language.pb.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {

class ExtractCommand : public Command {
 public:
  ExtractCommand();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Streams {
    std::ifstream input;
    std::ofstream pages;
    std::ofstream revisions;
  };

  struct Args {
    std::string input;
    std::string pages;
    std::string revisions;
    bool stdin;
    wikiopencite::proto::Language language;
    bool bz2;
  };

  void SetExtractor();

  static std::string ExtractLangCode(const std::string& input);

  void LoadArgs(std::vector<std::string> args);
  void OpenStreams();
  void CloseStreams();
  void AddHeaders(std::pair<uint64_t, uint64_t> counts);

  Args args_;
  Streams streams_;
  std::shared_ptr<wikiopencite::citescoop::Parser> parser_;

  std::unique_ptr<wikiopencite::citescoop::Extractor> extractor_;

  static const std::ios_base::openmode kWriteOpenMode =
      std::ios::out | std::ios::binary | std::ios::trunc;
  static const std::ios_base::openmode kReadOpenMode =
      std::ios::in | std::ios::binary;
};

}  // namespace wikiopencite::citescoop::cli::dump

#endif  // SRC_DUMP_EXTRACT_H_
