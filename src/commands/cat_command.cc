// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "cat_command.h"

#include <arpa/inet.h>
// NOLINTNEXTLINE(build/c++17)
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision_map.pb.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "spdlog/spdlog.h"

#include "exceptions.h"

namespace options = boost::program_options;
namespace fs = std::filesystem;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

namespace wikiopencite::citescoop::cli {
CatCommand::CatCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("cat", "Display a pbf file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.");
  positional_options_.add("file", 1);

  // clang-format on
}

int CatCommand::Run(std::vector<std::string> args,
                    // NOLINTNEXTLINE(whitespace/indent_namespace)
                    struct GlobalOptions) {
  auto parsed_args = ParseArgs(args);
  auto file = fs::path(EnsureArgument<std::string>("file", parsed_args.first));

  std::ifstream input(file, std::ios::in | std::ios::binary);
  auto raw_input = google::protobuf::io::IstreamInputStream(&input);
  auto coded_input =
      std::make_shared<google::protobuf::io::CodedInputStream>(&raw_input);

  auto header = ReadMessage<proto::FileHeader>(coded_input);
  std::shared_ptr<proto::FileHeader> header_message = std::move(header.second);
  PrintMessage(header_message, header.first);

  auto revisions = ReadMessage<proto::RevisionMap>(coded_input);
  std::shared_ptr<proto::RevisionMap> revisions_message =
      std::move(revisions.second);
  PrintMessage(revisions_message, revisions.first);

  for (int64_t i = 0; i < header_message->page_count(); i++) {
    auto page = ReadMessage<proto::Page>(coded_input);
    std::shared_ptr<proto::Page> page_message = std::move(page.second);
    PrintMessage(page_message, page.first);
  }

  return EXIT_SUCCESS;
}

void CatCommand::PrintMessage(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::shared_ptr<google::protobuf::Message> message, uint32_t size) {
  std::cout << message->GetTypeName() << " : " << size << " bytes" << '\n';
  std::cout << message->DebugString() << '\n';
}

}  // namespace wikiopencite::citescoop::cli
