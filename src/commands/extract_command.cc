// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extract_command.h"

// NOLINTNEXTLINE(misc-include-cleaner)
#include <arpa/inet.h>

#include <cstdint>
#include <cstdlib>
// NOLINTNEXTLINE(build/c++17)
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <regex>
#include <string>
#include <vector>

#include "boost/algorithm/string/predicate.hpp"
#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "citescoop/extract.h"
#include "citescoop/parser.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "fmt/ranges.h"
#include "spdlog/spdlog.h"

#include "../langmap.h"
#include "base_command.h"
#include "exceptions.h"

namespace wikiopencite::citescoop::cli {

namespace algo = boost::algorithm;
namespace options = boost::program_options;
namespace fs = std::filesystem;
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
    ("dump-root,I", options::value<std::string>(),
      "Root directory for data dump. Use when using bz2 files (warning, slow) "
      "with automatic file discovery.")
    ("output-dir,O", options::value<std::string>(),
      "Directory for output files when passing the dump-root.")
    ("wiki", options::value<std::string>()->default_value("all"),
      "Name of wiki being processed. When using bulk mode (i.e. with dump-root)"
      " this will select which wiki to process. When using stdin mode or file "
      "input mode, this will correctly set the language marker in the file "
      "header.");
  // clang-format on
}

int ExtractCommand::Run(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    std::vector<std::string> args,
    // NOLINTNEXTLINE(whitespace/indent_namespace,misc-unused-parameters)
    struct GlobalOptions globals) {
  auto parsed_args = ParseArgs(args);

  if (parsed_args.first.contains("dump-root")) {
    return RunMultiWikiBz2(parsed_args.first);
  }

  RunSingleWikiText(parsed_args.first);

  return EXIT_SUCCESS;
}

int ExtractCommand::RunMultiWikiBz2(const options::variables_map& args) {
  EnsureArgument<std::string>("output-dir", args);

  dump_root_ = fs::path(EnsureArgument<std::string>("dump-root", args));
  output_dir_ = fs::path();

  spdlog::debug(
      "Running extractor with following options: root: {}, output: {}",
      dump_root_.string(), output_dir_.string());

  auto wikis = GetWikis(EnsureArgument<std::string>("wiki", args));
  auto files = GetDumpFiles(wikis);

  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});
  extractor_ = std::unique_ptr<cs::Extractor>(new cs::Bz2Extractor(parser_));

  for (const auto& file : files) {
    ProcessFile(file);
  }

  return EXIT_SUCCESS;
}

int ExtractCommand::RunSingleWikiText(const options::variables_map& args) {
  spdlog::trace("Running single text extraction");
  auto output = EnsureArgument<std::string>("output", args);
  auto using_stdin = args.contains("stdin");
  auto lang = WikipediaCodeToLanguage(
      ExtractLangCode(EnsureArgument<std::string>("wiki", args)));

  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});
  extractor_ = std::unique_ptr<cs::Extractor>(new cs::TextExtractor(parser_));

  std::string input;
  if (!using_stdin) {
    input = EnsureArgument<std::string>("input", args);
  }

  if (using_stdin) {
    spdlog::trace("Reading input from stdin");
    std::ofstream output_file(
        output, std::ios::out | std::ios::binary | std::ios::trunc);
    ProcessFile(std::cin, output_file, lang);
    output_file.close();
  } else {
    ProcessFile(input, output, lang);
  }

  return EXIT_SUCCESS;
}

std::vector<std::string> ExtractCommand::GetWikis(
    const std::string& wiki_filter) {  // NOLINT(whitespace/indent_namespace)
  auto wikis = std::vector<std::string>();
  try {
    if (wiki_filter == "all") {
      if (!fs::exists(dump_root_) || !fs::is_directory(dump_root_)) {
        throw FilesystemException("Could not find directory " +
                                  dump_root_.string());
      }

      for (const auto& entry : fs::directory_iterator(dump_root_)) {
        if (entry.is_directory() &&
            algo::ends_with(entry.path().filename().string(), "wiki")) {
          wikis.push_back(entry.path().filename().string());
        }
      }
    } else {
      if (!fs::exists(dump_root_ / wiki_filter) ||
          !fs::is_directory(dump_root_ / wiki_filter)) {
        throw FilesystemException("Could not find directory " +
                                  (dump_root_ / wiki_filter).string());
      }
      wikis.push_back(wiki_filter);
    }
  } catch (const fs::filesystem_error& e) {
    throw FilesystemException(e.what());
  }

  spdlog::debug("Going to extract from the following wiki dumps: {}",
                fmt::join(wikis, ", "));
  return wikis;
}

std::vector<std::string> ExtractCommand::GetDumpFiles(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const std::vector<std::string>& wikis) {
  auto files = std::vector<std::string>();
  const std::regex kFilePattern(R"(^.+-.*-pages-meta-history\d+\.xml.*\.bz2$)");

  for (const auto& wiki : wikis) {
    // NOLINTNEXTLINE(readability-identifier-naming)
    const fs::path dir = dump_root_ / fs::path(wiki);
    try {
      if (!fs::exists(dir) || !fs::is_directory(dir)) {
        throw FilesystemException("Could not find directory " + dir.string());
      }

      for (const auto& entry : fs::directory_iterator(dir)) {
        if (fs::is_regular_file(entry.status())) {
          // NOLINTNEXTLINE(readability-identifier-naming)
          const std::string filename = entry.path().filename().string();

          if (std::regex_match(filename, kFilePattern)) {
            files.push_back(entry.path().string());
          }
        }
      }
    } catch (const fs::filesystem_error& e) {
      throw FilesystemException(e.what());
    }
  }

  spdlog::debug("Found {} bz2 files", files.size());

  return files;
}

void ExtractCommand::ProcessFile(std::string path) {
  spdlog::debug("Processing file {}", path);

  std::ifstream input_file(path);

  // NOLINTNEXTLINE(readability-identifier-naming)
  const fs::path file_path = path;
  auto base_name = file_path.filename().string();
  base_name = base_name.substr(0, base_name.rfind('.'));

  auto prefix = file_path.filename().string();
  prefix = prefix.substr(0, prefix.find('-'));
  auto code = ExtractLangCode(prefix);

  // NOLINTNEXTLINE(readability-identifier-naming)
  const fs::path out_dir = output_dir_ / prefix;
  EnsureDirectory(out_dir);
  // NOLINTNEXTLINE(readability-identifier-naming)
  const fs::path output = out_dir / (base_name + ".pbf");

  spdlog::trace("Writing output of {} to {}", file_path.string(),
                output.string());
  std::ofstream output_file(output,
                            std::ios::out | std::ios::binary | std::ios::trunc);

  ProcessFile(input_file, output_file, WikipediaCodeToLanguage(code));

  input_file.close();
  output_file.close();
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void ExtractCommand::ProcessFile(std::string input, const std::string& output,
                                 // NOLINTNEXTLINE(whitespace/indent_namespace)
                                 proto::Language lang) {
  spdlog::debug("Reading input from {}", input);
  std::ifstream input_file(input);
  std::ofstream output_file(output,
                            std::ios::out | std::ios::binary | std::ios::trunc);

  ProcessFile(input_file, output_file, lang);

  input_file.close();
  output_file.close();
}

void ExtractCommand::ProcessFile(std::istream& input, std::ostream& output,
                                 // NOLINTNEXTLINE(whitespace/indent_namespace)
                                 proto::Language lang) {
  spdlog::trace("Starting extraction from input stream");

  auto [pages, revisions] = extractor_->Extract(input);

  spdlog::trace("Finished extracting citations.");
  spdlog::debug("Stored {} pages and {} revisions.", pages->size(),
                revisions->revisions_size());

  auto fileheader = proto::FileHeader();
  fileheader.set_page_count(static_cast<int64_t>(pages->size()));
  fileheader.set_language(lang);

  spdlog::trace("Writing file header to output");
  WriteMessage(fileheader, output);

  spdlog::trace("Writing revisions to output");
  WriteMessage(*revisions, output);

  spdlog::trace("Writing pages to output");
  for (auto& page : *pages) {
    WriteMessage(page, output);
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

void ExtractCommand::EnsureDirectory(const std::filesystem::path& path) {
  if (!fs::exists(path)) {
    fs::create_directories(path);
  } else if (!fs::is_directory(path)) {
    throw FilesystemException("File is not a directory: " + path.string());
  }
}

void ExtractCommand::WriteMessage(const google::protobuf::Message& message,
                                  // NOLINTNEXTLINE(whitespace/indent_namespace)
                                  std::ostream& output) {
  std::string serialised_message;
  uint32_t serialised_size;

  message.SerializeToString(&serialised_message);
  // NOLINTNEXTLINE(misc-include-cleaner)
  serialised_size = htonl(serialised_message.size());

  output.write(reinterpret_cast<char*>(&serialised_size),
               sizeof(serialised_size));
  output.write(serialised_message.c_str(),
               static_cast<std::streamsize>(serialised_message.size()));
}

void ExtractCommand::DisplayProgress(double val) {
  std::cout << "Progress: " << val << "%\r" << std::flush;
}

template <typename T>
T ExtractCommand::EnsureArgument(const std::string& arg,
                                 // NOLINTNEXTLINE(whitespace/indent_namespace)
                                 const options::variables_map& args) {
  if (!args.contains(arg)) {
    throw MissingArgumentException("Missing required argument " + arg);
  }

  return args[arg].as<T>();
}

}  // namespace wikiopencite::citescoop::cli
