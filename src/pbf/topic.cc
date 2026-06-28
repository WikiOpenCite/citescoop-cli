// SPDX-FileCopyrightText: 2026 The University of St Andrews
// SPDX-License-Identifier: GPL-3.0-or-later

#include "topic.h"

#include <memory>

#include "cat.h"
#include "cli.h"
#include "meta.h"

namespace wikiopencite::citescoop::cli::pbf {

std::unique_ptr<Topic> NewPbfTopic() {
  auto topic =
      std::make_unique<Topic>("pbf", "View and manipulate generated PBF files");

  topic->Register(std::shared_ptr<Command>(new Cat()));
  topic->Register(std::shared_ptr<Command>(new Meta()));
  return topic;
}
}  // namespace wikiopencite::citescoop::cli::pbf
