// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "extract_command.h"

#include <arpa/inet.h>
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
#include "boost/program_options/positional_options.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "citescoop/extract.h"
#include "citescoop/parser.h"
#include "citescoop/proto/file_header.pb.h"
#include "fmt/ranges.h"
#include "spdlog/spdlog.h"

#include "../langmap.h"
#include "../progress_stream_buf.h"
#include "exceptions.h"

namespace algo = boost::algorithm;
namespace options = boost::program_options;
namespace fs = std::filesystem;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

namespace wikiopencite::citescoop::cli {
ExtractCommand::ExtractCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("extract", "Extract citations from") {
  cli_options_.add_options()("dump-root,I",
                             options::value<std::string>()->required(),
                             "Root directory for latest data dump")(
      "output,o", options::value<std::string>()->required(),
      "Directory for output files")(
      "wiki", options::value<std::string>()->default_value("all"),
      "Specify the wiki to process. If no wiki "
      "is specified, all will be processed");
}

int ExtractCommand::Run(std::vector<std::string> args,
                        // NOLINTNEXTLINE(whitespace/indent_namespace)
                        struct GlobalOptions globals) {
  auto parsed_args = ParseArgs(args);

  dump_root_ = fs::path(parsed_args.first["dump-root"].as<std::string>());
  output_dir_ = fs::path(parsed_args.first["output"].as<std::string>());

  spdlog::debug(
      "Running extractor with following options: root: {}, output: {}",
      dump_root_.string(), output_dir_.string());

  auto wikis = GetWikis(parsed_args.first["wiki"].as<std::string>());
  auto files = GetDumpFiles(wikis);

  parser_ = std::make_shared<cs::Parser>(
      cs::ParserOptions{.ignore_invalid_ident = true});
  extractor_ = std::unique_ptr<cs::Bz2Extractor>(new cs::Bz2Extractor(parser_));

  for (const auto& file : files) {
    ProcessFile(file);
  }

  return EXIT_SUCCESS;
}

std::vector<std::string> ExtractCommand::GetWikis(std::string wiki_filter) {
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
    std::vector<std::string> wikis) {
  auto files = std::vector<std::string>();
  std::regex file_pattern(R"(^.+-.*-pages-meta-history\d+\.xml.*\.bz2$)");

  for (const auto& wiki : wikis) {
    fs::path dir = dump_root_ / fs::path(wiki);
    try {
      if (!fs::exists(dir) || !fs::is_directory(dir)) {
        throw FilesystemException("Could not find directory " + dir.string());
      }

      for (const auto& entry : fs::directory_iterator(dir)) {
        if (fs::is_regular_file(entry.status())) {
          std::string filename = entry.path().filename().string();

          if (std::regex_match(filename, file_pattern)) {
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

  std::ifstream dump_file(path);
  dump_file.seekg(0, std::ios::end);
  auto file_size = dump_file.tellg();
  dump_file.seekg(0, std::ios::beg);

  ProgressStreamBuf progress_stream(dump_file.rdbuf(), file_size);
  std::istream tracked_stream(&progress_stream);

  std::atomic<bool> done{false};
  std::thread progress_thread([&]() {
    while (!done) {
      std::cout << static_cast<double>(progress_stream.getBytesRead()) / 1024
                << "%\r" << std::flush;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });

  fs::path fp = path;
  auto [pages, revisions] = extractor_->Extract(tracked_stream);

  done = true;
  progress_thread.join();

  auto fileheader = proto::FileHeader();
  fileheader.set_page_count(static_cast<int64_t>(pages->size()));

  auto prefix = fp.filename().string();
  prefix = prefix.substr(0, prefix.find("-"));
  auto code = ExtractLangCode(prefix);
  fileheader.set_language(WikipediaCodeToLanguage(code));

  auto base_name = fp.filename().string();
  base_name = base_name.substr(0, base_name.rfind("."));

  fs::path out_dir = output_dir_ / prefix;
  EnsureDirectory(out_dir);
  fs::path out_path = out_dir / (base_name + ".pbf");

  spdlog::trace("Writing output of {} to {}", fp.string(), out_path.string());
  std::ofstream output_file(out_path,
                            std::ios::out | std::ios::binary | std::ios::trunc);

  WriteMessage(fileheader, output_file);
  WriteMessage(*revisions, output_file);

  for (auto& page : *pages) {
    WriteMessage(page, output_file);
  }

  output_file.close();
}

std::string ExtractCommand::ExtractLangCode(const std::string& input) {
  const std::string suffix = "wiki";
  auto pos = input.rfind(suffix);

  if (pos != std::string::npos && pos == input.size() - suffix.size()) {
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
                                  std::ofstream& output) {
  std::string serialised_message;
  uint32_t serialised_size;

  message.SerializeToString(&serialised_message);
  serialised_size = htonl(serialised_message.size());

  output.write(reinterpret_cast<char*>(&serialised_size),
               sizeof(serialised_size));
  output.write(serialised_message.c_str(),
               static_cast<std::streamsize>(serialised_message.size()));
}

void ExtractCommand::DisplayProgress(double val) {
  std::cout << "Progress: " << val << "%\r" << std::flush;
}

}  // namespace wikiopencite::citescoop::cli
