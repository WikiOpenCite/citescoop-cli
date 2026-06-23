// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extract.h"

#include <arpa/inet.h>  // NOLINT(misc-include-cleaner)

#include <cstdint>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "boost/uuid/random_generator.hpp"
#include "boost/uuid/uuid.hpp"
#include "boost/uuid/uuid_io.hpp"
#include "citescoop/extract.h"
#include "citescoop/io.h"
#include "citescoop/parser.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "spdlog/spdlog.h"

#include "../langmap.h"
#include "cli.h"

namespace wikiopencite::citescoop::cli {

namespace cs = wikiopencite::citescoop;
namespace fs = std::filesystem;
namespace options = boost::program_options;
namespace proto = wikiopencite::proto;
namespace uuids = boost::uuids;

ExtractCommand::ExtractCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("extract", "Extract citations from") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::string>(), "Input file.")
    ("output,o", options::value<std::string>(), "Output file.")
    ("stdin,si", "Read input from stdin.")
    ("wiki", options::value<std::string>()->required(),
      "Name of wiki being processed. Used to set the language indicator"
      " of the file header.")
    ("bz2", "The input is compressed using bzip2 compression.")
    ("low-mem", "The reduced memory usage processing style should be used. "
      "This involves writing the pages and revisions to temporary files "
      "before recombining them at the end.")
    ("tmp-dir", options::value<std::string>(), "Directory to store temporary "
      "files when using the low memory mode.");
  // clang-format on
}

ExitCode ExtractCommand::Run(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::vector<std::string> args,
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    struct GlobalOptions /*globals*/) {
  auto parsed_args = ParseArgs(args);
  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});
  SetExtractor(parsed_args.first);

  if (parsed_args.first.contains("low-mem")) {
    return LowMemMode(parsed_args.first);
  }

  return NormalMode(parsed_args.first);
}

ExitCode ExtractCommand::NormalMode(const options::variables_map& args) {
  spdlog::debug("Starting dump processing in normal mode");

  auto output_file = EnsureArgument<std::string>("output", args);
  auto using_stdin = args.contains("stdin");
  auto lang = WikipediaCodeToLanguage(
      ExtractLangCode(EnsureArgument<std::string>("wiki", args)));
  std::string input_file;
  if (!using_stdin) {
    input_file = EnsureArgument<std::string>("input", args);
  }

  std::ofstream output(output_file,
                       std::ios::out | std::ios::binary | std::ios::trunc);

  if (using_stdin) {
    ProcessFileInMemory(std::cin, &output, lang);
  } else {
    std::ifstream input(input_file);
    ProcessFileInMemory(input, &output, lang);
    input.close();
  }

  output.close();
  return ExitCode::kOk;
}

ExitCode ExtractCommand::LowMemMode(const options::variables_map& args) {
  spdlog::debug("Starting dump processing in low memory mode");

  auto output_file = EnsureArgument<std::string>("output", args);
  auto tmp_dir = fs::path(EnsureArgument<std::string>("tmp-dir", args));
  auto using_stdin = args.contains("stdin");
  auto lang = WikipediaCodeToLanguage(
      ExtractLangCode(EnsureArgument<std::string>("wiki", args)));
  std::string input_file;
  if (!using_stdin) {
    input_file = EnsureArgument<std::string>("input", args);
  }

  DirMustExist(tmp_dir);

  auto run_uuid = GenerateUUID();
  auto paths = GetPaths(tmp_dir, run_uuid);

  std::ofstream pages_output(paths.pages, kWriteOpenMode);
  std::ofstream revisions_output(paths.revisions, kWriteOpenMode);

  std::pair<uint64_t, uint64_t> counts;

  if (using_stdin) {
    counts = extractor_->Extract(std::cin, &pages_output, &revisions_output);
  } else {
    std::ifstream input(input_file);
    counts = extractor_->Extract(input, &pages_output, &revisions_output);
    input.close();
  }

  auto fileheader = proto::FileHeader();
  fileheader.set_page_count(counts.first);
  fileheader.set_revision_count(counts.second);
  fileheader.set_language(lang);

  pages_output.close();
  revisions_output.close();

  std::ifstream pages_input(paths.pages, kReadOpenMode);
  std::ifstream revisions_input(paths.revisions, kReadOpenMode);
  std::ofstream output(output_file, kWriteOpenMode);
  auto writer = cs::MessageWriter(&output);

  spdlog::trace("Writing file header to output");
  writer.WriteMessage(fileheader);

  spdlog::trace("Copying pages and revisions");
  output << revisions_input.rdbuf();
  output << pages_input.rdbuf();

  output.close();
  pages_input.close();
  revisions_input.close();

  return ExitCode::kOk;
}

void ExtractCommand::ProcessFileInMemory(
    std::istream& input,     // NOLINT(whitespace/indent_namespace)
    std::ostream* output,    // NOLINT(whitespace/indent_namespace)
    proto::Language lang) {  // NOLINT(whitespace/indent_namespace)
  spdlog::trace("Starting extraction from input stream");

  auto [pages, revisions] = extractor_->Extract(input);

  spdlog::trace("Finished extracting citations.");
  spdlog::debug("Stored {} pages and {} revisions.", pages->size(),
                revisions->size());

  auto writer = cs::MessageWriter(output);

  auto fileheader = proto::FileHeader();
  fileheader.set_page_count(pages->size());
  fileheader.set_revision_count(revisions->size());
  fileheader.set_language(lang);

  spdlog::trace("Writing file header to output");
  writer.WriteMessage(fileheader);

  spdlog::trace("Writing revisions to output");
  for (auto& [unused, revision] : *revisions) {
    writer.WriteMessage(revision);
  }

  spdlog::trace("Writing pages to output");
  for (auto& page : *pages) {
    writer.WriteMessage(page);
  }
}

void ExtractCommand::SetExtractor(const options::variables_map& args) {
  if (args.contains("bz2")) {
    spdlog::debug("Using bz2 extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::Bz2Extractor(parser_));
  } else {
    spdlog::debug("Using plain text extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::TextExtractor(parser_));
  }
}

void ExtractCommand::DirMustExist(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("Directory does not exist: " + path.string());
  }
  if (!std::filesystem::is_directory(path)) {
    throw std::runtime_error("Path is not a directory: " + path.string());
  }
}

std::string ExtractCommand::ExtractLangCode(const std::string& input) {
  const std::string kSuffix = "wiki";
  auto pos = input.rfind(kSuffix);

  if (pos != std::string::npos && pos == input.size() - kSuffix.size()) {
    return input.substr(0, pos);
  }

  return input;
}

ExtractCommand::TempPaths ExtractCommand::GetPaths(
    const fs::path& temp_dir,  // NOLINT(whitespace/indent_namespace)
    const std::string& uuid    // NOLINT(whitespace/indent_namespace)
) {
  auto paths = TempPaths{.pages = temp_dir / (uuid + "-pages.tmp"),
                         .revisions = temp_dir / (uuid + "-revisions.tmp")};

  spdlog::debug(
      "Using following temporary paths for messages: pages: {}, revisions: {}",
      paths.pages.string(), paths.revisions.string());
  return paths;
}

std::string ExtractCommand::GenerateUUID() {
  // NOLINTNEXTLINE(readability-identifier-naming)
  const uuids::uuid uuid = uuids::random_generator()();
  return uuids::to_string(uuid);
}
}  // namespace wikiopencite::citescoop::cli
