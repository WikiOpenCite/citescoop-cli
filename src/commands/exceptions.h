// SPDX-FileCopyrightText: 2025 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_COMMANDS_EXCEPTIONS_H_
#define SRC_COMMANDS_EXCEPTIONS_H_

#include <string>

namespace wikiopencite::citescoop::cli {
class CommandException : public std::exception {
 public:
  explicit CommandException(const std::string& description)
      : m_description(description) {}

  virtual ~CommandException() noexcept = default;

  const char* what() const noexcept override { return m_description.c_str(); }

 protected:
  std::string m_description;
};

class FilesystemException : public CommandException {
 public:
  explicit FilesystemException(const std::string& description)
      : CommandException(description) {}

  virtual ~FilesystemException() noexcept = default;
};
}  // namespace wikiopencite::citescoop::cli
#endif  // SRC_COMMANDS_EXCEPTIONS_H_
