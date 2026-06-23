// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "topic.h"

#include <memory>

#include "cli.h"
#include "process.h"

namespace wikiopencite::citescoop::cli::openalex {

std::unique_ptr<Topic> NewOpenalexTopic() {
  auto topic = std::make_unique<Topic>(
      "openalex", "Process and inspect OpenAlex snapshot files");

  topic->Register(std::shared_ptr<Command>(new Process()));
  return topic;
}
}  // namespace wikiopencite::citescoop::cli::openalex
