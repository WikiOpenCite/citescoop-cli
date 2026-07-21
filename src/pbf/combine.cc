// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "combine.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_set>
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
#include "fmt/ranges.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "spdlog/spdlog.h"

#include "cli.h"
#include "exceptions.h"
#include "io.h"

namespace wikiopencite::citescoop::cli::pbf {

namespace {
namespace options = boost::program_options;
namespace cs = wikiopencite::citescoop;
namespace fs = std::filesystem;
namespace proto = wikiopencite::proto;
}  // namespace

Combine::Combine()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("combine", "Combine multiple pbf files into one") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::vector<std::string>>()->required(),
      "Input files to be combined.")
    ("output,o", options::value<std::string>()->required(), "Output file");
  // clang-format on

  positional_options_.add("output", 1);
  positional_options_.add("input", -1);
}

ExitCode Combine::Run(
    std::vector<std::string> args,  // NOLINT(whitespace/indent_namespace)
    struct GlobalOptions            // NOLINT(whitespace/indent_namespace)
) {
  language_ = proto::Language::LANGUAGE_UNSPECIFIED;
  total_messages_ = 0;
  file_type_ = proto::FileType::FILE_TYPE_UNSPECIFIED;

  LoadArgs(args);
  OpenStreams();
  try {
    ReadHeaders();
    CopyData();
  } catch (const exceptions::UnsupportedFileType& e) {
    spdlog::critical("Failed to read input file: {}", e.what());
    std::cerr << e.what() << '\n';

    CloseStreams();
    streams_.tmp_output.close();
    CleanupTempFile();

    return e.code();
  }

  CloseStreams();

  return ExitCode::kOk;
}

void Combine::LoadArgs(const std::vector<std::string>& args) {
  auto parsed_args = ParseArgs(args);
  args_.inputs =
      EnsureArgument<std::vector<std::string>>("input", parsed_args.first);
  args_.output = EnsureArgument<std::string>("output", parsed_args.first);

  spdlog::trace("Combine command arguments: Inputs: {} Output: {}",
                fmt::join(args_.inputs, ", "), args_.output);
}

void Combine::OpenStreams() {
  streams_.inputs = std::vector<std::unique_ptr<io::PbfFile>>();
  for (const auto& file : args_.inputs) {
    streams_.inputs.push_back(io::OpenPbfFile(file));
  }

  streams_.output = std::ofstream(
      args_.output, std::ios::out | std::ios::binary | std::ios::trunc);
  streams_.tmp_output =
      std::ofstream(args_.output + ".tmp",
                    std::ios::out | std::ios::binary | std::ios::trunc);
}

void Combine::CloseStreams() {
  for (auto& stream : streams_.inputs) {
    io::ClosePbfFile(std::move(stream));
  }

  streams_.output.close();
}

void Combine::ReadHeaders() {
  for (auto& stream : streams_.inputs) {
    auto header = io::ReadPbfHeader(stream.get());
    ValidateFileType(header->type());
    ValidateFileSpecificAttributes(*header);
    total_messages_ += header->count();
    message_counts_.push_back(header->count());
  }
}

void Combine::ValidateFileType(proto::FileType type) {
  if (file_type_ == proto::FileType::FILE_TYPE_UNSPECIFIED) {
    file_type_ = type;
  } else if (type != file_type_) {
    const google::protobuf::EnumDescriptor* descriptor =
        proto::FileType_descriptor();

    spdlog::error("file types do not match, got {} but expected {}",
                  descriptor->FindValueByNumber(type)->name(),
                  descriptor->FindValueByNumber(file_type_)->name());
    throw exceptions::UnsupportedFileType("file types do not match");
  }
}

void Combine::ValidateFileSpecificAttributes(
    const wikiopencite::proto::FileHeader& header) {
  switch (header.type()) {
    case proto::FileType::FILE_TYPE_PAGES:
    case proto::FileType::FILE_TYPE_REVISIONS:
      ValidateLanguage(header.dump_file_attributes().language());
      break;
    default:
      break;
  }
}

void Combine::ValidateLanguage(proto::Language language) {
  if (language_ == proto::Language::LANGUAGE_UNSPECIFIED) {
    language_ = language;
  } else if (language != language_) {
    const google::protobuf::EnumDescriptor* descriptor =
        proto::Language_descriptor();

    spdlog::error("file languages do not match, got {} but expected {}",
                  descriptor->FindValueByNumber(language)->name(),
                  descriptor->FindValueByNumber(language_)->name());
    throw exceptions::UserInputException("file languages do not match");
  }
}

namespace {

std::string GetOpenAlexId(const google::protobuf::Message& message) {
  const auto* field = message.GetDescriptor()->FindFieldByName("openalex_id");
  if (field == nullptr ||
      field->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
    return std::string();
  }

  return message.GetReflection()->GetString(message, field);
}

bool IsUniqueOpenAlexId(
    const google::protobuf::Message& message,
    const std::shared_ptr<std::unordered_set<std::string>>& seen_ids) {
  const std::string kOpenAlexId = GetOpenAlexId(message);
  if (kOpenAlexId.empty()) {
    return true;
  }

  return seen_ids->insert(kOpenAlexId).second;
}

}  // namespace

std::function<bool(const google::protobuf::Message&)>
Combine::PredicateFactory() const {
  switch (file_type_) {
    case proto::FileType::FILE_TYPE_OPENALEX_WORKS:
    case proto::FileType::FILE_TYPE_OPENALEX_INSTITUTIONS:
    case proto::FileType::FILE_TYPE_OPENALEX_AUTHORS: {
      auto seen_ids = std::make_shared<std::unordered_set<std::string>>();
      return [seen_ids](const google::protobuf::Message& message) {
        return IsUniqueOpenAlexId(message, seen_ids);
      };
    }
    default:
      return [](const google::protobuf::Message&) {
        return true;
      };
  }
}

void Combine::CopyData() {
  const auto kTempPath = args_.output + ".tmp";

  const uint64_t kTotalWritten = CopyMessagesToTemp();
  streams_.tmp_output.close();

  auto fileheader = proto::FileHeader();
  fileheader.set_type(file_type_);
  fileheader.set_count(kTotalWritten);

  SetAdditionalAttributes(&fileheader);

  auto writer = cs::MessageWriter(&streams_.output);
  writer.WriteMessage(fileheader);

  std::ifstream tmp_input(kTempPath, std::ios::in | std::ios::binary);
  streams_.output << tmp_input.rdbuf();
  tmp_input.close();

  CleanupTempFile();
}

uint64_t Combine::CopyMessagesToTemp() {
  uint64_t total_written = 0;
  auto tmp_writer = cs::MessageWriter(&streams_.tmp_output);
  auto predicate = PredicateFactory();

  for (std::size_t i = 0; i < streams_.inputs.size(); ++i) {
    auto& stream = streams_.inputs[i];
    auto count = message_counts_[i];
    for (uint64_t j = 0; j < count; ++j) {
      auto message = io::ReadGenericMessage(stream.get(), file_type_);
      if (predicate(*message)) {
        tmp_writer.WriteMessage(*message);
        total_written++;
      }
    }
  }
  return total_written;
}

void Combine::CleanupTempFile() const {
  const fs::path kTempPath = args_.output + ".tmp";
  std::error_code err;
  fs::remove(kTempPath, err);
  if (err) {
    spdlog::warn("Failed to remove temporary file: {}", kTempPath.string());
  }
}

void Combine::SetAdditionalAttributes(wikiopencite::proto::FileHeader* header) {
  switch (file_type_) {
    case proto::FileType::FILE_TYPE_PAGES:
    case proto::FileType::FILE_TYPE_REVISIONS: {
      auto attributes = proto::DumpFileAdditionalData();
      attributes.set_language(language_);
      header->set_allocated_dump_file_attributes(&attributes);
      break;
    }
    default:
      break;
  }
}

}  // namespace wikiopencite::citescoop::cli::pbf
