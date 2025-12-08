// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "meta_command.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>  // NOLINT(build/c++17)
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
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

#include "base_command.h"

namespace wikiopencite::citescoop::cli {

namespace options = boost::program_options;
namespace fs = std::filesystem;
namespace cs = wikiopencite::citescoop;
namespace proto = wikiopencite::proto;

namespace {
std::string FormatFileSize(size_t size) {
  constexpr std::array<const char*, 5> kUnits = {"B", "K", "M", "G", "T"};
  const int kUnitsSize = 5;

  // Handle zero
  if (size == 0) {
    return "0B";
  }

  // Calculate the appropriate unit
  uint8_t unit_index = 0;
  auto calc_size = static_cast<double>(size);

  const double kDivisor = 1024.0;

  while (calc_size >= kDivisor && unit_index < kUnitsSize - 1) {
    calc_size /= kDivisor;
    unit_index++;
  }

  // Determine decimal places based on size
  int decimal_places;
  if (calc_size >= 100) {  // NOLINT(readability-magic-numbers)
    decimal_places = 0;
  } else if (calc_size >= 10) {  // NOLINT(readability-magic-numbers)
    decimal_places = 1;
  } else {
    decimal_places = 2;
  }

  // Special case: bytes don't need decimals
  if (unit_index == 0) {
    decimal_places = 0;
  }

  // Format the output
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(decimal_places) << calc_size
      << kUnits[unit_index];

  return oss.str();
}
}  // namespace

MetaCommand::MetaCommand()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : BaseCommand("meta", "Display metainformation about a PBF file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.")
    ("pretty,p", options::value<bool>()->zero_tokens()->default_value(false),
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
  auto reader = cs::MessageReader(&input);

  auto header = reader.ReadMessage<proto::FileHeader>();

  size_t total_revisions_size_disk = 0;
  size_t total_revisions_size_mem = 0;
  for (uint64_t i = 0; i < header->revision_count(); i++) {
    auto revision = reader.ReadMessage<proto::Revision>();
    total_revisions_size_disk += revision->ByteSizeLong();
    total_revisions_size_mem += revision->SpaceUsedLong();
  }

  size_t total_page_size_disk = 0;
  size_t total_page_size_mem = 0;
  size_t total_citations = 0;
  for (uint64_t i = 0; i < header->page_count(); i++) {
    auto page = reader.ReadMessage<proto::Page>();
    total_page_size_disk += page->ByteSizeLong();
    total_page_size_mem += page->SpaceUsedLong();
    total_citations += static_cast<size_t>(page->citations_size());
  }

  const google::protobuf::EnumDescriptor* descriptor =
      proto::Language_descriptor();

  auto page_size_disk_str = std::to_string(total_page_size_disk);
  auto page_size_mem_str = std::to_string(total_page_size_mem);
  auto revisions_size_disk_str = std::to_string(total_revisions_size_disk);
  auto revisions_size_mem_str = std::to_string(total_revisions_size_mem);
  if (pretty_print) {
    page_size_disk_str = FormatFileSize(total_page_size_disk);
    page_size_mem_str = FormatFileSize(total_page_size_mem);
    revisions_size_disk_str = FormatFileSize(total_revisions_size_disk);
    revisions_size_mem_str = FormatFileSize(total_revisions_size_mem);
  }

  std::cout << "Language: "
            << descriptor->FindValueByNumber(header->language())->name()
            << '\n';
  std::cout << "Total pages: " << header->page_count() << '\n';
  std::cout << "Total pages size (disk): " << page_size_disk_str << '\n';
  std::cout << "Total pages size (memory): " << page_size_mem_str << '\n';
  std::cout << "Total citations: " << total_citations << '\n';
  std::cout << "Total revisions: " << header->revision_count() << '\n';
  std::cout << "Total revisions size (disk): " << revisions_size_disk_str
            << '\n';
  std::cout << "Total revisions size (memory): " << revisions_size_mem_str
            << '\n';

  return EXIT_SUCCESS;
}

}  // namespace wikiopencite::citescoop::cli
