// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extract_command.h"

#include <arpa/inet.h>  // NOLINT(misc-include-cleaner)

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
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

#include "../langmap.h"
#include "base_command.h"

namespace wikiopencite::citescoop::cli {

namespace options = boost::program_options;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

ExtractCommand::ExtractCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("extract", "Extract citations from") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::string>(), "Input file.")
    ("output,o", options::value<std::string>(), "Output file.")
    ("stdin,si", "Read input from stdin.")
    ("wiki", options::value<std::string>()->default_value("all"),
      "Name of wiki being processed. Used to set the language indicator"
      " of the file header.")
    ("bz2", "The input is compressed using bzip2 compression.");
  // clang-format on
}

int ExtractCommand::Run(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::vector<std::string> args,
    // NOLINTNEXTLINE(whitespace/indent_namespace,misc-unused-parameters)
    struct GlobalOptions globals) {
  auto parsed_args = ParseArgs(args);
  auto output_file = EnsureArgument<std::string>("output", parsed_args.first);
  auto using_stdin = parsed_args.first.contains("stdin");
  auto lang = WikipediaCodeToLanguage(
      ExtractLangCode(EnsureArgument<std::string>("wiki", parsed_args.first)));
  std::string input_file;
  if (!using_stdin) {
    input_file = EnsureArgument<std::string>("input", parsed_args.first);
  }

  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});

  if (parsed_args.first.contains("bz2")) {
    spdlog::debug("Using bz2 extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::Bz2Extractor(parser_));
  } else {
    spdlog::debug("Using plain text extractor");
    extractor_ = std::unique_ptr<cs::Extractor>(new cs::TextExtractor(parser_));
  }

  std::ofstream output(output_file,
                       std::ios::out | std::ios::binary | std::ios::trunc);

  if (using_stdin) {
    ProcessFile(std::cin, &output, lang);
  } else {
    std::ifstream input(input_file);
    ProcessFile(input, &output, lang);
    input.close();
  }

  output.close();

  return EXIT_SUCCESS;
}

void ExtractCommand::ProcessFile(std::istream& input, std::ostream* output,
                                 // NOLINTNEXTLINE(whitespace/indent_namespace)
                                 proto::Language lang) {
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

std::string ExtractCommand::ExtractLangCode(const std::string& input) {
  const std::string kSuffix = "wiki";
  auto pos = input.rfind(kSuffix);

  if (pos != std::string::npos && pos == input.size() - kSuffix.size()) {
    return input.substr(0, pos);
  }

  return input;
}
}  // namespace wikiopencite::citescoop::cli
