// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "citescoop/openalex.h"
#include "citescoop/proto/file_header.pb.h"

#include "cli.h"
#include "io.h"

namespace {
namespace options = boost::program_options;
namespace cs = wikiopencite::citescoop;
namespace fs = std::filesystem;
}  // namespace

namespace wikiopencite::citescoop::cli::openalex {
Process::Process() : Command("process", "Process OpenAlex dump files") {
  // clang-format off
  cli_options_.add_options()
    ("input,i", options::value<std::string>(), "Input OpenAlex dump file,")
    ("stdin,si", "Read input from stdin.")
    ("authors,a", options::value<std::string>()->required(),
      "Output file for authors.")
    ("institutions,I", options::value<std::string>()->required(),
      "Output file for institutions.")
    ("works,w", options::value<std::string>()->required(),
      "Output file for works.");
  // clang-format on
}

ExitCode Process::Run(std::vector<std::string> args,
                      // NOLINTNEXTLINE(whitespace/indent_namespace)
                      GlobalOptions /*globals*/) {
  auto processor = cs::openalex::SnapshotProcessor();

  LoadArgs(args);
  OpenOutputStreams();

  std::tuple<uint64_t, uint64_t, uint64_t> counts;
  if (args_.stdin) {
    counts = processor.ProcessWorksSnapshot(
        std::cin, &authors_stream_, &institutions_stream_, &works_stream_);
  } else {
    std::ifstream input_stream(args_.input);
    counts = processor.ProcessWorksSnapshot(
        input_stream, &authors_stream_, &institutions_stream_, &works_stream_);
    input_stream.close();
  }

  CloseOutputStreams();
  AddHeaders(counts);

  return ExitCode::kOk;
}

void Process::OpenOutputStreams() {
  authors_stream_ =
      std::ofstream(args_.authors + ".tmp",
                    std::ios::out | std::ios::binary | std::ios::trunc);

  institutions_stream_ =
      std::ofstream(args_.institutions + ".tmp",
                    std::ios::out | std::ios::binary | std::ios::trunc);

  works_stream_ = std::ofstream(
      args_.works + ".tmp", std::ios::out | std::ios::binary | std::ios::trunc);
}

void Process::CloseOutputStreams() {
  authors_stream_.close();
  institutions_stream_.close();
  works_stream_.close();
}

void Process::AddHeaders(std::tuple<uint64_t, uint64_t, uint64_t> counts) {
  std::ifstream tmp_works(args_.works + ".tmp");
  std::ifstream tmp_authors(args_.authors + ".tmp");
  std::ifstream tmp_institutions(args_.institutions + ".tmp");
  std::ofstream works(args_.works,
                      std::ios::out | std::ios::binary | std::ios::trunc);
  std::ofstream authors(args_.authors,
                        std::ios::out | std::ios::binary | std::ios::trunc);
  std::ofstream institutions(
      args_.institutions, std::ios::out | std::ios::binary | std::ios::trunc);

  auto header = proto::FileHeader();
  header.set_count(std::get<0>(counts));
  header.set_type(proto::FileType::FILE_TYPE_OPENALEX_AUTHORS);
  io::PrependHeader(header, tmp_authors, &authors);

  header.set_count(std::get<1>(counts));
  header.set_type(proto::FileType::FILE_TYPE_OPENALEX_INSTITUTIONS);
  io::PrependHeader(header, tmp_institutions, &institutions);

  header.set_count(std::get<2>(counts));
  header.set_type(proto::FileType::FILE_TYPE_OPENALEX_WORKS);
  io::PrependHeader(header, tmp_works, &works);

  tmp_authors.close();
  tmp_institutions.close();
  tmp_works.close();
  authors.close();
  institutions.close();
  works.close();

  fs::remove(args_.authors + ".tmp");
  fs::remove(args_.institutions + ".tmp");
  fs::remove(args_.works + ".tmp");
}

void Process::LoadArgs(std::vector<std::string> args) {
  auto parsed_args = ParseArgs(args);
  args_.stdin = parsed_args.first.contains("stdin");

  if (!args_.stdin)
    args_.input = EnsureArgument<std::string>("input", parsed_args.first);

  args_.authors = EnsureArgument<std::string>("authors", parsed_args.first);
  args_.works = EnsureArgument<std::string>("works", parsed_args.first);
  args_.institutions =
      EnsureArgument<std::string>("institutions", parsed_args.first);
}
}  // namespace wikiopencite::citescoop::cli::openalex
