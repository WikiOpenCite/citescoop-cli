// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "combine.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"
#include "google/protobuf/descriptor.h"
#include "spdlog/spdlog.h"

#include "cli.h"

namespace wikiopencite::citescoop::cli::dump {

namespace options = boost::program_options;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

CombineCommand::CombineCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("combine", "Combine multiple pbf files into one") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::vector<std::string>>()->required(),
      "Input files to be combined.")
    ("output,o", options::value<std::string>()->required(), "Output file");
  // clang-format on

  positional_options_.add("input", -1);
}

ExitCode CombineCommand::Run(
    std::vector<std::string> args,  // NOLINT(whitespace/indent_namespace)
    struct GlobalOptions            // NOLINT(whitespace/indent_namespace)
) {
  language_ = proto::Language::LANGUAGE_UNSPECIFIED;
  total_messages_ = 0;
  file_type_ = proto::FileType::FILE_TYPE_UNSPECIFIED;

  LoadArgs(args);
  OpenStreams();
  ReadHeaders();
  CopyData();
  CloseStreams();

  return ExitCode::kOk;
}

void CombineCommand::LoadArgs(std::vector<std::string> args) {
  auto parsed_args = ParseArgs(args);
  args_.inputs =
      EnsureArgument<std::vector<std::string>>("input", parsed_args.first);
  args_.output = EnsureArgument<std::string>("output", parsed_args.first);
}

void CombineCommand::OpenStreams() {
  streams_.inputs = std::vector<std::ifstream>();
  for (const auto& file : args_.inputs) {
    streams_.inputs.push_back(
        std::ifstream(file, std::ios::in | std::ios::binary));
  }

  streams_.output = std::ofstream(
      args_.output, std::ios::out | std::ios::binary | std::ios::trunc);
}

void CombineCommand::CloseStreams() {
  for (auto& stream : streams_.inputs) {
    stream.close();
  }

  streams_.output.close();
}

void CombineCommand::ReadHeaders() {
  for (auto& stream : streams_.inputs) {
    auto reader = cs::MessageReader(&stream);
    auto header = reader.ReadMessage<proto::FileHeader>();
    ValidateFileType(header->type());
    ValidateLanguage(header->dump_file_attributes().language());
    total_messages_ += header->count();
  }
}

void CombineCommand::ValidateFileType(proto::FileType type) {
  if (file_type_ == proto::FileType::FILE_TYPE_UNSPECIFIED) {
    file_type_ = type;
  } else if (type != file_type_) {
    const google::protobuf::EnumDescriptor* descriptor =
        proto::FileType_descriptor();

    spdlog::error("file types do not match, got {} but expected {}",
                  descriptor->FindValueByNumber(type)->name(),
                  descriptor->FindValueByNumber(file_type_)->name());
    throw std::invalid_argument("file types do not match");
  }
}

void CombineCommand::ValidateLanguage(proto::Language language) {
  if (language_ == proto::Language::LANGUAGE_UNSPECIFIED) {
    language_ = language;
  } else if (language != language_) {
    const google::protobuf::EnumDescriptor* descriptor =
        proto::Language_descriptor();

    spdlog::error("file languages do not match, got {} but expected {}",
                  descriptor->FindValueByNumber(language)->name(),
                  descriptor->FindValueByNumber(language_)->name());
    throw std::invalid_argument("file languages do not match");
  }
}

void CombineCommand::CopyData() {
  auto writer = cs::MessageWriter(&streams_.output);

  auto fileheader = proto::FileHeader();
  fileheader.set_type(file_type_);
  fileheader.set_count(total_messages_);
  auto attributes = proto::DumpFileAdditionalData();
  attributes.set_language(language_);
  fileheader.set_allocated_dump_file_attributes(&attributes);

  writer.WriteMessage(fileheader);

  for (auto& stream : streams_.inputs) {
    streams_.output << stream.rdbuf();
  }
}

}  // namespace wikiopencite::citescoop::cli::dump
