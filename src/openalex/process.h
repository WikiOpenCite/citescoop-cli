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
  /// @brief Open the output streams
  /// @param args Command line arguments passed by user.
  void OpenOutputStreams(const boost::program_options::variables_map& args);

  /// @brief Close the previously opened output streams
  void CloseOutputStreams();

  std::ofstream authors_stream_;
  std::ofstream institutions_stream_;
  std::ofstream works_stream_;
};
}  // namespace wikiopencite::citescoop::cli::openalex

#endif  // SRC_OPENALEX_PROCESS_H_
