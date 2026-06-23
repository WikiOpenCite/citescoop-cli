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
  auto parsed_args = ParseArgs(args);

  revisions_ = std::vector<std::unique_ptr<proto::Revision>>();
  pages_ = std::vector<std::unique_ptr<proto::Page>>();
  language_ = proto::Language::LANGUAGE_UNSPECIFIED;

  ReadFiles(
      EnsureArgument<std::vector<std::string>>("input", parsed_args.first));

  WriteOutput(EnsureArgument<std::string>("output", parsed_args.first));

  return ExitCode::kOk;
}

void CombineCommand::ReadFiles(const std::vector<std::string>& files) {
  for (const auto& file : files) {
    spdlog::trace("Reading from {}", file);

    std::ifstream input(file, std::ios::in | std::ios::binary);
    auto reader = cs::MessageReader(&input);
    auto header = reader.ReadMessage<proto::FileHeader>();

    if (language_ == proto::Language::LANGUAGE_UNSPECIFIED) {
      language_ = header->language();
    } else if (language_ != header->language()) {
      const google::protobuf::EnumDescriptor* descriptor =
          proto::Language_descriptor();

      spdlog::warn(
          "Language of file {} ({}) does not match language of other files "
          "({})",
          file, descriptor->FindValueByNumber(header->language())->name(),
          descriptor->FindValueByNumber(language_)->name());
    }

    for (uint64_t i = 0; i < header->revision_count(); i++) {
      revisions_.push_back(reader.ReadMessage<proto::Revision>());
    }

    for (uint64_t i = 0; i < header->page_count(); i++) {
      pages_.push_back(reader.ReadMessage<proto::Page>());
    }

    input.close();
  }
}

void CombineCommand::WriteOutput(const std::string& file) {
  const auto kWriteMode = std::ios::out | std::ios::binary | std::ios::trunc;
  std::ofstream output(file, kWriteMode);

  auto writer = cs::MessageWriter(&output);
  auto fileheader = proto::FileHeader();
  fileheader.set_page_count(pages_.size());
  fileheader.set_revision_count(revisions_.size());
  fileheader.set_language(language_);

  spdlog::trace("Writing file header");
  writer.WriteMessage(fileheader);

  spdlog::trace("Writing {} revisions", fileheader.revision_count());
  for (const auto& revision : revisions_) {
    writer.WriteMessage(*revision);
  }

  spdlog::trace("Writing {} pages", fileheader.page_count());
  for (const auto& page : pages_) {
    writer.WriteMessage(*page);
  }

  output.close();
}

}  // namespace wikiopencite::citescoop::cli::dump
