#ifndef SRC_EXCEPTIONS_H_
#define SRC_EXCEPTIONS_H_

#include <exception>
#include <string>

#include "cli.h"

namespace wikiopencite::citescoop::cli::exceptions {

class CliException : public std::exception {
 public:
  explicit CliException() : CliException("and unexpected error occurred") {}

  explicit CliException(std::string description)
      : error_description_(std::move(description)) {}

  [[nodiscard]] const char* what() const noexcept override {
    return error_description_.c_str();
  }

  ExitCode code() const { return kExitCode; }

 protected:
  std::string error_description_;
  static const ExitCode kExitCode = ExitCode::kGeneralError;
};

class UserInputException : public CliException {
 public:
  UserInputException() : UserInputException("invalid input") {}

  UserInputException(const char* msg) : CliException(msg) {}

 protected:
  static const ExitCode kExitCode = ExitCode::kInputError;
};

class UnsupportedFileType : public UserInputException {
 public:
  UnsupportedFileType() : UnsupportedFileType("file type not supported") {}

  UnsupportedFileType(const char* msg) : UserInputException(msg) {}
};
}  // namespace wikiopencite::citescoop::cli::exceptions

#endif
