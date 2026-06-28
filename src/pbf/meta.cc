// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "meta.h"

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
#include "boost/program_options/variables_map.hpp"
#include "citescoop/io.h"
#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"
#include "google/protobuf/descriptor.h"
#include "spdlog/spdlog.h"

#include "cli.h"
#include "exceptions.h"
#include "io.h"

namespace wikiopencite::citescoop::cli::pbf {

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

Meta::Meta()
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    : Command("meta", "Display metainformation about a PBF file") {
  // clang-format off
  cli_options_.add_options()
    ("file", options::value<std::string>()->required(), "Input file.")
    ("pretty,p", options::value<bool>()->zero_tokens()->default_value(false),
    "Display sizes like 1K 234M 2G etc. Uses powers of 1024");
  positional_options_.add("file", 1);

  // clang-format on
}

ExitCode Meta::Run(std::vector<std::string> args,
                   // NOLINTNEXTLINE(whitespace/indent_namespace)
                   struct GlobalOptions) {
  LoadArgs(args);

  OpenFile();

  try {
    LoadHeader();
  } catch (const exceptions::UnsupportedFileType& e) {
    spdlog::critical("Failed to read input file: {}", e.what());
    std::cerr << e.what() << '\n';

    return e.code();
  }

  size_t size_on_disk;
  size_t size_in_mem;

  try {
    auto sizes = CalculateSize();
    size_on_disk = sizes.first;
    size_in_mem = sizes.second;
  } catch (const exceptions::UnsupportedFileType& e) {
    spdlog::critical("Failed to read input file: {}", e.what());
    std::cerr << e.what() << '\n';

    return e.code();
  }

  auto attributes_str = FormatAdditionalAttributes();
  auto size_on_disk_str = FormatSize(size_on_disk);
  auto size_in_mem_str = FormatSize(size_in_mem);

  std::cout << "Attributes: " << attributes_str << '\n';
  std::cout << "Total messages: " << header_->count() << '\n';
  std::cout << "Total message size (disk): " << size_on_disk_str << '\n';
  std::cout << "Total message size (memory): " << size_in_mem_str << '\n';

  return ExitCode::kOk;
}

void Meta::LoadArgs(std::vector<std::string> args) {
  auto parsed_args = ParseArgs(args);

  args_.input = EnsureArgument<std::string>("file", parsed_args.first);
  args_.pretty = parsed_args.first.contains("pretty");
}

void Meta::OpenFile() {
  input_ = io::OpenPbfFile(args_.input);
}

void Meta::LoadHeader() {
  header_ = io::ReadPbfHeader(input_.get());
}

std::pair<size_t, size_t> Meta::CalculateSize() {
  size_t mem_size = 0;
  size_t disk_size = 0;

  for (uint64_t i = 0; i < header_->count(); i++) {
    auto message = io::ReadGenericMessage(input_.get(), header_->type());
    disk_size += message->ByteSizeLong();
    mem_size += message->SpaceUsedLong();
  }
  return std::make_pair(disk_size, mem_size);
}

std::string Meta::FormatAdditionalAttributes() {
  switch (header_->type()) {
    case proto::FileType::FILE_TYPE_PAGES:
    case proto::FileType::FILE_TYPE_REVISIONS: {
      const google::protobuf::EnumDescriptor* descriptor =
          proto::Language_descriptor();
      return "Language: " +
             std::string(descriptor
                             ->FindValueByNumber(
                                 header_->dump_file_attributes().language())
                             ->name());
    }

    default:
      return "";
  }
}

std::string Meta::FormatSize(size_t size) {
  if (args_.pretty)
    return FormatFileSize(size);
  return std::to_string(size);
}

}  // namespace wikiopencite::citescoop::cli::pbf
