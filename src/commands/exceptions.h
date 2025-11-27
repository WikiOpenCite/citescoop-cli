// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_EXCEPTIONS_H_
#define SRC_COMMANDS_EXCEPTIONS_H_

#include <exception>
#include <string>
#include <utility>

namespace wikiopencite::citescoop::cli {
class CommandException : public std::exception {
 public:
  explicit CommandException(std::string description)
      : error_description_(std::move(description)) {}

  [[nodiscard]] const char* what() const noexcept override {
    return error_description_.c_str();
  }

 protected:
  std::string error_description_;
};

class FilesystemException : public CommandException {
 public:
  explicit FilesystemException(const std::string& description)
      : CommandException(description) {}
};

class MissingArgumentException : public CommandException {
 public:
  explicit MissingArgumentException(const std::string& description)
      : CommandException(description) {}
};
}  // namespace wikiopencite::citescoop::cli
#endif  // SRC_COMMANDS_EXCEPTIONS_H_
