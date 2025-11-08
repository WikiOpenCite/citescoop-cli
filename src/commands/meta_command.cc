// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "meta_command.h"

#include <arpa/inet.h>
#include <cstddef>
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
#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision_map.pb.h"
#include "google/protobuf/descriptor.h"
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
namespace {
std::string FormatFileSize(size_t size) {
  const char* units[] = {"B", "K", "M", "G", "T"};
  const int numUnits = 5;

  // Handle negative values
  if (size < 0) {
    return "0B";
  }

  // Handle zero
  if (size == 0) {
    return "0B";
  }

  // Calculate the appropriate unit
  int unitIndex = 0;
  double calc_size = static_cast<double>(size);

  while (calc_size >= 1024.0 && unitIndex < numUnits - 1) {
    calc_size /= 1024.0;
    unitIndex++;
  }

  // Determine decimal places based on size
  int decimalPlaces;
  if (calc_size >= 100) {
    decimalPlaces = 0;  // e.g., "123 MB"
  } else if (calc_size >= 10) {
    decimalPlaces = 1;  // e.g., "12.3 MB"
  } else {
    decimalPlaces = 2;  // e.g., "1.23 MB"
  }

  // Special case: bytes don't need decimals
  if (unitIndex == 0) {
    decimalPlaces = 0;
  }

  // Format the output
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(decimalPlaces) << calc_size
      << units[unitIndex];

  return oss.str();
}
}  // namespace

MetaCommand::MetaCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("meta", "Display metainformation about a PBF file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.")
    ("pretty,p", options::bool_switch()->default_value(false),
    "Display sizes like 1K 234M 2G etc. Uses powers of 1024");
  positional_options_.add("file", 1);

  // clang-format on
}

int MetaCommand::Run(std::vector<std::string> args,
                     // NOLINTNEXTLINE(whitespace/indent_namespace)
                     struct GlobalOptions) {
  auto parsed_args = ParseArgs(args);
  auto file = fs::path(EnsureArgument<std::string>("file", parsed_args.first));
  auto pretty_print = EnsureArgument<bool>("pretty", parsed_args.first);

  std::ifstream input(file, std::ios::in | std::ios::binary);
  auto raw_input = google::protobuf::io::IstreamInputStream(&input);
  auto coded_input =
      std::make_shared<google::protobuf::io::CodedInputStream>(&raw_input);

  auto header = ReadMessage<proto::FileHeader>(coded_input);
  auto revisions = ReadMessage<proto::RevisionMap>(coded_input);

  int total_page_size_disk = 0;
  size_t total_page_size_mem = 0;
  int total_citations = 0;
  for (int64_t i = 0; i < header.second->page_count(); i++) {
    auto page = ReadMessage<proto::Page>(coded_input);
    total_page_size_disk += page.first;
    total_page_size_mem += page.second->SpaceUsedLong();
    total_citations += page.second->citations_size();
  }

  const google::protobuf::EnumDescriptor* descriptor =
      proto::Language_descriptor();

  auto page_size_disk_str = std::to_string(total_page_size_disk);
  auto page_size_mem_str = std::to_string(total_page_size_mem);
  auto revisions_size_disk_str = std::to_string(revisions.first);
  auto revisions_size_mem_str =
      std::to_string(revisions.second->SpaceUsedLong());
  if (pretty_print) {
    page_size_disk_str = FormatFileSize(total_page_size_disk);
    page_size_mem_str = FormatFileSize(total_page_size_mem);
    revisions_size_disk_str = FormatFileSize(revisions.first);
    revisions_size_mem_str = FormatFileSize(revisions.second->SpaceUsedLong());
  }

  std::cout << "Language: "
            << descriptor->FindValueByNumber(header.second->language())->name()
            << '\n';
  std::cout << "Total pages: " << header.second->page_count() << '\n';
  std::cout << "Total pages size (disk): " << page_size_disk_str << '\n';
  std::cout << "Total pages size (memory): " << page_size_mem_str << '\n';
  std::cout << "Total citations: " << total_citations << '\n';
  std::cout << "Total revisions: " << revisions.second->revisions_size()
            << '\n';
  std::cout << "Total revisions size (disk): " << revisions_size_disk_str
            << '\n';
  std::cout << "Total revisions size (memory): " << revisions_size_mem_str
            << '\n';

  return EXIT_SUCCESS;
}

}  // namespace wikiopencite::citescoop::cli
