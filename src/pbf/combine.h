// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_PBF_COMBINE_H_
#define SRC_PBF_COMBINE_H_

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "citescoop/proto/file_header.pb.h"
#include "citescoop/proto/language.pb.h"
#include "citescoop/proto/page.pb.h"
#include "citescoop/proto/revision.pb.h"

#include "cli.h"
#include "io.h"

namespace wikiopencite::citescoop::cli::pbf {

/// @brief Command to combine multiple PBF files into a single output file.
class Combine : public Command {
 public:
  Combine();

  /// @brief Execute the combine command.
  /// @param args CLI arguments passed to the command.
  /// @param globals Global options for the CLI.
  /// @return Exit code representing the result of the command.
  ExitCode Run(std::vector<std::string> args, GlobalOptions globals) override;

 private:
  struct Args {
    std::vector<std::string> inputs;  ///< Input file paths.
    std::string output;               ///< Output file path.
  };

  struct Streams {
    std::vector<std::unique_ptr<io::PbfFile>> inputs;  ///< Input file streams.
    std::ofstream output;                              ///< Output file stream.
    std::ofstream tmp_output;  ///< Temporary output file stream.
  };

  /// @brief Parse command line arguments.
  /// @param args CLI arguments passed to the command.
  void LoadArgs(const std::vector<std::string>& args);

  /// @brief Open all input streams and the output stream.
  void OpenStreams();

  /// @brief Close all open file streams. Note: Does not close the
  /// temporary output stream, as it is closed after writing the file
  /// header.
  void CloseStreams();

  /// @brief Read headers from each input file to validate compatibility.
  void ReadHeaders();

  /// @brief Validate that all input files have the same PBF file type.
  /// @param type File type from the current input header.
  void ValidateFileType(wikiopencite::proto::FileType type);

  /// @brief Validate file-specific header attributes such as language.
  /// @param header The file header read from the input file.
  void ValidateFileSpecificAttributes(
      const wikiopencite::proto::FileHeader& header);

  /// @brief Validate that all input files share the same language.
  /// @param language Language from the current input header.
  void ValidateLanguage(wikiopencite::proto::Language language);

  /// @brief Create a predicate used to filter messages during copy.
  /// @return A function used to determine whether a message should be copied.
  std::function<bool(const google::protobuf::Message&)> PredicateFactory()
      const;

  /// @brief Copy the payload of all input files to the output file.
  void CopyData();

  uint64_t CopyMessagesToTemp();

  void CleanupTempFile() const;

  /// @brief Set additional attributes for the output file.
  /// @param header The file header to set additional attributes for.
  void SetAdditionalAttributes(wikiopencite::proto::FileHeader* header);

  wikiopencite::proto::Language language_;
  Args args_;
  Streams streams_;
  uint64_t total_messages_;
  std::vector<uint64_t> message_counts_;
  wikiopencite::proto::FileType file_type_;
};

}  // namespace wikiopencite::citescoop::cli::pbf

#endif  // SRC_PBF_COMBINE_H_
