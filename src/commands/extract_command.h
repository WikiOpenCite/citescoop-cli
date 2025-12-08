// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_EXTRACT_COMMAND_H_
#define SRC_COMMANDS_EXTRACT_COMMAND_H_

#include <filesystem>  // NOLINT(build/c++17)
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

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class ExtractCommand : public BaseCommand {
 public:
  ExtractCommand();
  int Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct TempPaths {
    std::filesystem::path pages;
    std::filesystem::path revisions;
  };

  std::shared_ptr<wikiopencite::citescoop::Parser> parser_;

  std::unique_ptr<wikiopencite::citescoop::Extractor> extractor_;

  static const std::ios_base::openmode kWriteOpenMode =
      std::ios::out | std::ios::binary | std::ios::trunc;
  static const std::ios_base::openmode kReadOpenMode =
      std::ios::in | std::ios::binary;

  int NormalMode(const boost::program_options::variables_map& args);
  int LowMemMode(const boost::program_options::variables_map& args);

  void ProcessFileInMemory(std::istream& input, std::ostream* output,
                           wikiopencite::proto::Language lang);

  void SetExtractor(const boost::program_options::variables_map& args);

  static void DirMustExist(const std::filesystem::path& path);

  static std::string ExtractLangCode(const std::string& input);

  static std::string GenerateUUID();

  static TempPaths GetPaths(const std::filesystem::path& temp_dir,
                            const std::string& uuid);
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_EXTRACT_COMMAND_H_
