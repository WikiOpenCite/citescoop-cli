// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_OPENALEX_PROCESS_H_
#define SRC_OPENALEX_PROCESS_H_

#include <fstream>
#include <string>
#include <vector>

#include "boost/program_options/variables_map.hpp"

#include "cli.h"

namespace wikiopencite::citescoop::cli::openalex {
class Process : public Command {
 public:
  Process();

  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Args {
    std::string input;
    bool stdin;
    std::string authors;
    std::string institutions;
    std::string works;
  };

  /// @brief Open the output streams
  void OpenOutputStreams();

  /// @brief Close the previously opened output streams
  void CloseOutputStreams();

  void AddHeaders(std::tuple<uint64_t, uint64_t, uint64_t> counts);
  void LoadArgs(std::vector<std::string> args);

  std::ofstream authors_stream_;
  std::ofstream institutions_stream_;
  std::ofstream works_stream_;
  Args args_;
};
}  // namespace wikiopencite::citescoop::cli::openalex

#endif  // SRC_OPENALEX_PROCESS_H_
