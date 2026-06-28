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
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "spdlog/spdlog.h"

#include "cli.h"
#include "exceptions.h"

namespace wikiopencite::citescoop::cli::pbf {

namespace options = boost::program_options;
namespace fs = std::filesystem;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

Cat::Cat()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("cat", "Display a pbf file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.");
  positional_options_.add("file", 1);

  // clang-format on
}

ExitCode Cat::Run(std::vector<std::string> args,
                  // NOLINTNEXTLINE(whitespace/indent_namespace)
                  struct GlobalOptions) {
  LoadArgs(args);

  auto file = io::OpenPbfFile(args_.file);

  std::unique_ptr<proto::FileHeader> header;

  try {
    header = io::ReadPbfHeader(file.get());
  } catch (const exceptions::UnsupportedFileType& e) {
    spdlog::critical("Failed to read input file: {}", e.what());
    std::cerr << e.what() << '\n';

    return e.code();
  }

  file_type_ = header->type();
  message_count_ = header->count();

  PrintMessage(*header);

  for (uint64_t i = 0; i < message_count_; i++) {

    std::unique_ptr<google::protobuf::Message> message;
    try {
      message = io::ReadGenericMessage(file.get(), file_type_);
    } catch (const exceptions::UnsupportedFileType& e) {
      spdlog::critical("Failed to read input file: {}", e.what());
      std::cerr << e.what() << '\n';

      return e.code();
    }

    PrintMessage(*message);
  }

  return ExitCode::kOk;
}

void Cat::PrintMessage(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const google::protobuf::Message& message) {
  std::string text;
  google::protobuf::io::OstreamOutputStream output(&std::cout);

  std::cout << "# " << message.GetTypeName() << '\n';
  google::protobuf::TextFormat::Print(message, &output);
}

void Cat::LoadArgs(std::vector<std::string> args) {
  auto parsed_args = ParseArgs(args);
  args_.file = fs::path(EnsureArgument<std::string>("file", parsed_args.first));
}

}  // namespace wikiopencite::citescoop::cli::pbf
