// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_EXTRACT_COMMAND_H_
#define SRC_COMMANDS_EXTRACT_COMMAND_H_

// NOLINTNEXTLINE(build/c++17)
#include <filesystem>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "google/protobuf/message.h"

#include "citescoop/extract.h"
#include "citescoop/parser.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

class ExtractCommand : public BaseCommand {
 public:
  ExtractCommand();
  int Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  std::filesystem::path dump_root_;
  std::filesystem::path output_dir_;
  std::shared_ptr<wikiopencite::citescoop::Parser> parser_;
  std::unique_ptr<wikiopencite::citescoop::Bz2Extractor> extractor_;

  std::vector<std::string> GetWikis(std::string wiki_filter);
  std::vector<std::string> GetDumpFiles(std::vector<std::string> wikis);
  void ProcessFile(std::string path);

  static std::string ExtractLangCode(const std::string& input);
  static void EnsureDirectory(const std::filesystem::path& path);
  static void WriteMessage(const google::protobuf::Message& message,
                           std::ofstream& output);
  static void DisplayProgress(double val);
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_COMMANDS_EXTRACT_COMMAND_H_
