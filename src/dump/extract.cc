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
#include "citescoop/extract.h"
#include "citescoop/io.h"
#include "citescoop/parser.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "spdlog/spdlog.h"

#include "cli.h"
#include "io.h"
#include "langmap.h"

namespace wikiopencite::citescoop::cli::dump {

namespace cs = wikiopencite::citescoop;
namespace fs = std::filesystem;
namespace options = boost::program_options;
namespace proto = wikiopencite::proto;

ExtractCommand::ExtractCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("extract", "Extract citations from") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::string>(), "Input file.")
    ("pages,p", options::value<std::string>()->required(), "Filename for pages output.")
    ("revisions,r", options::value<std::string>()->required(), "Filename for revisions output.")
    ("stdin,si", "Read input from stdin.")
    ("wiki", options::value<std::string>()->required(),
      "Name of wiki being processed. Used to set the language indicator"
      " of the file header.")
    ("bz2", "The input is compressed using bzip2 compression.");
  // clang-format on
}

ExitCode ExtractCommand::Run(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::vector<std::string> args,
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    struct GlobalOptions /*globals*/) {
  LoadArgs(args);
  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});
  SetExtractor();

  OpenStreams();
  std::pair<uint64_t, uint64_t> counts;
  if (args_.stdin)
    counts =
        extractor_->Extract(std::cin, &streams_.pages, &streams_.revisions);
  else
    counts = extractor_->Extract(streams_.input, &streams_.pages,
                                 &streams_.revisions);

  CloseStreams();
  AddHeaders(counts);

  return ExitCode::kOk;
}

void ExtractCommand::SetExtractor() {
  if (args_.bz2) {
    spdlog::debug("Using bz2 extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::Bz2Extractor(parser_));
  } else {
    spdlog::debug("Using plain text extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::TextExtractor(parser_));
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

void ExtractCommand::LoadArgs(std::vector<std::string> args) {
  auto parsed_args = ParseArgs(args);

  args_.stdin = parsed_args.first.contains("stdin");
  if (!args_.stdin)
    args_.input = EnsureArgument<std::string>("input", parsed_args.first);

  args_.pages = EnsureArgument<std::string>("pages", parsed_args.first);
  args_.revisions = EnsureArgument<std::string>("revisions", parsed_args.first);
  args_.bz2 = parsed_args.first.contains("bz2");
  args_.language = WikipediaCodeToLanguage(
      ExtractLangCode(EnsureArgument<std::string>("wiki", parsed_args.first)));
}

void ExtractCommand::OpenStreams() {
  if (!args_.stdin)
    streams_.input = std::ifstream(args_.input);

  streams_.pages = std::ofstream(
      args_.pages + ".tmp", std::ios::out | std::ios::binary | std::ios::trunc);
  streams_.revisions =
      std::ofstream(args_.revisions + ".tmp",
                    std::ios::out | std::ios::binary | std::ios::trunc);
}

void ExtractCommand::CloseStreams() {
  if (!args_.stdin)
    streams_.input.close();

  streams_.pages.close();
  streams_.revisions.close();
}

void ExtractCommand::AddHeaders(std::pair<uint64_t, uint64_t> counts) {
  std::ifstream tmp_pages(args_.pages + ".tmp");
  std::ifstream tmp_revisions(args_.revisions + ".tmp");
  std::ofstream pages(args_.pages,
                      std::ios::out | std::ios::binary | std::ios::trunc);
  std::ofstream revisions(args_.revisions,
                          std::ios::out | std::ios::binary | std::ios::trunc);

  auto attributes = proto::DumpFileAdditionalData();
  attributes.set_language(args_.language);

  auto header = proto::FileHeader();
  header.set_count(counts.first);
  header.set_type(proto::FileType::FILE_TYPE_PAGES);
  header.set_allocated_dump_file_attributes(&attributes);
  io::PrependHeader(header, tmp_pages, &pages);

  header.set_count(counts.second);
  header.set_type(proto::FileType::FILE_TYPE_REVISIONS);
  io::PrependHeader(header, tmp_revisions, &revisions);

  tmp_pages.close();
  tmp_revisions.close();
  pages.close();
  revisions.close();

  fs::remove(args_.pages + ".tmp");
  fs::remove(args_.revisions + ".tmp");
}
}  // namespace wikiopencite::citescoop::cli::dump
