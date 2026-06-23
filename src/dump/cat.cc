// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cat.h"

#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {

namespace options = boost::program_options;
namespace fs = std::filesystem;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

CatCommand::CatCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("cat", "Display a pbf file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.");
  positional_options_.add("file", 1);

  // clang-format on
}

ExitCode CatCommand::Run(std::vector<std::string> args,
                         // NOLINTNEXTLINE(whitespace/indent_namespace)
                         struct GlobalOptions) {
  auto parsed_args = ParseArgs(args);
  auto file = fs::path(EnsureArgument<std::string>("file", parsed_args.first));

  std::ifstream input(file, std::ios::in | std::ios::binary);
  auto reader = cs::MessageReader(&input);

  auto header = reader.ReadMessage<proto::FileHeader>();
  PrintMessage(*header, header->ByteSizeLong());

  for (uint64_t i = 0; i < header->revision_count(); i++) {
    auto revision = reader.ReadMessage<proto::Revision>();
    PrintMessage(*revision, revision->ByteSizeLong());
  }

  for (uint64_t i = 0; i < header->page_count(); i++) {
    auto page = reader.ReadMessage<proto::Page>();
    PrintMessage(*page, page->ByteSizeLong());
  }

  return ExitCode::kOk;
}

void CatCommand::PrintMessage(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const google::protobuf::Message& message, uint32_t size) {
  std::cout << message.GetTypeName() << " : " << size << " bytes" << '\n';
  std::cout << message.DebugString() << '\n';
}

}  // namespace wikiopencite::citescoop::cli::dump
