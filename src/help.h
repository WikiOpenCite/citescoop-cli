// SPDX-FileCopyrightText: 2025-2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SRC_HELP_H_
#define SRC_HELP_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cli.h"

namespace wikiopencite::citescoop::cli {

/// Help is a special topic to provide help dialog to all other topics.
/// It does not behave like a normal topic and should not be used as such.
class Help : public Topic, Command {
 public:
  explicit Help(const std::map<std::string, std::shared_ptr<Topic>>& topics);

  ExitCode Run(std::string topic, std::vector<std::string> args,
               struct GlobalOptions globals) override;

  /// Provide a default implementation for this run inherited from
  /// Command. It should not be used.
  ///
  /// This is generally a bit hacky but it is a simple way to make the
  /// help command function nicely.
  ExitCode Run(std::vector<std::string> args,
               struct GlobalOptions globals) override {
    return ExitCode::kOk;
  }

 private:
  const std::map<std::string, std::shared_ptr<Topic>>& topics_;
};

}  // namespace wikiopencite::citescoop::cli

#endif  // SRC_HELP_H_
