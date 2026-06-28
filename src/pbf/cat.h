// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_PBF_CAT_H_
#define SRC_PBF_CAT_H_

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"
#include "google/protobuf/message.h"

#include "cli.h"
#include "io.h"

namespace wikiopencite::citescoop::cli::pbf {

class Cat : public Command {
 public:
  Cat();
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Args {
    std::filesystem::path file;
  };

  struct PbfFile {
    std::ifstream stream;
    std::unique_ptr<wikiopencite::citescoop::MessageReader> reader;
  };

  static void PrintMessage(const google::protobuf::Message& message);
  void LoadArgs(std::vector<std::string> args);

  Args args_;
  wikiopencite::proto::FileType file_type_;
  uint64_t message_count_;
  PbfFile input_;
};

}  // namespace wikiopencite::citescoop::cli::pbf

#endif  // SRC_PBF_CAT_H_
