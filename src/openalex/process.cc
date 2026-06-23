// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "process.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "boost/program_options/variables_map.hpp"
#include "citescoop/openalex.h"

#include "cli.h"

namespace {
namespace options = boost::program_options;
namespace cs = wikiopencite::citescoop;
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

  auto parsed_args = ParseArgs(args);
  auto using_stdin = parsed_args.first.contains("stdin");

  OpenOutputStreams(parsed_args.first);

  if (using_stdin) {
    processor.ProcessWorksSnapshot(std::cin, &authors_stream_,
                                   &institutions_stream_, &works_stream_);
  } else {
    std::ifstream input_stream(
        EnsureArgument<std::string>("input", parsed_args.first));
    processor.ProcessWorksSnapshot(input_stream, &authors_stream_,
                                   &institutions_stream_, &works_stream_);
    input_stream.close();
  }

  CloseOutputStreams();

  return ExitCode::kOk;
}

void Process::OpenOutputStreams(
    // NOLINTNEXTLINE(whitespace/indent_namespace)
    const boost::program_options::variables_map& args) {
  authors_stream_ =
      std::ofstream(args["authors"].as<std::string>(),
                    std::ios::out | std::ios::binary | std::ios::trunc);

  institutions_stream_ =
      std::ofstream(args["institutions"].as<std::string>(),
                    std::ios::out | std::ios::binary | std::ios::trunc);

  works_stream_ =
      std::ofstream(args["works"].as<std::string>(),
                    std::ios::out | std::ios::binary | std::ios::trunc);
}

void Process::CloseOutputStreams() {
  authors_stream_.close();
  institutions_stream_.close();
  works_stream_.close();
}
}  // namespace wikiopencite::citescoop::cli::openalex
